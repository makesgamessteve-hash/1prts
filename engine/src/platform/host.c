/* 1stPRTS engine: signals from the host page (start-up input scheme hint, user gestures). MIT License. */
#include "prts/prts_platform.h"

static struct {
    bool has_hint;
    prts_input_scheme hint;
    prts_platform_gesture_fn gesture_fn;
    void *gesture_user;
    unsigned gestures;
} host;

void prts_platform_hint_scheme(prts_input_scheme scheme)
{
    host.hint = scheme;
    host.has_hint = true;
}

bool prts_platform_take_scheme_hint(prts_input_scheme *out)
{
    if (!host.has_hint) return false;
    host.has_hint = false;
    *out = host.hint;
    return true;
}

void prts_platform_set_gesture_handler(prts_platform_gesture_fn fn, void *user)
{
    host.gesture_fn = fn;
    host.gesture_user = user;
    /* the page may have been clicked while it was still loading */
    if (fn && host.gestures > 0) fn(user);
}

void prts_platform_user_gesture(void)
{
    host.gestures++;
    if (host.gesture_fn) host.gesture_fn(host.gesture_user);
}

unsigned prts_platform_gesture_count(void) { return host.gestures; }
