/* 1stPRTS engine: device state and script-defined actions. MIT License. */
#ifndef PRTS_INPUT_H
#define PRTS_INPUT_H

#include "prts_base.h"

#define PRTS_INPUT_MAX_KEYS 512
#define PRTS_INPUT_MAX_BUTTONS 8
#define PRTS_INPUT_MAX_PAD_AXES 6
#define PRTS_INPUT_MAX_PAD_BUTTONS 32

/* Raw device state, filled by the platform layer. Mouse motion is accumulated per frame. */
typedef struct prts_input_state {
    bool keys[PRTS_INPUT_MAX_KEYS];
    bool mouse[PRTS_INPUT_MAX_BUTTONS];
    float mouse_dx, mouse_dy, wheel;
    bool pad_connected;
    float pad_axes[PRTS_INPUT_MAX_PAD_AXES];
    bool pad_buttons[PRTS_INPUT_MAX_PAD_BUTTONS];
    int fingers; /* active touch points */
    /* which device produced input most recently */
    int last_device;
} prts_input_state;

typedef enum prts_input_scheme {
    PRTS_SCHEME_KBM = 0,
    PRTS_SCHEME_GAMEPAD,
    PRTS_SCHEME_TOUCH,
    PRTS_SCHEME_XR
} prts_input_scheme;

const char *prts_input_scheme_name(prts_input_scheme s);

typedef enum prts_binding_kind {
    PRTS_BIND_KEY = 0,
    PRTS_BIND_MOUSE_BUTTON,
    PRTS_BIND_MOUSE_AXIS, /* 0: x, 1: y, 2: wheel */
    PRTS_BIND_PAD_BUTTON,
    PRTS_BIND_PAD_AXIS    /* code: axis index; scale may be negative */
} prts_binding_kind;

typedef struct prts_input prts_input;
typedef int (*prts_key_resolver)(const char *name);

prts_input *prts_input_create(prts_key_resolver keys);
void prts_input_destroy(prts_input *in);

/* Defines (or finds) an action. Returns its index. */
int prts_input_define(prts_input *in, const char *action);
int prts_input_find(const prts_input *in, const char *action);
size_t prts_input_action_count(const prts_input *in);
const char *prts_input_action_name(const prts_input *in, int index);
/* Adds a binding; returns false for unknown actions, key names or codes. */
bool prts_input_bind(prts_input *in, const char *action, prts_binding_kind kind, const char *key_name, int code,
                     float scale);
void prts_input_clear_bindings(prts_input *in, const char *action);

/* Computes action values from device state. Call once per frame. */
void prts_input_update(prts_input *in, const prts_input_state *state);
float prts_input_value(const prts_input *in, const char *action);
bool prts_input_down(const prts_input *in, const char *action);
/* True if the action went down since the last end_tick (so no press is lost between ticks). */
bool prts_input_pressed(const prts_input *in, const char *action);
void prts_input_end_tick(prts_input *in);

/* Scripted input for tests: holds a value for some ticks, or presses once. */
void prts_input_override(prts_input *in, const char *action, float value, int ticks);
void prts_input_tick_overrides(prts_input *in);

prts_input_scheme prts_input_scheme_active(const prts_input *in);
/* True once after the active scheme changed. */
bool prts_input_take_scheme_changed(prts_input *in);
void prts_input_set_scheme(prts_input *in, prts_input_scheme s);

#endif
