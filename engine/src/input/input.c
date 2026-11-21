/* 1stPRTS engine: device state and script-defined actions. MIT License. */
#include "prts/prts_input.h"

#include <math.h>
#include <string.h>

typedef struct binding {
    prts_binding_kind kind;
    int code;
    float scale;
} binding;

typedef struct action {
    char *name;
    prts_array bindings; /* binding */
    float value;
    bool down, was_down, pressed;
    float override_value;
    int override_ticks;
} action;

struct prts_input {
    prts_array actions; /* action */
    prts_key_resolver keys;
    prts_input_scheme scheme;
    bool scheme_changed;
};

const char *prts_input_scheme_name(prts_input_scheme s)
{
    switch (s) {
    case PRTS_SCHEME_KBM: return "kbm";
    case PRTS_SCHEME_GAMEPAD: return "gamepad";
    case PRTS_SCHEME_TOUCH: return "touch";
    default: return "xr";
    }
}

prts_input *prts_input_create(prts_key_resolver keys)
{
    prts_input *in = prts_calloc(1, sizeof(*in));
    prts_array_init(&in->actions, sizeof(action));
    in->keys = keys;
    return in;
}

void prts_input_destroy(prts_input *in)
{
    if (!in) return;
    for (size_t i = 0; i < in->actions.len; i++) {
        action *a = prts_array_at(&in->actions, i);
        prts_free(a->name);
        prts_array_free(&a->bindings);
    }
    prts_array_free(&in->actions);
    prts_free(in);
}

int prts_input_find(const prts_input *in, const char *name)
{
    for (size_t i = 0; i < in->actions.len; i++) {
        action *a = prts_array_at(&in->actions, i);
        if (strcmp(a->name, name) == 0) return (int)i;
    }
    return -1;
}

int prts_input_define(prts_input *in, const char *name)
{
    int found = prts_input_find(in, name);
    if (found >= 0) return found;
    action *a = prts_array_push(&in->actions, NULL);
    a->name = prts_strdup(name);
    prts_array_init(&a->bindings, sizeof(binding));
    return (int)in->actions.len - 1;
}

size_t prts_input_action_count(const prts_input *in) { return in->actions.len; }

const char *prts_input_action_name(const prts_input *in, int index)
{
    action *a = index >= 0 ? prts_array_at(&in->actions, (size_t)index) : NULL;
    return a ? a->name : NULL;
}

static action *get(const prts_input *in, const char *name)
{
    int i = prts_input_find(in, name);
    return i >= 0 ? prts_array_at(&in->actions, (size_t)i) : NULL;
}

bool prts_input_bind(prts_input *in, const char *name, prts_binding_kind kind, const char *key_name, int code,
                     float scale)
{
    action *a = get(in, name);
    if (!a) return false;
    if (kind == PRTS_BIND_KEY) {
        code = key_name && in->keys ? in->keys(key_name) : -1;
        if (code <= 0 || code >= PRTS_INPUT_MAX_KEYS) return false;
    }
    int limit = kind == PRTS_BIND_MOUSE_BUTTON ? PRTS_INPUT_MAX_BUTTONS
              : kind == PRTS_BIND_MOUSE_AXIS   ? 3
              : kind == PRTS_BIND_PAD_BUTTON   ? PRTS_INPUT_MAX_PAD_BUTTONS
              : kind == PRTS_BIND_PAD_AXIS     ? PRTS_INPUT_MAX_PAD_AXES
                                               : PRTS_INPUT_MAX_KEYS;
    if (code < 0 || code >= limit) return false;
    binding b = {kind, code, scale == 0.0f ? 1.0f : scale};
    prts_array_push(&a->bindings, &b);
    return true;
}

void prts_input_clear_bindings(prts_input *in, const char *name)
{
    action *a = get(in, name);
    if (a) prts_array_clear(&a->bindings);
}

static float binding_value(const binding *b, const prts_input_state *s)
{
    switch (b->kind) {
    case PRTS_BIND_KEY: return s->keys[b->code] ? b->scale : 0.0f;
    case PRTS_BIND_MOUSE_BUTTON: return s->mouse[b->code] ? b->scale : 0.0f;
    case PRTS_BIND_MOUSE_AXIS: return (b->code == 0 ? s->mouse_dx : b->code == 1 ? s->mouse_dy : s->wheel) * b->scale;
    case PRTS_BIND_PAD_BUTTON: return s->pad_connected && s->pad_buttons[b->code] ? b->scale : 0.0f;
    default: {
        float v = s->pad_connected ? s->pad_axes[b->code] : 0.0f;
        return fabsf(v) < 0.15f ? 0.0f : v * b->scale; /* dead zone */
    }
    }
}

void prts_input_update(prts_input *in, const prts_input_state *s)
{
    for (size_t i = 0; i < in->actions.len; i++) {
        action *a = prts_array_at(&in->actions, i);
        float v = 0.0f;
        for (size_t j = 0; j < a->bindings.len; j++) v += binding_value(prts_array_at(&a->bindings, j), s);
        if (a->override_ticks > 0) v = a->override_value;
        a->value = v;
        a->was_down = a->down;
        a->down = fabsf(v) > 0.5f;
        if (a->down && !a->was_down) a->pressed = true;
    }
    prts_input_scheme scheme = (prts_input_scheme)PRTS_CLAMP(s->last_device, 0, 3);
    if (scheme != in->scheme) {
        in->scheme = scheme;
        in->scheme_changed = true;
    }
}

float prts_input_value(const prts_input *in, const char *name)
{
    action *a = get(in, name);
    return a ? a->value : 0.0f;
}

bool prts_input_down(const prts_input *in, const char *name)
{
    action *a = get(in, name);
    return a && a->down;
}

bool prts_input_pressed(const prts_input *in, const char *name)
{
    action *a = get(in, name);
    return a && a->pressed;
}

void prts_input_end_tick(prts_input *in)
{
    for (size_t i = 0; i < in->actions.len; i++) ((action *)prts_array_at(&in->actions, i))->pressed = false;
}

void prts_input_override(prts_input *in, const char *name, float value, int ticks)
{
    action *a = get(in, name);
    if (!a) return;
    a->override_value = value;
    a->override_ticks = ticks;
}

void prts_input_tick_overrides(prts_input *in)
{
    for (size_t i = 0; i < in->actions.len; i++) {
        action *a = prts_array_at(&in->actions, i);
        if (a->override_ticks > 0) a->override_ticks--;
    }
}

prts_input_scheme prts_input_scheme_active(const prts_input *in) { return in->scheme; }

bool prts_input_take_scheme_changed(prts_input *in)
{
    bool c = in->scheme_changed;
    in->scheme_changed = false;
    return c;
}

void prts_input_set_scheme(prts_input *in, prts_input_scheme s)
{
    if (s != in->scheme) in->scheme_changed = true;
    in->scheme = s;
}
