/* 1stPRTS engine: touch controls (stick, look drag, tap, context buttons). MIT License. */
#include "prts/prts_touch.h"

#include <math.h>
#include <string.h>

typedef enum finger_role { ROLE_IGNORED = 0, ROLE_STICK, ROLE_LOOK, ROLE_BUTTON } finger_role;

typedef struct finger {
    bool active;
    int64_t id;
    finger_role role;
    int button;
    float x0, y0; /* where it went down */
    float lx, ly; /* position at the last update */
    float time, travel;
} finger;

struct prts_touch {
    prts_touch_layout layout;
    finger fingers[PRTS_INPUT_MAX_TOUCHES];
};

void prts_touch_layout_default(prts_touch_layout *l)
{
    memset(l, 0, sizeof(*l));
    l->split = 0.5f;
    l->stick_radius = 0.12f;
    l->look_speed = 1000.0f;
    l->tap_distance = 0.03f;
    l->tap_seconds = 0.3f;
}

prts_touch *prts_touch_create(void)
{
    prts_touch *t = prts_calloc(1, sizeof(*t));
    prts_touch_layout_default(&t->layout);
    return t;
}

void prts_touch_destroy(prts_touch *t) { prts_free(t); }

void prts_touch_set_layout(prts_touch *t, const prts_touch_layout *l)
{
    t->layout = *l;
    t->layout.button_count = PRTS_CLAMP(l->button_count, 0, PRTS_TOUCH_MAX_BUTTONS);
    t->layout.stick_radius = fmaxf(l->stick_radius, 0.01f);
}

const prts_touch_layout *prts_touch_get_layout(const prts_touch *t) { return &t->layout; }

int prts_touch_find_button(const prts_touch *t, const char *id)
{
    for (int i = 0; i < t->layout.button_count; i++) {
        if (strcmp(t->layout.buttons[i].id, id) == 0) return i;
    }
    return -1;
}

void prts_touch_set_visible(prts_touch *t, int button, bool visible)
{
    if (button >= 0 && button < t->layout.button_count) t->layout.buttons[button].visible = visible;
}

static bool stick_taken(const prts_touch *t)
{
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) {
        if (t->fingers[i].active && t->fingers[i].role == ROLE_STICK) return true;
    }
    return false;
}

static finger_role classify(const prts_touch *t, float x, float y, float aspect, int *button)
{
    for (int b = 0; b < t->layout.button_count; b++) {
        const prts_touch_button *tb = &t->layout.buttons[b];
        float dx = (x - tb->x) * aspect, dy = y - tb->y;
        if (tb->visible && dx * dx + dy * dy <= tb->radius * tb->radius) {
            *button = b;
            return ROLE_BUTTON;
        }
    }
    if (x < t->layout.split) return stick_taken(t) ? ROLE_IGNORED : ROLE_STICK;
    return ROLE_LOOK;
}

static const prts_touch_point *find_point(const prts_input_state *s, int64_t id)
{
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) {
        if (s->touches[i].down && s->touches[i].id == id) return &s->touches[i];
    }
    return NULL;
}

void prts_touch_update(prts_touch *t, prts_input_state *s, float dt, float aspect)
{
    const prts_touch_layout *l = &t->layout;
    memset(s->touch_axes, 0, sizeof(s->touch_axes));
    memset(s->touch_buttons, 0, sizeof(s->touch_buttons));
    /* fingers that were lifted; a short, still press on the look side is a tap */
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) {
        finger *f = &t->fingers[i];
        if (!f->active || find_point(s, f->id)) continue;
        if (f->role == ROLE_LOOK && f->travel <= l->tap_distance && f->time <= l->tap_seconds) s->touch_buttons[0] = true;
        f->active = false;
    }
    /* new fingers get their role from where they went down */
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) {
        const prts_touch_point *p = &s->touches[i];
        bool known = false;
        for (int k = 0; k < PRTS_INPUT_MAX_TOUCHES; k++) known = known || (t->fingers[k].active && t->fingers[k].id == p->id);
        if (!p->down || known) continue;
        for (int k = 0; k < PRTS_INPUT_MAX_TOUCHES; k++) {
            finger *f = &t->fingers[k];
            if (f->active) continue;
            memset(f, 0, sizeof(*f));
            f->active = true;
            f->id = p->id;
            f->role = classify(t, p->x, p->y, aspect, &f->button);
            f->x0 = f->lx = p->x;
            f->y0 = f->ly = p->y;
            break;
        }
    }
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) {
        finger *f = &t->fingers[i];
        if (!f->active) continue;
        const prts_touch_point *p = find_point(s, f->id);
        f->time += dt;
        if (f->role == ROLE_STICK) {
            float dx = (p->x - f->x0) * aspect / l->stick_radius, dy = (p->y - f->y0) / l->stick_radius;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 1.0f) {
                dx /= len;
                dy /= len;
            }
            s->touch_axes[0] = dx;
            s->touch_axes[1] = dy;
        } else if (f->role == ROLE_LOOK) {
            float dx = (p->x - f->lx) * aspect, dy = p->y - f->ly;
            s->touch_axes[2] += dx * l->look_speed;
            s->touch_axes[3] += dy * l->look_speed;
            f->travel += sqrtf(dx * dx + dy * dy);
        } else if (f->role == ROLE_BUTTON) {
            s->touch_buttons[f->button + 1] = true;
        }
        f->lx = p->x;
        f->ly = p->y;
    }
}

bool prts_touch_stick(const prts_touch *t, float *ox, float *oy, float *kx, float *ky)
{
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) {
        const finger *f = &t->fingers[i];
        if (f->active && f->role == ROLE_STICK) {
            *ox = f->x0;
            *oy = f->y0;
            *kx = f->lx;
            *ky = f->ly;
            return true;
        }
    }
    return false;
}

bool prts_touch_button_held(const prts_touch *t, int button)
{
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) {
        const finger *f = &t->fingers[i];
        if (f->active && f->role == ROLE_BUTTON && f->button == button) return true;
    }
    return false;
}
