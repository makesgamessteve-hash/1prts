/* 1stPRTS engine: state shared by the engine's app files. MIT License. */
#ifndef PRTS_ENGINE_INTERNAL_H
#define PRTS_ENGINE_INTERNAL_H

#include <lua.h>

#include "prts/prts_audio.h"
#include "prts/prts_commander.h"
#include "prts/prts_console.h"
#include "prts/prts_engine.h"
#include "prts/prts_gfx.h"
#include "prts/prts_input.h"
#include "prts/prts_jobs.h"
#include "prts/prts_platform.h"
#include "prts/prts_model.h"
#include "prts/prts_scene.h"
#include "prts/prts_script.h"
#include "prts/prts_touch.h"
#include "prts/prts_ui.h"
#include "prts/prts_vfs.h"
#include "prts/prts_world.h"

#define PRTS_TICK_HZ 30

/* Playing in XR (engine_xr.c): comfort options, the video settings used in XR and the state
 * of the play area. */
typedef struct prts_engine_xr {
    /* comfort options (prts.xr.settings) */
    bool teleport;          /* teleport locomotion instead of smooth movement */
    bool snap_turn;         /* turn in steps instead of smoothly */
    float snap_angle;       /* radians per snap turn */
    float turn_speed;       /* radians per second at full deflection (smooth turning) */
    bool vignette;          /* the comfort vignette while moving or turning smoothly */
    float vignette_strength;
    bool seated;            /* seated mode: the view is raised to standing eye height */
    /* video settings in XR; the others (fog) follow the flat screen's */
    prts_res_preset preset;
    bool dither, snap, affine, crosshair;
    /* the session */
    bool was_active;
    bool align_pending;     /* line the play area up on the next tracked frame */
    int flat_scheme;        /* input scheme before the session, restored after it */
    float turn;             /* yaw of the play area (the reference space) in the world */
    prts_vec3 room, room_prev; /* the play area's origin relative to the feet (world axes), this and last tick */
    float seat;             /* height added in seated mode */
    bool snap_ready;        /* the turn stick came back to the centre since the last snap */
    bool aim_locked;        /* a teleport is aimed: the pointer hand stays */
    int hand;               /* the pointer hand: 0 left, 1 right */
    float prev_trigger[2];
    float vignette_level;   /* current strength (eases in and out) */
    prts_vec3 pushback;     /* the last view's head push-back */
} prts_engine_xr;
/* Registry key of the per-entity script data table (prts.entity.data). */
#define PRTS_REG_EDATA "prts.edata"

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
    prts_audio *audio;
    prts_input_state dev;
    prts_engine_xr xr;
    prts_clock clock;
    prts_render_settings render;
    bool quit;
    int exit_code;
    double time;
    uint64_t frames; /* prts_engine_frame calls */
    /* scenario runner */
    lua_State *co;
    int co_ref;
    int wait_ticks;
    bool scenario_active;
    char shot_name[128];
    bool want_shot;
    uint64_t scenario_ticks;
    uint64_t scenario_max_ticks;
    char scenario_final_cell[64]; /* the cell the scenario expects the commander to end in ("": any) */
    /* the result once it has ended (spec: Same scenario result on desktop and web) */
    int scenario_state;     /* prts_scenario_state */
    char scenario_cell[64]; /* the commander's cell at the end */
    char *scenario_message; /* why it failed, or NULL */
    uint64_t scenario_hash; /* the state hash at the end */
    prts_array move_done; /* prts_entity: movers that finished this tick */
    char *mod_versions[PRTS_MAX_MODS + 1]; /* per VFS mount (base first), from mod.lua */
    char *title;                           /* the window title: `title` of the base game's mod.lua */
    /* infrastructure: jobs, test reports (report.c), saves and replays (replay.c), hot reload */
    prts_jobs *jobs;
    struct prts_report *report;
    struct prts_replay *replay;
    bool hot_reload;
    double reload_timer;
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

/* api_audio.c */
void prts_engine_open_audio_api(prts_engine *e);
/* Moves the listener to the commander's eye, moves sounds with their entities and feeds
 * the audio device. Called once per frame. */
void prts_engine_audio_frame(prts_engine *e, double real_seconds, double alpha);
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

/* engine_xr.c */
void prts_engine_xr_init(prts_engine *e);
/* Session start and end, and the controllers into the device state (before the input update). */
void prts_engine_xr_before_input(prts_engine *e);
/* In XR: turning, the head's view direction and the comfort vignette. Returns false outside
 * XR (then the usual look applies). */
bool prts_engine_xr_look(prts_engine *e, double dt);
/* The commander's XR input for the coming tick, and the play area after it. */
void prts_engine_xr_before_tick(prts_engine *e);
void prts_engine_xr_after_tick(prts_engine *e);
/* The eye views of the current XR frame (with the head push-back); 0 outside XR. */
int prts_engine_xr_views(prts_engine *e, double alpha, prts_view *out, int max);
/* World position of the tracked head as drawn (after the push-back); false outside XR. */
bool prts_engine_xr_head(prts_engine *e, double alpha, prts_dvec3 *out);
prts_render_settings prts_engine_xr_render_settings(const prts_engine *e);
/* Draws the XR frame with pointer rays; false if there is nothing to draw in XR. */
bool prts_engine_xr_render(prts_engine *e, double alpha);
void prts_engine_open_xr_api(prts_engine *e);
/* t.xr_begin, t.xr_frame, t.xr_end, t.xr_view on the test table at index `table`. */
void prts_engine_open_test_xr(prts_engine *e, int table);

/* testing.c */
bool prts_engine_start_scenario(prts_engine *e, const char *real_path);
void prts_engine_scenario_step(prts_engine *e);
int prts_engine_run_lua_tests(prts_engine *e);
/* Ends an unfinished test case and writes the --junit report. */
void prts_engine_finish_report(prts_engine *e);

#endif
