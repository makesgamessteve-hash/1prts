/* 1stPRTS engine: WebXR back end, the C side of web/prts_xr.js (design D13). Emscripten only. MIT License. */
#include <emscripten.h>

#include "prts/prts_xr.h"

/* Called from web/prts_xr.js. */
float *prts_xr_web_frame_buffer(void);
void prts_xr_web_set_supported(int supported);
void prts_xr_web_session_started(int framebuffer, int width, int height);
void prts_xr_web_session_ended(void);
int prts_xr_web_frame(int count);

static float frame_data[PRTS_XR_FRAME_FLOATS];

/* web/prts_xr.js writes each frame's poses here (layout: PRTS_XR_F_* in prts_xr.h). */
EMSCRIPTEN_KEEPALIVE float *prts_xr_web_frame_buffer(void) { return frame_data; }

EMSCRIPTEN_KEEPALIVE void prts_xr_web_set_supported(int supported)
{
    prts_xr_set_supported(prts_xr_get(), supported != 0);
}

/* The session runs: from now on the XR session's requestAnimationFrame drives prts_frame(),
 * so SDL's own loop (the window's requestAnimationFrame) is paused. */
EMSCRIPTEN_KEEPALIVE void prts_xr_web_session_started(int framebuffer, int width, int height)
{
    prts_xr_begin(prts_xr_get(), (unsigned)framebuffer, width, height);
    emscripten_pause_main_loop();
}

/* The game, the browser or the headset ended the session: hand frames back to SDL's loop. */
EMSCRIPTEN_KEEPALIVE void prts_xr_web_session_ended(void)
{
    if (!prts_xr_get()->active) return;
    prts_xr_end(prts_xr_get());
    emscripten_resume_main_loop();
}

EMSCRIPTEN_KEEPALIVE int prts_xr_web_frame(int count)
{
    return prts_xr_apply_frame(prts_xr_get(), frame_data, count > 0 ? (size_t)count : 0) ? 1 : 0;
}
