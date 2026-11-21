/* Integration tests: settings and bindings that survive a restart, the developer console,
 * release-build behavior, the front-end boot, world surfaces and UI drawing. MIT License. */
#include <lauxlib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "app/engine_internal.h"
#include "gfx/gfx_internal.h"
#include "gfx/image.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"
#include "prts/prts_xr.h"
#include "utest.h"

#define FIX PRTS_TEST_FIXTURES
#define TMP PRTS_TEST_TMP

static bool display(void) { return getenv("DISPLAY") != NULL || getenv("WAYLAND_DISPLAY") != NULL; }

static prts_engine *start(prts_engine_config *cfg)
{
    char err[1024];
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine *e = prts_engine_create(cfg, err, sizeof(err));
    prts_log_set_min_level(PRTS_LOG_INFO);
    return e;
}

static void config(prts_engine_config *cfg, const char *user)
{
    prts_engine_config_default(cfg);
    cfg->data_dir = PRTS_TEST_DATA;
    cfg->user_dir = user;
    cfg->headless = true;
}

static const char *eval(prts_engine *e, const char *expr)
{
    static char out[512];
    char code[1024];
    snprintf(code, sizeof(code), "__r = tostring(%s)", expr);
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    bool ok = prts_script_run_string(e->script, code, "=test");
    prts_log_set_min_level(PRTS_LOG_INFO);
    if (!ok) return "<error>";
    lua_State *L = prts_script_lua(e->script);
    lua_getglobal(L, "__r");
    snprintf(out, sizeof(out), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return out;
}

static bool console_has(const prts_engine *e, const char *text)
{
    for (size_t i = 0; i < prts_console_line_count(e->console); i++) {
        if (strstr(prts_console_line(e->console, i), text)) return true;
    }
    return false;
}

static void quiet_frames(prts_engine *e, int n)
{
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    for (int i = 0; i < n; i++) prts_engine_frame(e, 1.0 / 30.0);
    prts_log_set_min_level(PRTS_LOG_INFO);
}

UTEST(boot, new_command_line_options)
{
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    char err[256];
    char *argv[] = {"1stprts", "--start", "proving_ground", "--front-end", "--no-dev"};
    ASSERT_EQ(0, prts_args_parse(5, argv, &cfg, err, sizeof(err)));
    ASSERT_STREQ("proving_ground", cfg.start_map);
    ASSERT_TRUE(cfg.front_end && cfg.no_dev);
    char *missing[] = {"1stprts", "--start"};
    ASSERT_EQ(1, prts_args_parse(2, missing, &cfg, err, sizeof(err)));
}

UTEST(settings, video_preset_survives_a_restart)
{
    /* Spec: Settings persist */
    const char *user = TMP "/settings_restart";
    prts_fs_remove_tree(user);
    prts_engine_config cfg;
    config(&cfg, user);
    prts_engine *e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ(PRTS_RES_RETRO, e->render.preset);
    ASSERT_STREQ("true", eval(e, "(function() require('game.settings').set_video('preset', 'authentic') return true end)()"));
    ASSERT_EQ(PRTS_RES_AUTHENTIC, e->render.preset); /* applied without a restart */
    prts_engine_destroy(e);
    ASSERT_TRUE(prts_fs_is_file(TMP "/settings_restart/config.lua"));
    e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ(PRTS_RES_AUTHENTIC, e->render.preset);
    ASSERT_STREQ("authentic", eval(e, "prts.render.settings().preset"));
    ASSERT_STREQ("180", eval(e, "prts.render.settings().lines"));
    prts_engine_destroy(e);
}

UTEST(settings, rebinding_use_survives_a_restart)
{
    /* Spec: Rebinding use (E -> F; after a restart F triggers use and E does not) */
    const char *user = TMP "/rebind_restart";
    prts_fs_remove_tree(user);
    prts_engine_config cfg;
    config(&cfg, user);
    prts_engine *e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    ASSERT_STREQ("true", eval(e, "prts.input.rebind('use', 'kbm', 'F') and prts.settings.save()"));
    prts_engine_destroy(e);
    e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    int f = prts_platform_key_from_name("F"), key_e = prts_platform_key_from_name("E");
    e->dev.keys[f] = true;
    prts_input_update(e->input, &e->dev);
    ASSERT_TRUE(prts_input_down(e->input, "use"));
    e->dev.keys[f] = false;
    e->dev.keys[key_e] = true;
    prts_input_update(e->input, &e->dev);
    ASSERT_FALSE(prts_input_down(e->input, "use"));
    ASSERT_STREQ("3", eval(e, "prts.input.bindings('use').gamepad[1].code + 1")); /* other schemes keep defaults */
    prts_engine_destroy(e);
}

UTEST(settings, broken_or_unwritable_config_falls_back_to_defaults)
{
    const char *user = TMP "/settings_broken";
    const char *files[] = {"return 5\n", "this is not Lua\n", "return { f = load }\n"};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(files); i++) {
        prts_fs_remove_tree(user);
        ASSERT_TRUE(prts_fs_write_file(TMP "/settings_broken/config.lua", files[i], strlen(files[i])));
        prts_engine_config cfg;
        config(&cfg, user);
        prts_engine *e = start(&cfg);
        ASSERT_TRUE(e != NULL);
        ASSERT_EQ(PRTS_RES_RETRO, e->render.preset);
        ASSERT_STREQ("nil", eval(e, "prts.settings.get('f')"));
        prts_engine_destroy(e);
    }
    prts_engine_config cfg;
    config(&cfg, TMP "/settings_unwritable");
    prts_engine *e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    prts_vfs_set_user_dir(e->vfs, NULL); /* nowhere to write */
    ASSERT_STREQ("nil", eval(e, "prts.settings.save()"));
    prts_engine_destroy(e);
}

UTEST(console, commands_run_in_the_game_environment)
{
    /* Spec: Running a command */
#if !defined(PRTS_DEV)
    UTEST_SKIP("release build: no developer console");
#endif
    prts_engine_config cfg;
    config(&cfg, TMP "/console");
    prts_engine *e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(e->developer);
    ASSERT_TRUE(e->console != NULL);
    prts_console_submit(e->console, "prts.log.info(prts.api_version)");
    ASSERT_TRUE(console_has(e, "> prts.log.info(prts.api_version)"));
    char version[8];
    snprintf(version, sizeof(version), "%d", PRTS_API_VERSION);
    ASSERT_STREQ(version, prts_console_line(e->console, prts_console_line_count(e->console) - 1));
    prts_console_submit(e->console, "1 + 1, 'two'");
    ASSERT_STREQ("2\ttwo", prts_console_line(e->console, prts_console_line_count(e->console) - 1));
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_console_submit(e->console, "x = = 1");
    prts_log_set_min_level(PRTS_LOG_INFO);
    ASSERT_TRUE(console_has(e, "[error]"));
    prts_console_submit(e->console, "help");
    ASSERT_TRUE(console_has(e, "commands:"));
    prts_console_submit(e->console, "clear");
    ASSERT_EQ((size_t)0, prts_console_line_count(e->console));
    prts_console_submit(e->console, "reload");
    ASSERT_TRUE(console_has(e, "nothing was reloaded"));
    ASSERT_STREQ("true", eval(e, "(function() prts.dev.reload = function() end return true end)()"));
    prts_console_submit(e->console, "reload");
    ASSERT_TRUE(console_has(e, "scripts reloaded"));
    /* the observer camera: a developer view that leaves the commander where he is */
    prts_console_submit(e->console, "observer");
    ASSERT_TRUE(e->observer.active);
    prts_camera cam = {prts_dv3(0, 0, 0), 0, 0};
    ASSERT_TRUE(prts_engine_camera_override(e, &cam));
    ASSERT_TRUE(cam.eye.y > 1.0);
    prts_dvec3 before = prts_world_pos(e->world, e->commander.entity);
    e->dev.keys[prts_platform_key_from_name("W")] = true;
    e->dev.keys[prts_platform_key_from_name("Left Shift")] = true;
    e->dev.mouse_dx = 40.0f;
    quiet_frames(e, 10);
    prts_camera moved = cam;
    prts_engine_camera_override(e, &moved);
    ASSERT_TRUE(fabs(moved.eye.z - cam.eye.z) + fabs(moved.eye.x - cam.eye.x) > 5.0);
    ASSERT_TRUE(moved.yaw != cam.yaw);
    prts_dvec3 after = prts_world_pos(e->world, e->commander.entity);
    ASSERT_NEAR(before.z, after.z, 0.01);
    /* with the console open the observer camera stays put */
    prts_console_set_open(e->console, true);
    prts_engine_camera_override(e, &cam);
    quiet_frames(e, 3);
    prts_engine_camera_override(e, &moved);
    ASSERT_EQ(cam.eye.z, moved.eye.z);
    e->dev.keys[prts_platform_key_from_name("W")] = false;
    prts_console_set_open(e->console, false);
    ASSERT_STREQ("false", eval(e, "prts.dev.observer(false)"));
    ASSERT_FALSE(prts_engine_camera_override(e, &cam));
    ASSERT_TRUE(prts_engine_set_observer(e, true)); /* without a commander the camera starts at the origin */
    prts_engine_set_observer(e, false);
    prts_engine_destroy(e);
}

UTEST(console, release_builds_have_no_way_out_of_the_commanders_eyes)
{
    /* Spec: Release build (a developer build with developer features turned off behaves the
     * same; real release builds run the dev_console scenario on Windows and macOS) */
    prts_engine_config cfg;
    config(&cfg, TMP "/release");
    cfg.no_dev = true;
    prts_engine *e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    ASSERT_FALSE(e->developer);
    ASSERT_TRUE(e->console == NULL);
    ASSERT_STREQ("nil", eval(e, "prts.dev"));
    ASSERT_STREQ("false", eval(e, "prts.boot.developer"));
    ASSERT_FALSE(prts_engine_set_observer(e, true));
    prts_camera cam = {prts_dv3(0, 0, 0), 0, 0};
    ASSERT_FALSE(prts_engine_camera_override(e, &cam));
    e->dev.keys[prts_platform_key_from_name("`")] = true;
    quiet_frames(e, 2);
    ASSERT_FALSE(e->observer.active);
    ASSERT_FALSE(prts_input_blocked(e->input));
    for (size_t i = 0; i < prts_ui_text_count(e->ui, PRTS_UI_SCREEN); i++) {
        ASSERT_TRUE(strstr(prts_ui_text_at(e->ui, PRTS_UI_SCREEN, i), "DEVELOPER") == NULL);
    }
    prts_engine_destroy(e);
}

UTEST(boot, front_end_menu_and_start_option)
{
    /* Spec: Script-driven boot (the front-end menu decides what starts) */
    prts_engine_config cfg;
    config(&cfg, TMP "/front_end");
    cfg.front_end = true;
    prts_engine *e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    ASSERT_FALSE(e->has_commander);
    ASSERT_STREQ("false", eval(e, "prts.boot.auto_start"));
    ASSERT_TRUE(e->ui_modal);
    quiet_frames(e, 2);
    ASSERT_TRUE(prts_input_blocked(e->input));
    prts_engine_destroy(e);
    config(&cfg, TMP "/front_end");
    cfg.start_map = "proving_ground";
    e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(e->has_commander);
    ASSERT_STREQ("proving_ground", eval(e, "prts.boot.map"));
    prts_engine_destroy(e);
}

UTEST(boot, broken_font_uses_fallback_metrics)
{
    prts_engine_config cfg;
    config(&cfg, FIX "/user");
    cfg.mods[cfg.mod_count++] = "broken_font";
    prts_engine *e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ(6, prts_ui_text_width(prts_ui_get_font(e->ui), "a", -1));
    prts_engine_destroy(e);
}

UTEST(surfaces, eye_ray_and_touch_drive_a_world_surface)
{
    prts_engine_config cfg;
    config(&cfg, TMP "/surfaces");
    prts_engine *e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    /* a 2 m board one meter in front of the commander's eyes, with one big button */
    const char *setup =
        "clicks = 0\n"
        "local x, y, z = prts.entity.pos(prts.commander.entity())\n"
        "local yaw = prts.commander.view()\n"
        "local fx, fz, rx, rz = -math.sin(yaw), -math.cos(yaw), math.cos(yaw), -math.sin(yaw)\n"
        "prts.ui.surface{ id = 'board', width = 64, height = 64,\n"
        "  origin = { x + fx - rx, y + 2.7, z + fz - rz }, right = { 2 * rx, 0, 2 * rz }, down = { 0, -2, 0 } }\n"
        "prts.events.on('ui', 'test.board', function(name, w, h)\n"
        "  if name ~= 'board' then return end\n"
        "  prts.ui.window('Board', 0, 0, w, h, { title = false }, function()\n"
        "    prts.ui.row({ -1 }, -1)\n"
        "    if prts.ui.button('Press') then clicks = clicks + 1 end\n"
        "  end)\n"
        "end)\n";
    ASSERT_TRUE(prts_script_run_string(e->script, setup, "=setup"));
    quiet_frames(e, 3);
    prts_input_override(e->input, "use", 1.0f, 1);
    quiet_frames(e, 3);
    ASSERT_STREQ("1", eval(e, "clicks"));
    /* touch: the ray goes through the finger */
    e->dev.last_device = PRTS_SCHEME_TOUCH;
    e->dev.touches[0] = (prts_touch_point){7, 0.5f, 0.5f, true};
    quiet_frames(e, 3);
    e->dev.touches[0].down = false;
    quiet_frames(e, 2);
    ASSERT_STREQ("2", eval(e, "clicks"));
    /* the screen gets the finger as its pointer */
    e->dev.touches[0].down = true;
    quiet_frames(e, 1);
    e->dev.touches[0].down = false;
    quiet_frames(e, 1);
    /* XR: the controller's pointer ray drives the board while the head looks away from it */
    e->dev.last_device = PRTS_SCHEME_KBM;
    quiet_frames(e, 3);
    int clicks = atoi(eval(e, "clicks"));
    prts_xr *xr = prts_xr_get();
    prts_xr_begin(xr, 0, 100, 100);
    xr->has_pose = true;
    xr->head.position[1] = 1.6f;
    xr->head.orientation[3] = 1.0f;
    prts_xr_controller *ctl = &xr->controllers[0];
    ctl->connected = ctl->has_aim = true;
    ctl->hand = PRTS_XR_HAND_RIGHT;
    ctl->aim.position[1] = 1.6f;
    ctl->aim.orientation[3] = 1.0f; /* straight ahead, at the board */
    quiet_frames(e, 2);
    xr->head.orientation[1] = 0.7071f; /* the head turns 90 degrees to the left */
    xr->head.orientation[3] = 0.7071f;
    quiet_frames(e, 2);
    prts_input_override(e->input, "use", 1.0f, 1);
    quiet_frames(e, 3);
    ASSERT_EQ(clicks + 1, atoi(eval(e, "clicks")));
    prts_xr_end(xr);
    quiet_frames(e, 1);
    prts_xr_reset(xr);
    /* menus and the console take the pointer away from world surfaces */
    e->ui_modal = true;
    quiet_frames(e, 1);
    e->ui_modal = false;
    ASSERT_EQ((uint32_t)0, prts_gfx_ui_surface_texture(e->gfx_ui, 1)); /* the null back end draws nothing */
    ASSERT_EQ((uint32_t)0, prts_gfx_ui_surface_texture(e->gfx_ui, 0));
    ASSERT_EQ((size_t)0, prts_gfx_ui_draw_count(e->gfx_ui));
    prts_engine_destroy(e);
}

static unsigned char pixel(const unsigned char *rgba, int w, int x, int y, int c) { return rgba[(y * w + x) * 4 + c]; }

UTEST(gfx, ui_overlay_and_surface_textures_are_drawn)
{
    if (!display()) UTEST_SKIP("no display");
    prts_engine_config cfg;
    config(&cfg, TMP "/gl_ui");
    cfg.headless = false;
    cfg.hidden = true;
    cfg.width = 640;
    cfg.height = 360;
    cfg.front_end = true;
    prts_engine *e = start(&cfg);
    ASSERT_TRUE(e != NULL);
    const char *setup =
        "prts.ui.surface{ id = 'pool', width = 32, height = 16, origin = { 0, 1, 0 }, right = { 1, 0, 0 }, down = { 0, -1, 0 } }\n"
        "prts.events.on('ui', 'test.draw', function(name, w, h)\n"
        "  if name == 'pool' then prts.ui.draw_rect(0, 0, 32, 8, { 0, 255, 0, 255 }) end\n"
        "  if name == 'screen' then prts.ui.draw_rect(0, 0, 20, 20, { 255, 0, 0, 255 }) end\n"
        "end)\n";
    ASSERT_TRUE(prts_script_run_string(e->script, setup, "=setup"));
    e->dev.mouse_x = 320.0f; /* the pointer over the menu */
    e->dev.mouse_y = 200.0f;
    quiet_frames(e, 3);
    ASSERT_TRUE(prts_gfx_ui_draw_count(e->gfx_ui) >= 3);
    ASSERT_TRUE(prts_ui_hovered(e->ui, PRTS_UI_SCREEN));
    /* the screen overlay: red in the top-left corner (scale 1 at 360 lines) */
    prts_fs_mkdirs(TMP "/gl_ui");
    e->cfg.out_dir = TMP "/gl_ui";
    snprintf(e->shot_name, sizeof(e->shot_name), "%s", "overlay");
    e->want_shot = true;
    quiet_frames(e, 1);
    char *png = NULL;
    size_t size = 0;
    ASSERT_TRUE(prts_fs_read_file(TMP "/gl_ui/overlay.png", &png, &size));
    int w = 0, h = 0;
    unsigned char *px = prts_image_decode(png, size, &w, &h);
    prts_free(png);
    ASSERT_TRUE(px != NULL);
    ASSERT_EQ(255, pixel(px, w, 5, 5, 0));
    ASSERT_EQ(0, pixel(px, w, 5, 5, 1));
    prts_image_free(px);
    /* the surface texture: green in the top half (row 0 of the texture is the top of the UI) */
    GLuint tex = prts_gfx_ui_surface_texture(e->gfx_ui, 1);
    ASSERT_TRUE(tex != 0);
    prts_gl *gl = &e->gfx->gl;
    GLuint fbo = 0;
    gl->glGenFramebuffers(1, &fbo);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    unsigned char texels[32 * 16 * 4];
    gl->glPixelStorei(GL_PACK_ALIGNMENT, 1);
    gl->glReadPixels(0, 0, 32, 16, GL_RGBA, GL_UNSIGNED_BYTE, texels);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl->glDeleteFramebuffers(1, &fbo);
    ASSERT_EQ(255, pixel(texels, 32, 4, 2, 1));  /* texture row 2: UI row 2 */
    ASSERT_EQ(0, pixel(texels, 32, 4, 12, 3));   /* lower half: transparent */
    /* resizing a surface recreates its texture; empty batches only clear */
    ASSERT_TRUE(prts_script_run_string(e->script, "prts.ui.surface{ id = 'pool', width = 16, height = 16 }\n"
                                                  "prts.events.off('ui', 'test.draw')", "=resize"));
    quiet_frames(e, 1);
    prts_gfx_ui_draw_surface(e->gfx_ui, 0, prts_ui_batch_of(e->ui, 1)); /* slot 0 is the screen: ignored */
    prts_engine_destroy(e);
}
