/* 1stPRTS engine: state shared by the engine's app files. MIT License. */
#ifndef PRTS_ENGINE_INTERNAL_H
#define PRTS_ENGINE_INTERNAL_H

#include <lua.h>

#include "prts/prts_commander.h"
#include "prts/prts_console.h"
#include "prts/prts_engine.h"
#include "prts/prts_gfx.h"
#include "prts/prts_input.h"
#include "prts/prts_platform.h"
#include "prts/prts_scene.h"
#include "prts/prts_script.h"
#include "prts/prts_touch.h"
#include "prts/prts_ui.h"
#include "prts/prts_vfs.h"
#include "prts/prts_world.h"

#define PRTS_TICK_HZ 30

struct prts_engine {
    prts_engine_config cfg;
    char *data_dir;
    char *user_dir;
    prts_vfs *vfs;
    prts_script *script;
    prts_world *world;
    prts_cworld *cworld;
    prts_scene *scene;
    prts_input *input;
    prts_commander commander;
    bool has_commander;
    prts_platform *platform;
    prts_gfx *gfx;
    prts_input_state dev;
    prts_clock clock;
    prts_render_settings render;
    bool quit;
    int exit_code;
    double time;
    /* scenario runner */
    lua_State *co;
    int co_ref;
    int wait_ticks;
    bool scenario_active;
    char shot_name[128];
    bool want_shot;
    uint64_t scenario_ticks;
    uint64_t scenario_max_ticks;
    prts_array move_done; /* prts_entity: movers that finished this tick */
    /* user interface, settings and developer tools (engine_ui.c, api_ui.c, api_app.c, api_settings.c) */
    prts_ui *ui;
    prts_gfx_ui *gfx_ui;
    prts_touch *touch;
    prts_console *console; /* developer builds only */
    bool developer;        /* developer features: console, observer camera */
    bool ui_modal;         /* a script menu has the pointer and blocks game input */
    bool mouse_relative;
    bool text_input;
    bool prev_console_key;
    prts_input_state prev_dev;         /* device state of the previous UI frame (key capture edges) */
    char ui_text[PRTS_INPUT_TEXT_MAX]; /* typed text waiting for the next UI frame */
    float ui_wheel, ui_mouse_dx, ui_mouse_dy;
    int screen_w, screen_h;
    bool test_pointer, test_down; /* scripted pointer for tests, in screen UI units */
    float test_x, test_y;
    struct {
        bool active;
        prts_dvec3 pos;
        float yaw, pitch;
    } observer;
    uint32_t scene_ready; /* scene revision after the last post-load step */
    bool scene_ever_ready;
};

void prts_engine_open_game_api(prts_engine *e);
void prts_engine_tick(prts_engine *e);
prts_engine *prts_engine_from_lua(lua_State *L);
prts_entity prts_engine_check_entity(lua_State *L, int idx);

/* engine_ui.c */
void prts_engine_ui_create(prts_engine *e);
void prts_engine_ui_destroy(prts_engine *e);
void prts_engine_ui_before_input(prts_engine *e, double dt);
void prts_engine_ui_frame(prts_engine *e, double dt);
void prts_engine_ui_draw_surfaces(prts_engine *e);
void prts_engine_ui_draw_overlay(prts_engine *e, int w, int h);
bool prts_engine_camera_override(const prts_engine *e, prts_camera *cam);
void prts_engine_ui_test_frame(prts_engine *e);
void prts_engine_console_exec(const char *line, void *user);
bool prts_engine_set_observer(prts_engine *e, bool on);
/* Post-load step after a map was built: collision, terrain and baked lighting (once per change). */
void prts_engine_finish_scene(prts_engine *e);
/* api_ui.c, api_app.c, api_settings.c */
void prts_engine_open_ui_api(prts_engine *e);
void prts_engine_open_app_api(prts_engine *e);
void prts_engine_open_settings_api(prts_engine *e);
void prts_engine_load_settings(prts_engine *e);
/* Binds an action from its default table at index t, then applies the player's overrides. */
void prts_engine_apply_bindings(lua_State *L, prts_engine *e, const char *id, int t);
/* testing_ui.c: UI helpers for scenario tests (t) and Lua tests (T, synchronous frames) */
void prts_engine_open_test_ui(prts_engine *e, int table, bool synchronous);

/* testing.c */
bool prts_engine_start_scenario(prts_engine *e, const char *real_path);
void prts_engine_scenario_step(prts_engine *e);
int prts_engine_run_lua_tests(prts_engine *e);

#endif
