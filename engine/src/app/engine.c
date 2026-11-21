/* 1stPRTS engine: boot and main loop. MIT License. */
#include <lauxlib.h>
#include <stdio.h>
#include <string.h>

#include "app/engine_internal.h"
#include "app/mods.h"
#include "app/report.h"
#include "app/snapshot.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"
#include "prts/prts_rng.h"
#include "prts/prts_xr.h"

static const char *ENGINE_KEY = "prts.engine";

prts_engine *prts_engine_from_lua(lua_State *L)
{
    return prts_script_get_context(prts_script_from_lua(L), ENGINE_KEY);
}

/* Finds the data folder: --data, next to the executable, or (developer builds) the
 * source tree. Fills `searched` with every place that was tried. */
static char *find_data_dir(const prts_engine_config *cfg, prts_str *searched)
{
    if (cfg->data_dir) {
        prts_str_appendf(searched, "%s", cfg->data_dir);
        return prts_fs_is_dir(cfg->data_dir) ? prts_strdup(cfg->data_dir) : NULL;
    }
    char *base = prts_fs_base_dir();
    char *next_to_exe = prts_path_join(base, "data");
    prts_free(base);
    prts_str_appendf(searched, "%s", next_to_exe);
    if (prts_fs_is_dir(next_to_exe)) return next_to_exe;
    prts_free(next_to_exe);
#if defined(PRTS_DEV)
    prts_str_appendf(searched, ", %s", PRTS_SOURCE_DATA_DIR);
    if (prts_fs_is_dir(PRTS_SOURCE_DATA_DIR)) return prts_strdup(PRTS_SOURCE_DATA_DIR);
#endif
    return NULL;
}

static void resume_audio(void *audio) { prts_audio_resume(audio); }

prts_engine *prts_engine_create(const prts_engine_config *cfg, char *err, size_t err_size)
{
    prts_str searched = {0};
    char *data_dir = find_data_dir(cfg, &searched);
    if (!data_dir) {
        /* Spec: Data folder missing */
        snprintf(err, err_size, "The data folder is missing. 1stPRTS looked in: %s", prts_str_cstr(&searched));
        prts_str_free(&searched);
        return NULL;
    }
    prts_str_free(&searched);
    char *base = prts_path_join(data_dir, "base");
    if (!prts_fs_is_dir(base)) {
        snprintf(err, err_size, "The data folder %s has no base/ game folder.", data_dir);
        prts_free(base);
        prts_free(data_dir);
        return NULL;
    }

    prts_engine *e = prts_calloc(1, sizeof(*e));
    e->cfg = *cfg;
    e->data_dir = data_dir;
    e->feel_motion = 1.0f;
    e->user_dir = cfg->user_dir ? prts_strdup(cfg->user_dir) : prts_fs_user_dir();
    if (!e->user_dir) e->user_dir = prts_path_join(data_dir, "../user");
    prts_array_init(&e->move_done, sizeof(prts_entity));
    prts_clock_init(&e->clock, PRTS_TICK_HZ, 5);
    prts_render_settings_default(&e->render);
    prts_engine_xr_init(e);
    e->vfs = prts_vfs_create();
    prts_vfs_set_user_dir(e->vfs, e->user_dir);
    e->jobs = prts_jobs_create(cfg->headless ? 0 : prts_jobs_default_threads());
    e->script = prts_script_create(e->vfs, cfg->seed);
    if (cfg->lua_coverage) prts_script_enable_coverage(e->script);
    prts_script_set_context(e->script, ENGINE_KEY, e);
    prts_script_set_time(e->script, 0, e->clock.tick_seconds);

    if (!prts_engine_check_base(e, base, err, err_size)) {
        prts_free(base);
        prts_engine_destroy(e);
        return NULL;
    }
    prts_vfs_mount(e->vfs, base, "base");
    prts_free(base);
    prts_engine_mount_mods(e);

    e->world = prts_world_create();
    e->cworld = prts_cworld_create();
    e->scene = prts_scene_create();
    e->input = prts_input_create(prts_platform_key_from_name);
    if (!cfg->headless) {
        prts_platform_config pc = {e->title, cfg->width, cfg->height, cfg->hidden, 3, 3};
        e->platform = prts_platform_create(&pc);
        if (!e->platform) {
            /* Spec: OpenGL context unavailable */
            snprintf(err, err_size, "%s needs OpenGL 3.3 (core profile) or WebGL 2, which this system could not provide.",
                     e->title);
            prts_engine_destroy(e);
            return NULL;
        }
        e->gfx = prts_gfx_create(prts_platform_gl_getproc, e->vfs, prts_platform_gles(e->platform));
        if (!e->gfx) {
            snprintf(err, err_size, "The OpenGL driver is missing functions %s needs.", e->title);
            prts_engine_destroy(e);
            return NULL;
        }
        prts_log_info("renderer: %s", prts_gfx_info(e->gfx));
        prts_xr_detect(prts_xr_get());
    } else {
        e->gfx = prts_gfx_create(NULL, e->vfs, false);
    }

    e->audio = prts_audio_create(e->vfs, !cfg->headless, NULL);
    prts_platform_set_gesture_handler(resume_audio, e->audio); /* browsers: audio after a click */

    prts_engine_open_game_api(e);
    prts_engine_open_audio_api(e);
    prts_engine_open_save_api(e);
    prts_engine_open_xr_api(e);
#if defined(PRTS_DEV) && !defined(__EMSCRIPTEN__)
    /* developer builds on desktop: on for interactive runs, opt-in for tests */
    e->hot_reload = cfg->hot_reload || (!cfg->headless && !cfg->scenario && !cfg->test_lua && !cfg->replay);
    if (e->hot_reload) prts_script_enable_hot_reload(e->script);
#endif
    prts_engine_ui_create(e);
    if (!prts_script_run_file(e->script, "scripts/main.lua")) {
        snprintf(err, err_size, "The boot script scripts/main.lua failed: %s", prts_script_last_error(e->script));
        prts_engine_destroy(e);
        return NULL;
    }
    prts_engine_finish_scene(e);
    if (cfg->junit) {
        e->report = prts_report_create(cfg->test_lua ? "lua_unit_tests" : cfg->replay ? "replays" : "scenarios");
    }
    if (cfg->scenario && !prts_engine_start_scenario(e, cfg->scenario)) {
        snprintf(err, err_size, "The scenario %s could not be started: %s", cfg->scenario,
                 prts_script_last_error(e->script));
        if (e->report) prts_report_fail(e->report, cfg->scenario, "(start)", err);
        prts_engine_destroy(e);
        return NULL;
    }
    if (!prts_engine_replay_start(e, err, err_size)) {
        prts_engine_destroy(e);
        return NULL;
    }
    return e;
}

void prts_engine_destroy(prts_engine *e)
{
    if (!e) return;
    prts_jobs_destroy(e->jobs);
    if (e->script) prts_engine_replay_finish(e);
    prts_engine_finish_report(e);
    if (e->cfg.lua_coverage && e->script) prts_script_write_coverage(e->script, e->cfg.lua_coverage);
    prts_engine_ui_destroy(e);
    prts_script_destroy(e->script);
    prts_gfx_destroy(e->gfx);
    prts_platform_set_gesture_handler(NULL, NULL);
    prts_audio_destroy(e->audio);
    prts_platform_destroy(e->platform);
    prts_input_destroy(e->input);
    prts_scene_destroy(e->scene);
    prts_cworld_destroy(e->cworld);
    prts_world_destroy(e->world);
    prts_vfs_destroy(e->vfs);
    prts_array_free(&e->move_done);
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(e->mod_versions); i++) prts_free(e->mod_versions[i]);
    prts_free(e->scenario_message);
    prts_free(e->title);
    prts_free(e->data_dir);
    prts_free(e->user_dir);
    prts_free(e);
}

void prts_engine_finish_scene(prts_engine *e)
{
    prts_cworld_set_heightfield(e->cworld, e->scene->terrain);
    prts_cworld_sync(e->cworld, e->world);
    if (e->scene_ever_ready && e->scene_ready == e->scene->revision) return;
    if (!e->cfg.headless) prts_scene_bake_sun(e->scene, e->cworld, e->world);
    e->scene_ready = e->scene->revision;
    e->scene_ever_ready = true;
}

static void on_move_done(prts_entity ent, void *user)
{
    prts_engine *e = user;
    prts_array_push(&e->move_done, &ent);
}

static void emit_entity_event(prts_engine *e, const char *event, prts_entity ent)
{
    lua_State *L = prts_script_lua(e->script);
    lua_pushinteger(L, (lua_Integer)ent);
    lua_pushstring(L, prts_world_name(e->world, ent));
    prts_script_emit(e->script, event, 2);
}

void prts_engine_tick(prts_engine *e)
{
    prts_engine_xr_before_tick(e); /* XR body input, before a replay records or replaces it */
    prts_engine_replay_before_tick(e);
    float dt = (float)e->clock.tick_seconds;
    prts_world_begin_tick(e->world);
    prts_array_clear(&e->move_done);
    prts_world_tick_movers(e->world, dt, on_move_done, e);
    prts_cworld_sync(e->cworld, e->world);
    if (e->has_commander) {
        e->commander.feel.motion = e->feel_motion; /* the camera motion setting */
        prts_commander_tick(&e->commander, e->world, e->cworld, e->input, dt);
        prts_engine_xr_after_tick(e);
        if (prts_input_pressed(e->input, "use")) {
            prts_entity target = prts_commander_target(&e->commander, e->world, e->cworld);
            if (target != PRTS_ENTITY_NONE) emit_entity_event(e, "use", target);
        }
    }
    for (size_t i = 0; i < e->move_done.len; i++) {
        emit_entity_event(e, "move_done", *(prts_entity *)prts_array_at(&e->move_done, i));
    }
    e->clock.tick++;
    prts_script_set_time(e->script, e->clock.tick, e->clock.tick_seconds);
    lua_State *L = prts_script_lua(e->script);
    lua_pushinteger(L, (lua_Integer)e->clock.tick);
    prts_script_emit(e->script, "tick", 1);
    prts_input_end_tick(e->input);
    prts_input_tick_overrides(e->input);
    prts_engine_replay_after_tick(e); /* before the scenario drives the next tick */
    if (e->scenario_active) prts_engine_scenario_step(e);
}

static void apply_look(prts_engine *e)
{
    if (!e->has_commander || prts_engine_replaying(e)) return;
    float s = e->commander.p.look_sensitivity;
    prts_commander_look(&e->commander, -prts_input_value(e->input, "look_x") * s,
                        -prts_input_value(e->input, "look_y") * s);
}

static void render_frame(prts_engine *e, double alpha)
{
    if (!e->platform) return;
    prts_engine_ui_draw_surfaces(e); /* in-world UI surfaces, shown by materials in every view */
    /* WebXR: both eyes into the session's framebuffer; the browser presents it */
    if (prts_engine_xr_render(e, alpha)) return;
    int w = 0, h = 0;
    prts_platform_drawable_size(e->platform, &w, &h);
    prts_camera cam = {prts_dv3(0, 1.7, 0), 0.0f, 0.0f};
    if (e->has_commander) {
        cam.eye = prts_commander_eye(&e->commander, e->world, alpha);
        cam.yaw = e->commander.yaw;
        float kick = 0.0f; /* the view's feedback motion: dips, bob and shake (never in XR) */
        cam.eye.y += prts_commander_view_offset(&e->commander, alpha, &kick);
        cam.pitch = e->commander.pitch + kick;
    }
    prts_engine_camera_override(e, &cam); /* developer observer camera */
    prts_gfx_render(e->gfx, e->scene, e->world, &cam, &e->render, w, h, alpha, e->time);
    prts_engine_ui_draw_overlay(e, w, h);
    if (e->want_shot) {
        char path[512];
        const char *dir = e->cfg.out_dir ? e->cfg.out_dir : e->user_dir;
        snprintf(path, sizeof(path), "%s/%s.png", dir, e->shot_name);
        if (prts_gfx_screenshot(e->gfx, path, w, h)) prts_log_info("screenshot saved: %s", path);
        else prts_log_error("screenshot failed: %s", path);
        e->want_shot = false;
    }
    prts_platform_swap(e->platform);
}

bool prts_engine_frame(prts_engine *e, double real_seconds)
{
    prts_input_scheme hint;
    if (prts_platform_take_scheme_hint(&hint)) e->dev.last_device = (int)hint; /* web: touch-first device */
    e->frames++;
    prts_jobs_apply_completed(e->jobs); /* the one point per frame where job results land */
#if defined(PRTS_DEV)
    e->reload_timer += real_seconds;
    if (e->hot_reload && e->reload_timer >= 1.0) prts_script_check_reload(e->script);
    if (e->reload_timer >= 1.0) e->reload_timer = 0.0;
#endif
    prts_engine_ui_before_input(e, real_seconds);
    prts_engine_xr_before_input(e);
    prts_input_update(e->input, &e->dev);
    if (!prts_engine_xr_look(e, real_seconds)) apply_look(e);
    prts_platform_begin_frame(&e->dev);
    /* scenarios run one tick per frame, so they are deterministic even with a window */
    int n = e->scenario_active || e->cfg.scenario ? 1 : prts_clock_advance(&e->clock, real_seconds);
    for (int i = 0; i < n && !e->quit; i++) prts_engine_tick(e);
    e->time = (double)e->clock.tick * e->clock.tick_seconds;
    prts_engine_ui_frame(e, real_seconds);
    if (e->cfg.ticks > 0 && e->clock.tick >= (uint64_t)e->cfg.ticks) e->quit = true;
    double alpha = e->scenario_active ? 1.0 : prts_clock_alpha(&e->clock);
    render_frame(e, alpha);
    prts_engine_audio_frame(e, real_seconds, alpha);
    return !e->quit;
}

void prts_engine_event(prts_engine *e, const void *sdl_event)
{
    if (prts_platform_apply_event(e->platform, sdl_event, &e->dev)) e->quit = true;
#if !defined(__EMSCRIPTEN__) /* a web page has nothing to quit to; there Escape releases the mouse */
    /* Escape quits only when the scripts define no menu action to handle it */
    if (e->dev.keys[41 /* Escape */] && prts_input_find(e->input, "menu") < 0) e->quit = true;
#endif
}

bool prts_engine_is_batch(const prts_engine *e) { return e->cfg.headless || e->cfg.test_lua; }

bool prts_engine_run_step(prts_engine *e)
{
    if (e->quit) return false;
    prts_platform_begin_frame(&e->dev);
    if (e->platform && prts_platform_poll(e->platform, &e->dev)) e->quit = true;
    if (!prts_engine_frame(e, e->clock.tick_seconds)) return false;
    if (!e->cfg.scenario && !e->cfg.replay && e->cfg.ticks <= 0 && e->cfg.headless) e->quit = true;
    return !e->quit;
}

int prts_engine_run(prts_engine *e)
{
    if (e->cfg.test_lua) return prts_engine_run_lua_tests(e);
    while (prts_engine_run_step(e)) {}
    return e->exit_code;
}

int prts_engine_exit_code(const prts_engine *e) { return e->exit_code; }

void prts_engine_get_status(const prts_engine *e, prts_engine_status *out)
{
    memset(out, 0, sizeof(*out));
    out->tick = e->clock.tick;
    out->frames = e->frames;
    out->scheme = (int)prts_input_scheme_active(e->input);
    out->scenario = e->scenario_state;
    out->scenario_ticks = e->scenario_ticks;
    out->scenario_cell = e->scenario_cell;
    out->scenario_message = e->scenario_message ? e->scenario_message : "";
    out->scenario_hash = e->scenario_hash;
    out->has_commander = e->has_commander;
    prts_render_settings xr = prts_engine_xr_render_settings(e);
    out->xr_teleport = e->xr.teleport;
    out->xr_vignette = e->xr.vignette;
    out->xr_snap = xr.snap;
    out->xr_vignette_level = e->xr.vignette_level;
    out->xr_turn = e->xr.turn;
    out->xr_seat = e->xr.seat;
    if (!e->has_commander) return;
    prts_dvec3 p = prts_world_pos(e->world, e->commander.entity);
    out->commander[0] = p.x;
    out->commander[1] = p.y;
    out->commander[2] = p.z;
    out->yaw = e->commander.yaw;
    out->pitch = e->commander.pitch;
}

prts_vfs *prts_engine_vfs(prts_engine *e) { return e->vfs; }
prts_audio *prts_engine_audio(prts_engine *e) { return e->audio; }
prts_ui *prts_engine_ui(prts_engine *e) { return e->ui; }

uint64_t prts_engine_hash(const prts_engine *e)
{
    prts_hasher h;
    prts_hasher_init(&h);
    prts_hasher_u64(&h, prts_world_hash(e->world));
    prts_hasher_u64(&h, e->clock.tick);
    prts_rng *r = prts_script_rng(e->script);
    prts_hasher_u64(&h, r->state);
    if (e->has_commander) {
        const prts_commander *c = &e->commander;
        prts_hasher_f64(&h, c->yaw);
        prts_hasher_f64(&h, c->pitch);
        prts_hasher_f64(&h, c->vertical_speed);
        prts_hasher_f64(&h, c->eye);
        prts_hasher_f64(&h, c->stamina);
        prts_hasher_f64(&h, c->vel.x);
        prts_hasher_f64(&h, c->vel.z);
        prts_hasher_u64(&h, (uint64_t)c->on_ground | ((uint64_t)c->crouched << 1) | ((uint64_t)c->aiming << 2) |
                                ((uint64_t)c->jump_buffer << 8) | ((uint64_t)c->coyote << 16) |
                                ((uint64_t)c->jumping << 24));
    }
    prts_engine_hash_script_data(e, &h);
    return h.h;
}
