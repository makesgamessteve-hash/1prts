/* 1stPRTS engine: touch controls. Fingers become virtual axes and buttons: a floating movement
 * stick on the left, look-by-drag and tap on the right, and context buttons. The layout comes
 * from scripts; actions bind to the result like to any other device. MIT License. */
#ifndef PRTS_TOUCH_H
#define PRTS_TOUCH_H

#include "prts_input.h"

#define PRTS_TOUCH_MAX_BUTTONS (PRTS_INPUT_TOUCH_BUTTONS - 1)
#define PRTS_TOUCH_ID_MAX 32

/* A round context button. Position: fractions of the width and height; radius: fraction of
 * the height. Button i drives touch button i + 1 (touch button 0 is the tap). */
typedef struct prts_touch_button {
    char id[PRTS_TOUCH_ID_MAX];
    float x, y, radius;
    bool visible; /* hidden buttons ignore fingers (scripts show them only when usable) */
} prts_touch_button;

typedef struct prts_touch_layout {
    float split;        /* a new finger left of this fraction of the width drives the stick */
    bool stick_right;   /* left-handed (M9): the stick is right of split instead, looking left of it */
    float stick_radius; /* finger distance for full stick deflection, fraction of the height */
    float look_speed;   /* look units (like mouse pixels) per screen height of dragging */
    float tap_distance; /* the most a finger may move and still tap, fraction of the height */
    float tap_seconds;  /* the longest press that still counts as a tap */
    int button_count;
    prts_touch_button buttons[PRTS_TOUCH_MAX_BUTTONS];
} prts_touch_layout;

void prts_touch_layout_default(prts_touch_layout *l);

typedef struct prts_touch prts_touch;

prts_touch *prts_touch_create(void);
void prts_touch_destroy(prts_touch *t);
void prts_touch_set_layout(prts_touch *t, const prts_touch_layout *l);
const prts_touch_layout *prts_touch_get_layout(const prts_touch *t);
int prts_touch_find_button(const prts_touch *t, const char *id);
void prts_touch_set_visible(prts_touch *t, int button, bool visible);

/* Turns s->touches into s->touch_axes and s->touch_buttons for this frame. Every finger
 * keeps the role it got when it went down, so moving and looking work at the same time.
 * aspect is the window's width / height. */
void prts_touch_update(prts_touch *t, prts_input_state *s, float dt, float aspect);

/* For drawing the overlay: the stick's origin and knob (fractions of the window), and
 * whether a context button is held. */
bool prts_touch_stick(const prts_touch *t, float *ox, float *oy, float *kx, float *ky);
bool prts_touch_button_held(const prts_touch *t, int button);

#endif
