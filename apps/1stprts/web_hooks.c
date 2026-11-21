/* 1stPRTS launcher: browser test hooks, the C side of web/prts_test_hooks.js (window.prts_test,
 * only with ?test=1). Built only for the web with PRTS_WEB_TEST_HOOKS (CI); shipped builds have
 * none. MIT License. */
#include <emscripten.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "prts/prts_audio.h"
#include "prts/prts_platform.h"
#include "prts/prts_ui.h"
#include "prts/prts_vfs.h"
#include "prts/prts_xr.h"
#include "web_hooks.h"

int prts_test_state(void);
double prts_test_frames(void);
double prts_test_commander(int field);
int prts_test_scheme(void);
int prts_test_gestures(void);
double prts_test_xr(int field);
int prts_test_user_write(const char *path, const char *text);
const char *prts_test_user_read(const char *path);
double prts_test_scenario(int field);
const char *prts_test_scenario_text(int field);
double prts_test_audio(int field);
int prts_test_ui_has(const char *text);

static prts_engine *g_engine;
static bool g_stopped;
static char *g_read;

void prts_web_hooks_attach(prts_engine *e)
{
    if (!e && g_engine) g_stopped = true;
    g_engine = e;
}

static bool status(prts_engine_status *s)
{
    if (!g_engine) return false;
    prts_engine_get_status(g_engine, s);
    return true;
}

/* 0: booting, 1: running (a frame has been run), 2: stopped. */
EMSCRIPTEN_KEEPALIVE int prts_test_state(void)
{
    prts_engine_status s;
    if (g_stopped) return 2;
    return status(&s) && s.frames > 0 ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE double prts_test_frames(void)
{
    prts_engine_status s;
    return status(&s) ? (double)s.frames : 0.0;
}

/* 0-2: feet position x, y, z; 3: yaw; 4: pitch. NaN when there is no commander. */
EMSCRIPTEN_KEEPALIVE double prts_test_commander(int field)
{
    prts_engine_status s;
    if (!status(&s) || !s.has_commander || field < 0 || field > 4) return NAN;
    if (field < 3) return s.commander[field];
    return field == 3 ? (double)s.yaw : (double)s.pitch;
}

/* prts_input_scheme of the running engine, -1 before it runs. */
EMSCRIPTEN_KEEPALIVE int prts_test_scheme(void)
{
    prts_engine_status s;
    return status(&s) ? s.scheme : -1;
}

EMSCRIPTEN_KEEPALIVE int prts_test_gestures(void) { return (int)prts_platform_gesture_count(); }

/* 0 supported, 1 active, 2 views, 3 XR frames, 4 sessions, 5 connected controllers,
 * 6 framebuffer, 7 head height, 8 x of the second view's viewport. */
EMSCRIPTEN_KEEPALIVE double prts_test_xr(int field)
{
    const prts_xr *xr = prts_xr_get();
    int controllers = 0;
    for (int i = 0; i < PRTS_XR_MAX_CONTROLLERS; i++) controllers += xr->controllers[i].connected ? 1 : 0;
    switch (field) {
    case 0: return xr->supported ? 1.0 : 0.0;
    case 1: return xr->active ? 1.0 : 0.0;
    case 2: return xr->view_count;
    case 3: return (double)xr->frames;
    case 4: return (double)xr->sessions;
    case 5: return controllers;
    case 6: return xr->framebuffer;
    case 7: return xr->has_pose ? (double)xr->head.position[1] : NAN;
    case 8: return xr->view_count > 1 ? xr->views[1].viewport[0] : NAN;
    default: return NAN;
    }
}

/* Writes a text file in the user area through the VFS (on the web: IndexedDB-backed). */
EMSCRIPTEN_KEEPALIVE int prts_test_user_write(const char *path, const char *text)
{
    if (!g_engine) return 0;
    return prts_vfs_user_write(prts_engine_vfs(g_engine), path, text, strlen(text)) == PRTS_VFS_OK;
}

/* Reads a text file from the user area; NULL if it does not exist. */
EMSCRIPTEN_KEEPALIVE const char *prts_test_user_read(const char *path)
{
    prts_free(g_read);
    g_read = NULL;
    if (!g_engine) return NULL;
    size_t size = 0;
    if (prts_vfs_user_read(prts_engine_vfs(g_engine), path, &g_read, &size) != PRTS_VFS_OK) return NULL;
    return g_read;
}

/* Scenario runs (?scenario=<name>): 0 prts_scenario_state, 1 ticks run. */
EMSCRIPTEN_KEEPALIVE double prts_test_scenario(int field)
{
    prts_engine_status s;
    if (!status(&s)) return NAN;
    return field == 0 ? (double)s.scenario : field == 1 ? (double)s.scenario_ticks : NAN;
}

/* 0 the commander's final cell, 1 the state hash (16 hex digits), 2 why it failed. */
EMSCRIPTEN_KEEPALIVE const char *prts_test_scenario_text(int field)
{
    static char hash[24];
    prts_engine_status s;
    if (!status(&s)) return "";
    if (field == 0) return s.scenario_cell;
    if (field == 2) return s.scenario_message;
    if (s.scenario < PRTS_SCENARIO_PASSED) return "";
    snprintf(hash, sizeof(hash), "%016llx", (unsigned long long)s.scenario_hash);
    return hash;
}

/* 0 held (nothing mixed: the browser has not unlocked audio), 1 seconds mixed, 2 and 3 the
 * left and right level, 4 voices, 5 1 if music plays, 6 the music's fade-in (0..1). */
EMSCRIPTEN_KEEPALIVE double prts_test_audio(int field)
{
    if (!g_engine) return NAN;
    prts_audio *a = prts_engine_audio(g_engine);
    prts_mixer *m = prts_audio_mixer(a);
    float l = 0.0f, r = 0.0f;
    prts_audio_levels(a, &l, &r);
    prts_voice_info info;
    bool music = prts_mixer_voice_info(m, prts_audio_music_voice(a), &info);
    switch (field) {
    case 0: return prts_audio_held(a) ? 1.0 : 0.0;
    case 1: return (double)prts_audio_frames_mixed(a) / PRTS_AUDIO_RATE;
    case 2: return l;
    case 3: return r;
    case 4: return prts_mixer_voice_count(m);
    case 5: return music ? 1.0 : 0.0;
    case 6: return music ? info.fade : NAN;
    default: return NAN;
    }
}

/* 1 if a text drawn on the screen in the last frame contains `text` (like t.ui_has). */
EMSCRIPTEN_KEEPALIVE int prts_test_ui_has(const char *text)
{
    if (!g_engine) return 0;
    prts_ui *ui = prts_engine_ui(g_engine);
    size_t n = prts_ui_text_count(ui, PRTS_UI_SCREEN);
    for (size_t i = 0; i < n; i++) {
        if (strstr(prts_ui_text_at(ui, PRTS_UI_SCREEN, i), text)) return 1;
    }
    return 0;
}
