/* 1stPRTS engine: state shared by the engine's app files. MIT License. */
#ifndef PRTS_ENGINE_INTERNAL_H
#define PRTS_ENGINE_INTERNAL_H

#include <lua.h>

#include "prts/prts_audio.h"
#include "prts/prts_commander.h"
#include "prts/prts_engine.h"
#include "prts/prts_gfx.h"
#include "prts/prts_input.h"
#include "prts/prts_platform.h"
#include "prts/prts_scene.h"
#include "prts/prts_script.h"
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
    prts_audio *audio;
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

/* testing.c */
bool prts_engine_start_scenario(prts_engine *e, const char *real_path);
void prts_engine_scenario_step(prts_engine *e);
int prts_engine_run_lua_tests(prts_engine *e);

#endif
