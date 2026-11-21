/* Unit and integration tests: command line, engine boot, scenarios, rendering. MIT License. */
#include <SDL3/SDL.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"
#include "prts/prts_xr.h"
#include "utest.h"

#define FIX PRTS_TEST_FIXTURES
#define TMP PRTS_TEST_TMP

static int parse(prts_engine_config *cfg, char *err, int argc, char **argv)
{
    prts_engine_config_default(cfg);
    return prts_args_parse(argc, argv, cfg, err, 2048);
}

UTEST(args, options)
{
    prts_engine_config cfg;
    char err[2048];
    char *ok[] = {"1stprts", "--data", "d", "--user", "u", "--mod", "m1", "--mod", "m2", "--headless", "--hidden",
                  "--size", "800x600", "--seed", "42", "--ticks", "90", "--test-lua", "--scenario", "s.lua",
                  "--out", "o", "--lua-coverage", "c.info"};
    ASSERT_EQ(0, parse(&cfg, err, (int)PRTS_ARRAY_COUNT(ok), ok));
    ASSERT_STREQ("d", cfg.data_dir);
    ASSERT_STREQ("u", cfg.user_dir);
    ASSERT_EQ(2, cfg.mod_count);
    ASSERT_STREQ("m2", cfg.mods[1]);
    ASSERT_TRUE(cfg.headless && cfg.hidden && cfg.test_lua);
    ASSERT_EQ(800, cfg.width);
    ASSERT_EQ(600, cfg.height);
    ASSERT_EQ((uint64_t)42, cfg.seed);
    ASSERT_EQ(90, cfg.ticks);
    ASSERT_STREQ("s.lua", cfg.scenario);
    ASSERT_STREQ("o", cfg.out_dir);
    ASSERT_STREQ("c.info", cfg.lua_coverage);
    char *help[] = {"1stprts", "--help"};
    ASSERT_EQ(2, parse(&cfg, err, 2, help));
    ASSERT_TRUE(strstr(err, "--scenario") != NULL);
    char *version[] = {"1stprts", "--version"};
    ASSERT_EQ(2, parse(&cfg, err, 2, version));
    ASSERT_TRUE(strstr(err, PRTS_ENGINE_VERSION) != NULL);
    ASSERT_TRUE(strstr(prts_args_usage(), "--headless") != NULL);
}

UTEST(args, errors)
{
    prts_engine_config cfg;
    char err[2048];
    char *missing[] = {"1stprts", "--data"};
    ASSERT_EQ(1, parse(&cfg, err, 2, missing));
    ASSERT_TRUE(strstr(err, "needs a value") != NULL);
    char *bad_seed[] = {"1stprts", "--seed", "-3"};
    ASSERT_EQ(1, parse(&cfg, err, 3, bad_seed));
    char *bad_ticks[] = {"1stprts", "--ticks", "10x"};
    ASSERT_EQ(1, parse(&cfg, err, 3, bad_ticks));
    char *empty_ticks[] = {"1stprts", "--ticks", ""};
    ASSERT_EQ(1, parse(&cfg, err, 3, empty_ticks));
    char *bad_size[] = {"1stprts", "--size", "big"};
    ASSERT_EQ(1, parse(&cfg, err, 3, bad_size));
    char *small_size[] = {"1stprts", "--size", "10x10"};
    ASSERT_EQ(1, parse(&cfg, err, 3, small_size));
    char *unknown[] = {"1stprts", "--fly"};
    ASSERT_EQ(1, parse(&cfg, err, 2, unknown));
    ASSERT_TRUE(strstr(err, "unknown option") != NULL);
    char *many[2 + 2 * (PRTS_MAX_MODS + 1)];
    many[0] = "1stprts";
    int n = 1;
    for (int i = 0; i <= PRTS_MAX_MODS; i++) {
        many[n++] = "--mod";
        many[n++] = "x";
    }
    ASSERT_EQ(1, parse(&cfg, err, n, many));
    ASSERT_TRUE(strstr(err, "too many") != NULL);
}

static prts_engine *boot(const char *data, const char *user, const char *scenario, bool headless, char *err)
{
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = data;
    cfg.user_dir = user ? user : TMP "/user";
    cfg.scenario = scenario;
    cfg.headless = headless;
    cfg.hidden = true;
    cfg.width = 320;
    cfg.height = 180;
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine *e = prts_engine_create(&cfg, err, 1024);
    prts_log_set_min_level(PRTS_LOG_INFO);
    return e;
}

UTEST(engine, boot_errors)
{
    char err[1024];
    /* Spec: Data folder missing */
    ASSERT_TRUE(boot(FIX "/nowhere", NULL, NULL, true, err) == NULL);
    ASSERT_TRUE(strstr(err, "data folder is missing") != NULL);
    ASSERT_TRUE(strstr(err, FIX "/nowhere") != NULL);
    ASSERT_TRUE(boot(FIX "/data_nobase", NULL, NULL, true, err) == NULL);
    ASSERT_TRUE(strstr(err, "no base/") != NULL);
    ASSERT_TRUE(boot(FIX "/data_nomanifest", NULL, NULL, true, err) == NULL);
    ASSERT_TRUE(strstr(err, "mod.lua") != NULL);
    ASSERT_TRUE(boot(FIX "/data_broken", NULL, NULL, true, err) == NULL);
    ASSERT_TRUE(strstr(err, "boot failed on purpose") != NULL);
    /* scenarios that cannot start */
    const char *bad[] = {FIX "/scenarios/missing.lua", FIX "/scenarios/syntax.lua", FIX "/scenarios/not_table.lua",
                         FIX "/scenarios/no_run.lua", FIX "/scenarios/load_error.lua"};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(bad); i++) {
        ASSERT_TRUE(boot(PRTS_TEST_DATA, NULL, bad[i], true, err) == NULL);
        ASSERT_TRUE(strstr(err, "could not be started") != NULL);
    }
}

UTEST(engine, mods_are_checked_and_mounted)
{
    /* Spec: Incompatible mod; a mod's boot script replaces the base one */
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = PRTS_TEST_DATA;
    cfg.user_dir = FIX "/user";
    cfg.headless = true;
    const char *mods[] = {"bad_api", "bad_manifest", "no_manifest", "not_installed", "broken_look"};
    for (int i = 0; i < 5; i++) cfg.mods[cfg.mod_count++] = mods[i];
    char err[1024];
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_log_reset_error_count();
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    prts_log_set_min_level(PRTS_LOG_INFO);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ((size_t)2, prts_vfs_mount_count(e->vfs));
    ASSERT_STREQ("broken_look", prts_vfs_mount_id(e->vfs, 1));
    ASSERT_TRUE(prts_log_error_count() >= 4);
    ASSERT_TRUE(prts_scene_find_material(e->scene, "missing") >= 0);
    prts_engine_destroy(e);
}

static const char *lua_eval(prts_engine *e, const char *expr)
{
    static char out[256];
    char code[512];
    snprintf(code, sizeof(code), "__r = tostring(%s)", expr);
    if (!prts_script_run_string(e->script, code, "=test")) return "<error>";
    lua_State *L = prts_script_lua(e->script);
    lua_getglobal(L, "__r");
    snprintf(out, sizeof(out), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return out;
}

UTEST(engine, game_api_from_scripts)
{
    char err[1024];
    prts_engine *e = boot(PRTS_TEST_DATA, NULL, NULL, true, err);
    ASSERT_TRUE(e != NULL);
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    ASSERT_STREQ("keep.door", lua_eval(e, "prts.entity.name(prts.entity.find('keep.door'))"));
    ASSERT_STREQ("nil", lua_eval(e, "(prts.entity.find('nobody'))"));
    ASSERT_STREQ("0.0", lua_eval(e, "prts.entity.yaw(prts.entity.find('keep.door'))"));
    ASSERT_STREQ("-1.1000000238419", lua_eval(e, "(prts.entity.pos(prts.entity.find('keep.door')))"));
    ASSERT_STREQ("true", lua_eval(e, "(function() prts.entity.set_solid(prts.entity.find('keep.door'), false) return true end)()"));
    ASSERT_STREQ("12.0", lua_eval(e, "(function() local e = prts.entity.spawn{ name = 'p', pos = { x = 12, y = 1, z = 2 } } return (prts.entity.pos(e)) end)()"));
    ASSERT_STREQ("true", lua_eval(e, "prts.commander.entity() ~= nil"));
    ASSERT_STREQ("0.0", lua_eval(e, "(prts.commander.view())"));
    ASSERT_STREQ("4", lua_eval(e, "select('#', prts.commander.view())"));
    ASSERT_STREQ("true", lua_eval(e, "prts.commander.spawn{ pos = { 0, 0, 40 }, yaw = 1 } ~= nil"));
    ASSERT_STREQ("<error>", lua_eval(e, "prts.commander.spawn{ marker = 'nowhere' }"));
    ASSERT_STREQ("nil", lua_eval(e, "(prts.scene.marker_pos('nowhere'))"));
    ASSERT_STREQ("<error>", lua_eval(e, "prts.scene.box{ min = {0,0,0}, max = {1,1,1} }"));
    ASSERT_STREQ("<error>", lua_eval(e, "prts.scene.box{ min = {0,0,0}, max = {1,1,1}, material = 'nope' }"));
    ASSERT_STREQ("<error>", lua_eval(e, "prts.scene.box{ min = 5, max = {1,1,1}, material = 'stone' }"));
    ASSERT_STREQ("<error>", lua_eval(e, "prts.scene.material{ texture = 'x' }"));
    ASSERT_STREQ("<error>", lua_eval(e, "prts.scene.terrain{ size = 1, spacing = 4, material = 'grass' }"));
    ASSERT_STREQ("<error>", lua_eval(e, "prts.entity.spawn{ parent = 123456789 }"));
    ASSERT_STREQ("<error>", lua_eval(e, "prts.input.action{ id = 'x', mouse_buttons = { { code = 99 } } }"));
    ASSERT_STREQ("true", lua_eval(e, "(function() prts.input.action{ id = 'x', mouse_buttons = { { code = 1, scale = 1 } } } return true end)()"));
    ASSERT_STREQ("<error>", lua_eval(e, "prts.scene.illusion{ center = {0,0,0} }"));
    ASSERT_STREQ("true", lua_eval(e, "(function() prts.scene.prism{ base = {0,0,90}, solid = true, material = 'rock' } return true end)()"));
    ASSERT_STREQ("true", lua_eval(e, "(function() prts.scene.light{ pos = {0,1,0} } return true end)()"));
    /* Spec: Missing model (the placeholder is drawn and the call reports false) */
    ASSERT_STREQ("false", lua_eval(e, "(prts.scene.model{ path = 'models/missing.glb', pos = {0,0,80} })"));
    ASSERT_STREQ("<error>", lua_eval(e, "prts.scene.model{ pos = {0,0,0} }"));
    ASSERT_STREQ("0.84", lua_eval(e, "(function() local ok, b = prts.scene.model{ path = 'models/crate.vox', pos = {0,0,80}, "
                                    "scale = 0.07, solid = true } return string.format('%.2f', b.max[2] - b.min[2]) end)()"));
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_engine_destroy(e);
}

UTEST(engine, headless_ticks_and_determinism)
{
    /* Spec: Two identical runs (state hashes match), Record and replay (same inputs, same state) */
    char err[1024];
    uint64_t hashes[2];
    for (int run = 0; run < 2; run++) {
        prts_engine_config cfg;
        prts_engine_config_default(&cfg);
        cfg.data_dir = PRTS_TEST_DATA;
        cfg.user_dir = TMP "/user";
        cfg.headless = true;
        cfg.ticks = 90;
        cfg.seed = 99;
        prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
        ASSERT_TRUE(e != NULL);
        prts_input_override(e->input, "move_forward", 1.0f, 45);
        prts_input_override(e->input, "move_left", 1.0f, 20);
        ASSERT_TRUE(prts_script_run_string(e->script, "math.random(100)", "=rng"));
        ASSERT_EQ(0, prts_engine_run(e));
        ASSERT_EQ((uint64_t)90, e->clock.tick);
        ASSERT_FALSE(prts_engine_is_batch(e) == false);
        hashes[run] = prts_engine_hash(e);
        ASSERT_EQ(0, prts_engine_exit_code(e));
        prts_engine_destroy(e);
    }
    ASSERT_EQ(hashes[0], hashes[1]);
}

UTEST(engine, scenario_outcomes)
{
    char err[1024];
    const char *cases[][2] = {{FIX "/scenarios/helpers.lua", "0"},
                              {FIX "/scenarios/fails.lua", "1"},
                              {FIX "/scenarios/timeout.lua", "1"},
                              {FIX "/scenarios/wrong_cell.lua", "1"}};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(cases); i++) {
        prts_engine *e = boot(PRTS_TEST_DATA, NULL, cases[i][0], true, err);
        ASSERT_TRUE(e != NULL);
        prts_log_set_min_level(PRTS_LOG_ERROR + 1);
        int code = prts_engine_run(e);
        prts_log_set_min_level(PRTS_LOG_INFO);
        ASSERT_EQ(atoi(cases[i][1]), code);
        prts_engine_destroy(e);
    }
}

UTEST(engine, scenario_result_with_final_cell_and_hash)
{
    /* Spec: Same scenario result on desktop and web. Every run reports passed or failed, the
     * commander's final cell and the state hash; the web launcher runs it a step at a time. */
    char err[1024];
    prts_engine *e = boot(PRTS_TEST_DATA, NULL, FIX "/scenarios/helpers.lua", true, err);
    ASSERT_TRUE(e != NULL);
    prts_engine_status s;
    prts_engine_get_status(e, &s);
    ASSERT_EQ((int)PRTS_SCENARIO_RUNNING, s.scenario);
    ASSERT_STREQ("", s.scenario_cell);
    ASSERT_EQ((uint64_t)0, s.scenario_hash);
    int steps = 0;
    while (prts_engine_run_step(e)) steps++;
    ASSERT_FALSE(prts_engine_run_step(e)); /* stays over */
    ASSERT_GT(steps, 1);
    prts_engine_get_status(e, &s);
    ASSERT_EQ((int)PRTS_SCENARIO_PASSED, s.scenario);
    ASSERT_STREQ("outside", s.scenario_cell);
    ASSERT_STREQ("", s.scenario_message);
    ASSERT_EQ(prts_engine_hash(e), s.scenario_hash);
    ASSERT_GT(s.scenario_ticks, (uint64_t)1);
    ASSERT_TRUE(prts_engine_audio(e) == e->audio && prts_engine_ui(e) == e->ui);
    prts_engine_destroy(e);

    /* Spec: Failing scenario. A scenario naming its final cell fails with the expected and
     * actual cell when the commander ends elsewhere. */
    e = boot(PRTS_TEST_DATA, NULL, FIX "/scenarios/wrong_cell.lua", true, err);
    ASSERT_TRUE(e != NULL);
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    ASSERT_EQ(1, prts_engine_run(e));
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_engine_get_status(e, &s);
    ASSERT_EQ((int)PRTS_SCENARIO_FAILED, s.scenario);
    ASSERT_STREQ("outside", s.scenario_cell);
    ASSERT_STREQ("expected the commander to end in cell 'keep.hall', but they are in 'outside'", s.scenario_message);
    prts_engine_destroy(e);

    /* not a scenario run: no result */
    e = boot(PRTS_TEST_DATA, NULL, NULL, true, err);
    ASSERT_TRUE(e != NULL);
    prts_engine_get_status(e, &s);
    ASSERT_EQ((int)PRTS_SCENARIO_NONE, s.scenario);
    prts_engine_destroy(e);
}

UTEST(engine, lua_test_runner_reports_failures)
{
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = FIX "/data_tests";
    cfg.user_dir = TMP "/user";
    cfg.headless = true;
    cfg.test_lua = true;
    cfg.lua_coverage = TMP "/cov/fixture.info";
    char err[1024];
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(prts_engine_is_batch(e));
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    ASSERT_EQ(1, prts_engine_run(e));
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_engine_destroy(e);
    ASSERT_TRUE(prts_fs_is_file(TMP "/cov/fixture.info"));
}

/* --- windowed tests: need a display (CI runs them under Xvfb with Mesa llvmpipe) --------- */

static bool have_display(void) { return getenv("DISPLAY") != NULL || getenv("WAYLAND_DISPLAY") != NULL; }

UTEST(platform, impossible_gl_version_is_reported)
{
    /* Spec: OpenGL context unavailable */
    if (!have_display()) UTEST_SKIP("no display");
    prts_platform_config pc = {"test", 64, 64, true, 9, 9};
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_log_reset_error_count();
    ASSERT_TRUE(prts_platform_create(&pc) == NULL);
    prts_log_set_min_level(PRTS_LOG_INFO);
    ASSERT_EQ((size_t)1, prts_log_error_count());
}

static void push_event(prts_engine *e, SDL_Event *ev) { prts_engine_event(e, ev); }

UTEST(platform, events_become_device_state)
{
    if (!have_display()) UTEST_SKIP("no display");
    prts_platform_config pc = {"test", 64, 64, true, 3, 3};
    prts_platform *p = prts_platform_create(&pc);
    ASSERT_TRUE(p != NULL);
    prts_input_state s;
    memset(&s, 0, sizeof(s));
    SDL_Event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = SDL_EVENT_KEY_DOWN;
    ev.key.scancode = SDL_SCANCODE_W;
    ASSERT_FALSE(prts_platform_apply_event(p, &ev, &s));
    ASSERT_TRUE(s.keys[SDL_SCANCODE_W]);
    ev.type = SDL_EVENT_MOUSE_MOTION;
    ev.motion.xrel = 3;
    ev.motion.yrel = -2;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_EQ(3.0f, s.mouse_dx);
    ev.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    ev.button.button = 1;
    ev.button.down = true;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_TRUE(s.mouse[1]);
    ev.type = SDL_EVENT_MOUSE_WHEEL;
    ev.wheel.y = 1;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_EQ(1.0f, s.wheel);
    ev.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
    ev.gaxis.axis = 0;
    ev.gaxis.value = 20000;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_EQ(PRTS_SCHEME_GAMEPAD, s.last_device);
    ev.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    ev.gbutton.button = 0;
    ev.gbutton.down = true;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_TRUE(s.pad_buttons[0]);
    ev.type = SDL_EVENT_GAMEPAD_ADDED;
    prts_platform_apply_event(p, &ev, &s);
    ev.type = SDL_EVENT_FINGER_DOWN;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_EQ(PRTS_SCHEME_TOUCH, s.last_device);
    ASSERT_EQ(1, s.fingers);
    ev.type = SDL_EVENT_FINGER_UP;
    prts_platform_apply_event(p, &ev, &s);
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_EQ(0, s.fingers);
    ev.type = SDL_EVENT_WINDOW_SHOWN;
    ASSERT_FALSE(prts_platform_apply_event(p, &ev, &s));
    ev.type = SDL_EVENT_QUIT;
    ASSERT_TRUE(prts_platform_apply_event(p, &ev, &s));
    prts_platform_begin_frame(&s);
    ASSERT_EQ(0.0f, s.mouse_dx);
    ASSERT_FALSE(prts_platform_poll(p, &s));
    ASSERT_TRUE(prts_platform_key_from_name("Left Shift") > 0);
    ASSERT_EQ(0, prts_platform_key_from_name("No Such Key"));
    ASSERT_TRUE(prts_platform_ticks_ns() > 0);
    prts_platform_set_relative_mouse(p, false);
    prts_platform_destroy(p);
    prts_platform_destroy(NULL);
}

static void *no_gl(const char *name)
{
    return strcmp(name, "glDrawArrays") == 0 ? NULL : prts_platform_gl_getproc(name);
}

UTEST(gfx, windowed_engine_renders_and_recovers_from_broken_content)
{
    /* Spec: Broken shader, Missing model/texture placeholder, Changing the preset */
    if (!have_display()) UTEST_SKIP("no display");
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = PRTS_TEST_DATA;
    cfg.user_dir = FIX "/user";
    cfg.hidden = true;
    cfg.width = 320;
    cfg.height = 180;
    cfg.ticks = 4;
    cfg.mods[cfg.mod_count++] = "broken_look";
    char err[1024];
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    ASSERT_TRUE(e != NULL);
    ASSERT_FALSE(prts_gfx_is_null(e->gfx));
    ASSERT_TRUE(strstr(prts_gfx_info(e->gfx), "/") != NULL);
    ASSERT_TRUE(prts_gfx_shader_errors(e->gfx) >= 2);
    ASSERT_TRUE(prts_engine_frame(e, 1.0 / 30.0));
    ASSERT_TRUE(prts_gfx_mesh_count(e->gfx) > 10);
    /* Spec: Live war table (the map's war table surface is drawn every frame) */
    ASSERT_EQ(1, prts_gfx_surfaces_drawn(e->gfx));
    ASSERT_TRUE(prts_gfx_last_visible_cells(e->gfx) & PRTS_CELL_BIT(0));
    ASSERT_TRUE(prts_gfx_texture_count(e->gfx) > 5);
    e->render.preset = PRTS_RES_NATIVE;
    e->render.dither = e->render.snap = e->render.fog = false;
    e->render.affine = true;
    ASSERT_TRUE(prts_engine_frame(e, 1.0 / 30.0));
    e->cfg.out_dir = "/proc/1stprts-not-writable";
    snprintf(e->shot_name, sizeof(e->shot_name), "%s", "x");
    e->want_shot = true;
    ASSERT_TRUE(prts_engine_frame(e, 1.0 / 30.0));
    ASSERT_TRUE(prts_gfx_screenshot(e->gfx, TMP "/shot.png", 320, 180));
    /* Spec: Two views (stereo): both eyes drawn side by side in one frame */
    prts_camera cam = {prts_dv3(0, 1.7, 40), 0.0f, 0.0f};
    prts_view eyes[2];
    prts_view_from_camera(&eyes[0], &cam, 1.6f, 160, 180);
    eyes[1] = eyes[0];
    eyes[0].tan_right *= 0.8f; /* asymmetric, like a real headset */
    eyes[1].tan_left *= 0.8f;
    eyes[1].eye.x += 0.064;
    eyes[1].x = 160;
    prts_gfx_render_views(e->gfx, e->scene, e->world, eyes, 2, &e->render, 1.0, 0.0);
    ASSERT_EQ(2, prts_gfx_last_view_count(e->gfx));
    /* Spec: Stereo views from the headset. A WebXR frame (here into the window) draws both
     * eyes at the commander's position instead of the flat view. */
    e->cfg.ticks = 0; /* keep running past the configured tick limit */
    e->quit = false;
    prts_xr *xr = prts_xr_get();
    prts_xr_begin(xr, 0, 320, 180);
    xr->view_count = 2;
    for (int i = 0; i < 2; i++) {
        prts_mat4 id = prts_mat4_identity(), proj = prts_mat4_frustum(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
        memcpy(xr->views[i].view, id.m, sizeof(id.m));
        xr->views[i].view[12] = i ? -0.032f : 0.032f;
        xr->views[i].view[13] = -1.6f;
        memcpy(xr->views[i].projection, proj.m, sizeof(proj.m));
        int vp[4] = {i * 160, 0, 160, 180};
        memcpy(xr->views[i].viewport, vp, sizeof(vp));
    }
    ASSERT_TRUE(prts_engine_frame(e, 1.0 / 30.0));
    ASSERT_EQ(2, prts_gfx_last_view_count(e->gfx));
    prts_xr_end(xr);
    ASSERT_TRUE(prts_engine_frame(e, 1.0 / 30.0));
    ASSERT_EQ(1, prts_gfx_last_view_count(e->gfx));
    prts_gfx_reload_shaders(e->gfx);
    SDL_Event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = SDL_EVENT_KEY_DOWN;
    ev.key.scancode = SDL_SCANCODE_ESCAPE;
    push_event(e, &ev);
    ASSERT_FALSE(prts_engine_frame(e, 1.0 / 30.0));
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_engine_destroy(e);

    /* a driver without a needed function is refused */
    prts_platform_config pc = {"test", 64, 64, true, 3, 3};
    prts_platform *p = prts_platform_create(&pc);
    ASSERT_TRUE(p != NULL);
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    ASSERT_TRUE(prts_gfx_create(no_gl, NULL, false) == NULL);
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_platform_destroy(p);
}

UTEST(gfx, settings_and_null_backend)
{
    prts_render_settings s;
    prts_render_settings_default(&s);
    ASSERT_EQ(PRTS_RES_RETRO, s.preset);
    prts_res_preset p;
    ASSERT_TRUE(prts_res_preset_parse("crisp", &p));
    ASSERT_EQ(PRTS_RES_CRISP, p);
    ASSERT_FALSE(prts_res_preset_parse("ultra", &p));
    ASSERT_EQ(540, prts_res_preset_lines(PRTS_RES_HD));
    ASSERT_EQ(0, prts_res_preset_lines(PRTS_RES_NATIVE));
    int w, h;
    prts_gfx_internal_size(PRTS_RES_RETRO, 1920, 1080, &w, &h);
    ASSERT_EQ(480, w);
    ASSERT_EQ(270, h);
    prts_gfx_internal_size(PRTS_RES_HD, 640, 360, &w, &h); /* never larger than the window */
    ASSERT_EQ(640, w);
    prts_gfx_internal_size(PRTS_RES_NATIVE, 0, 0, &w, &h);
    ASSERT_EQ(1, w);
    prts_gfx *g = prts_gfx_create(NULL, NULL, false);
    ASSERT_TRUE(prts_gfx_is_null(g));
    ASSERT_STREQ("null", prts_gfx_info(g));
    ASSERT_FALSE(prts_gfx_screenshot(g, TMP "/null.png", 4, 4));
    prts_gfx_reload_shaders(g);
    ASSERT_EQ((size_t)0, prts_gfx_shader_errors(g));
    prts_view many[8];
    memset(many, 0, sizeof(many));
    prts_gfx_render_views(g, NULL, NULL, many, 8, &s, 0.0, 0.0);
    ASSERT_EQ(4, prts_gfx_last_view_count(g)); /* at most PRTS_GFX_MAX_VIEWS */
    prts_gfx_destroy(g);
    prts_gfx_destroy(NULL);
}
