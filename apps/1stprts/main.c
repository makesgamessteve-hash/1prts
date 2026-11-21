/* 1stPRTS launcher: SDL3 app callbacks around the engine. MIT License. */
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

#include "prts/prts_engine.h"
#include "prts/prts_platform.h"

typedef struct app {
    prts_engine *engine;
    uint64_t last_ns;
} app;

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
    if (prts_engine_is_batch(e) || cfg.scenario) {
        int code = prts_engine_run(e);
        prts_engine_destroy(e);
        return code == 0 ? SDL_APP_SUCCESS : SDL_APP_FAILURE;
    }
    app *a = SDL_calloc(1, sizeof(*a));
    a->engine = e;
    a->last_ns = prts_platform_ticks_ns();
    *appstate = a;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    app *a = appstate;
    uint64_t now = prts_platform_ticks_ns();
    double dt = (double)(now - a->last_ns) / 1e9;
    a->last_ns = now;
    return prts_engine_frame(a->engine, dt) ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
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
    if (!a) return;
    prts_engine_destroy(a->engine);
    SDL_free(a);
}
