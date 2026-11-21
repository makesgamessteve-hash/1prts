/* 1stPRTS engine: entry points the web glue (web/prts_pre.js) calls. Emscripten only. MIT License. */
#include <emscripten.h>

#include "platform/web_glue.h"
#include "prts/prts_platform.h"

EMSCRIPTEN_KEEPALIVE void prts_web_set_touch_first(int touch_first)
{
    if (touch_first) prts_platform_hint_scheme(PRTS_SCHEME_TOUCH);
}

EMSCRIPTEN_KEEPALIVE void prts_web_user_gesture(void) { prts_platform_user_gesture(); }
