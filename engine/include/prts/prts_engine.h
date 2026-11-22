/* 1stPRTS engine: boot, main loop, tests and scenario runner. MIT License. */
#ifndef PRTS_ENGINE_H
#define PRTS_ENGINE_H

#include "prts_base.h"
#include "prts_vfs.h"

#define PRTS_MAX_MODS 16

typedef struct prts_engine_config {
    const char *data_dir;     /* NULL: look next to the executable */
    const char *user_dir;     /* NULL: per-user preference directory */
    const char *mods[PRTS_MAX_MODS];
    int mod_count;
    bool headless;            /* no window, GPU or audio */
    bool hidden;              /* window exists but is not shown (screenshots) */
    bool test_lua;            /* run Lua unit tests and exit */
    const char *scenario;     /* real path of a scenario test to run */
    const char *out_dir;      /* where scenario screenshots go */
    const char *lua_coverage; /* write Lua line coverage (lcov) here on exit */
    const char *start_map;    /* start this map at once instead of showing the front-end menu */
    bool front_end;           /* show the front-end menu even in test and batch runs */
    bool no_dev;              /* turn developer features off (tests of release behavior) */
    uint64_t seed;
    int ticks;                /* stop after this many ticks (0: run until quit) */
    int width, height;
    const char *junit;        /* write a JUnit XML report of Lua tests, scenarios or a replay check */
    const char *record;       /* record a replay of this run to this file */
    const char *replay;       /* play back this replay and check every tick's state hash */
    bool hot_reload;          /* developer builds: reload changed Lua modules (default for interactive runs) */
} prts_engine_config;

void prts_engine_config_default(prts_engine_config *cfg);
const char *prts_args_usage(void);
/* Parses the command line. Returns 0 to run, 1 on error (message in err), 2 if help or the
 * version was requested (text in err). */
int prts_args_parse(int argc, char **argv, prts_engine_config *cfg, char *err, size_t err_size);

typedef struct prts_engine prts_engine;

/* Boots the engine. On failure returns NULL and fills err. */
prts_engine *prts_engine_create(const prts_engine_config *cfg, char *err, size_t err_size);
void prts_engine_destroy(prts_engine *e);
/* One windowed frame: events, ticks, rendering. Returns false when the game should end. */
bool prts_engine_frame(prts_engine *e, double real_seconds);
void prts_engine_event(prts_engine *e, const void *sdl_event);
/* Runs headless (or a scenario/test run) to completion and returns the exit code. */
int prts_engine_run(prts_engine *e);
/* True if this configuration runs to completion without a window loop. */
bool prts_engine_is_batch(const prts_engine *e);
int prts_engine_exit_code(const prts_engine *e);
uint64_t prts_engine_hash(const prts_engine *e);

/* One run the batch way: a frame at the fixed tick length. Returns false once the run is
 * over (prts_engine_run calls it until then; the web launcher calls it from the browser's
 * frame loop, a slice at a time, so the page stays responsive). */
bool prts_engine_run_step(prts_engine *e);

typedef enum prts_scenario_state {
    PRTS_SCENARIO_NONE = 0, /* not a scenario run */
    PRTS_SCENARIO_RUNNING,
    PRTS_SCENARIO_PASSED,
    PRTS_SCENARIO_FAILED
} prts_scenario_state;

/* Read-only view of the running engine for tools and the browser test hooks (web/). */
typedef struct prts_engine_status {
    uint64_t tick;        /* simulation ticks run */
    uint64_t frames;      /* frames run (prts_engine_frame calls) */
    bool has_commander;
    double commander[3];  /* world position of the commander's feet */
    float yaw, pitch;     /* radians */
    int scheme;           /* active prts_input_scheme */
    /* scenario runs (--scenario): the result, filled in once the scenario has ended */
    int scenario;                 /* prts_scenario_state */
    uint64_t scenario_ticks;      /* ticks the scenario ran */
    const char *scenario_cell;    /* the commander's cell at the end ("" before) */
    const char *scenario_message; /* why it failed ("" otherwise) */
    uint64_t scenario_hash;       /* the state hash at the end (0 before) */
    /* XR comfort options and their state */
    bool xr_teleport;     /* teleport locomotion selected */
    bool xr_vignette;     /* comfort vignette on */
    bool xr_snap;         /* vertex snapping in XR */
    float xr_vignette_level, xr_turn, xr_seat;
    int xr_pointer;   /* the pointer hand: 0 left, 1 right */
    bool xr_carried;  /* carried by something moving (the riding vignette) */
} prts_engine_status;
/* The strings point into the engine and stay valid until it is destroyed. */
void prts_engine_get_status(const prts_engine *e, prts_engine_status *out);
/* The engine's virtual file system, including the user area. */
prts_vfs *prts_engine_vfs(prts_engine *e);
/* The engine's audio system and screen UI (read-only use by tools and the browser test hooks). */
struct prts_audio *prts_engine_audio(prts_engine *e);
struct prts_ui *prts_engine_ui(prts_engine *e);

#endif
