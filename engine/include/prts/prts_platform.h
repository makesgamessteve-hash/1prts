/* 1stPRTS engine: window, GL context, events and time (implemented with SDL3). MIT License. */
#ifndef PRTS_PLATFORM_H
#define PRTS_PLATFORM_H

#include "prts_base.h"
#include "prts_input.h"

typedef struct prts_platform prts_platform;

typedef struct prts_platform_config {
    const char *title;
    int width, height;
    bool hidden;     /* tests and screenshots */
    int gl_major, gl_minor;
} prts_platform_config;

/* Initialises video and creates a window with an OpenGL context. Returns NULL (and logs a
 * readable message) when no suitable context can be created. */
prts_platform *prts_platform_create(const prts_platform_config *cfg);
void prts_platform_destroy(prts_platform *p);
bool prts_platform_gles(const prts_platform *p);
void *prts_platform_gl_getproc(const char *name);
void prts_platform_swap(prts_platform *p);
void prts_platform_drawable_size(const prts_platform *p, int *w, int *h);
void prts_platform_set_relative_mouse(prts_platform *p, bool on);
/* Turns delivery of typed text (prts_input_state.text) on or off, e.g. for the console. */
void prts_platform_text_input(prts_platform *p, bool on);

/* Applies one SDL event (passed as void* so callers need no SDL headers) to the device
 * state. Returns true if the event asks the application to quit. */
bool prts_platform_apply_event(prts_platform *p, const void *sdl_event, prts_input_state *state);
/* Starts a new frame of device state (clears per-frame mouse motion and typed text). */
void prts_platform_begin_frame(prts_input_state *state);
/* Polls and applies all pending events; returns true if quitting was requested. */
bool prts_platform_poll(prts_platform *p, prts_input_state *state);

/* SDL scancode for a key name such as "W", "Left Shift" or "Space"; 0 if unknown. */
int prts_platform_key_from_name(const char *name);
/* Name of an SDL scancode ("E", "Left Shift"); "" if it has none. */
const char *prts_platform_key_name(int code);
uint64_t prts_platform_ticks_ns(void);
/* Shows a native error message box when a window system is available (on the web: a message
 * on the page). */
void prts_platform_error_box(const char *title, const char *message);

/* --- Signals from the host page (web/ glue); see engine/src/platform/host.c ------------- */

/* Suggests the input scheme for the next frame, for example touch on a touch-first phone or
 * tablet (the web glue checks this at start-up). The engine applies it once; real device input
 * switches the scheme as usual afterwards. */
void prts_platform_hint_scheme(prts_input_scheme scheme);
/* Takes the pending scheme hint. Returns false if there is none. */
bool prts_platform_take_scheme_hint(prts_input_scheme *out);

/* Browsers start audio only after a user gesture (click, tap or key press). The web glue
 * reports every such gesture here; the audio module registers a handler that resumes its
 * device. A handler registered after a gesture was already reported is called at once.
 * Desktop platforms never report gestures. */
typedef void (*prts_platform_gesture_fn)(void *user);
void prts_platform_set_gesture_handler(prts_platform_gesture_fn fn, void *user);
void prts_platform_user_gesture(void);
/* Number of user gestures reported so far (tests and the browser test hooks). */
unsigned prts_platform_gesture_count(void);

#endif
