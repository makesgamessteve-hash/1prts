/* 1stPRTS engine: the commander's first-person body. MIT License. */
#ifndef PRTS_COMMANDER_H
#define PRTS_COMMANDER_H

#include "prts_collide.h"
#include "prts_input.h"
#include "prts_world.h"

typedef struct prts_commander_params {
    float walk_speed, run_speed, sprint_speed, crouch_speed;
    float jump_speed, gravity;
    float eye_stand, eye_crouch;
    float height_stand, height_crouch, half_width;
    float step_height;
    float stamina_max, stamina_drain, stamina_regen;
    float reach;
    float look_sensitivity; /* radians per mouse pixel */
} prts_commander_params;

typedef struct prts_commander {
    prts_entity entity; /* feet position; parent frame = what it stands on */
    prts_commander_params p;
    float yaw, pitch;
    float vertical_speed;
    bool on_ground, crouched, hit_wall;
    float eye;      /* current eye height above the feet (smoothed) */
    float eye_prev; /* previous tick, for interpolation */
    float stamina;
} prts_commander;

void prts_commander_params_default(prts_commander_params *p);
void prts_commander_init(prts_commander *c, prts_entity e, const prts_commander_params *p);
/* Applies look input; called every rendered frame. Pitch is clamped to about +-85 degrees. */
void prts_commander_look(prts_commander *c, float dyaw, float dpitch);
/* Simulates one fixed tick of movement. */
void prts_commander_tick(prts_commander *c, prts_world *w, const prts_cworld *cw, const prts_input *in, float dt);
prts_aabb prts_commander_box(const prts_commander *c, const prts_world *w);
/* Eye position in world space, interpolated between ticks. */
prts_dvec3 prts_commander_eye(const prts_commander *c, const prts_world *w, double alpha);
/* The usable entity the commander is looking at within reach, or PRTS_ENTITY_NONE. */
prts_entity prts_commander_target(const prts_commander *c, const prts_world *w, const prts_cworld *cw);

#endif
