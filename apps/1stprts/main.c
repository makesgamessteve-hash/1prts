/* 1stPRTS launcher: SDL3 app callbacks around the engine. MIT License. */
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>

#include "prts/prts_engine.h"
#include "prts/prts_platform.h"

typedef struct app {
    prts_engine *engine;
    prts_engine_config cfg; /* kept to start the game again (prts.app.restart) */
    char next_map[64];
    uint64_t last_ns;
    int batch_steps; /* web batch runs (a scenario): run steps per browser frame, 0 otherwise */
} app;

#if defined(__EMSCRIPTEN__)
#include "web_hooks.h"

/* Web: while a WebXR session runs, SDL's loop is paused and web/prts_xr.js calls this from
 * XRSession.requestAnimationFrame instead (design D13). It does what SDL's loop does: hand the
 * queued events to SDL_AppEvent, then run one SDL_AppIterate. */
static app *g_web_app;
void prts_frame(void);
void prts_frame(void)
{
    if (!g_web_app) return;
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) SDL_AppEvent(g_web_app, &ev);
    SDL_AppIterate(g_web_app);
}
#endif

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    char msg[2048];
    int rc = prts_args_parse(argc, argv, &cfg, msg, sizeof(msg));
    if (rc == 2) {
        fputs(msg, stdout);
        return SDL_APP_SUCCESS;
    }
    if (rc != 0) {
        fprintf(stderr, "%s\n", msg);
        return SDL_APP_FAILURE;
    }
    prts_engine *e = prts_engine_create(&cfg, msg, sizeof(msg));
    if (!e) {
        fprintf(stderr, "1stprts: %s\n", msg);
        if (!cfg.headless && !cfg.hidden && !cfg.scenario && !cfg.test_lua) prts_platform_error_box("1stPRTS", msg);
        return SDL_APP_FAILURE;
    }
    bool batch = prts_engine_is_batch(e) || cfg.scenario;
#if defined(__EMSCRIPTEN__)
    /* Web: a scenario runs in the browser's frame loop a slice at a time instead of blocking
     * the page, and the engine stays up when it ends, so the test hooks can read the result.
     * Headless runs take many steps per frame; windowed ones draw every step. */
    int batch_steps = batch && !cfg.test_lua ? (cfg.headless ? 240 : 1) : 0;
    if (batch_steps) batch = false;
#endif
    if (batch) {
        int code = prts_engine_run(e);
        prts_engine_destroy(e);
        return code == 0 ? SDL_APP_SUCCESS : SDL_APP_FAILURE;
    }
    app *a = SDL_calloc(1, sizeof(*a));
    a->engine = e;
    a->cfg = cfg;
    a->last_ns = prts_platform_ticks_ns();
#if defined(__EMSCRIPTEN__)
    a->batch_steps = batch_steps;
#endif
    *appstate = a;
#if defined(__EMSCRIPTEN__)
    g_web_app = a;
    prts_web_hooks_attach(e);
#endif
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    app *a = appstate;
    if (a->batch_steps) {
        for (int i = 0; i < a->batch_steps && prts_engine_run_step(a->engine); i++) {}
        return SDL_APP_CONTINUE; /* a finished run stays readable (web test hooks) */
    }
    uint64_t now = prts_platform_ticks_ns();
    double dt = (double)(now - a->last_ns) / 1e9;
    a->last_ns = now;
    if (prts_engine_frame(a->engine, dt)) return SDL_APP_CONTINUE;
    const char *next = prts_engine_restart_map(a->engine);
    if (next) { /* prts.app.restart: the game starts again on that map (a campaign's next act) */
        snprintf(a->next_map, sizeof(a->next_map), "%s", next);
#if defined(__EMSCRIPTEN__)
        prts_web_hooks_attach(NULL);
#endif
        prts_engine_destroy(a->engine);
        a->cfg.start_map = a->next_map;
        a->cfg.front_end = false;
        char msg[2048];
        a->engine = prts_engine_create(&a->cfg, msg, sizeof(msg));
        if (!a->engine) {
            fprintf(stderr, "1stprts: %s\n", msg);
            return SDL_APP_FAILURE;
        }
#if defined(__EMSCRIPTEN__)
        prts_web_hooks_attach(a->engine);
#endif
        a->last_ns = prts_platform_ticks_ns();
        return SDL_APP_CONTINUE;
    }
    return prts_engine_exit_code(a->engine) == 0 ? SDL_APP_SUCCESS : SDL_APP_FAILURE; /* e.g. a replay diverged */
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    app *a = appstate;
    if (a) prts_engine_event(a->engine, event);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;
    app *a = appstate;
    if (!a || !a->engine) {
        SDL_free(a);
        return;
    }
#if defined(__EMSCRIPTEN__)
    g_web_app = NULL;
    prts_web_hooks_attach(NULL);
#endif
    prts_engine_destroy(a->engine);
    SDL_free(a);
}
