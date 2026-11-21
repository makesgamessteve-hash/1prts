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

/* How far a teleport can reach along the pointer ray (meters). */
#define PRTS_TELEPORT_RANGE 12.0f

/* Per-tick input from an XR session, set by the engine before each tick (and recorded in
 * replays like the look direction). Positions are relative to the commander's feet, in world
 * axes. All zero outside XR. */
typedef struct prts_commander_xr {
    bool active;      /* the head and the hands come from an XR session */
    bool teleport;    /* teleport locomotion: the move actions do not walk; "teleport" aims */
    bool has_pointer; /* pointer_origin and pointer_dir hold a controller's pointer ray */
    prts_vec3 head;   /* the tracked head: the body follows it across the floor */
    prts_vec3 pointer_origin, pointer_dir;
} prts_commander_xr;

typedef struct prts_commander {
    prts_entity entity; /* feet position; parent frame = what it stands on */
    prts_commander_params p;
    float yaw, pitch;
    float vertical_speed;
    bool on_ground, crouched, hit_wall;
    float eye;      /* current eye height above the feet (smoothed) */
    float eye_prev; /* previous tick, for interpolation */
    float stamina;
    prts_commander_xr xr; /* XR input for the next tick */
    prts_vec3 followed;   /* how far the body followed the head in the last tick (world axes) */
    bool aiming;          /* teleport locomotion: the "teleport" action is held (aiming) */
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
/* The usable entity the commander is looking at within reach, or PRTS_ENTITY_NONE. In XR the
 * controller's pointer ray picks it instead of the view, unless the hand is inside a wall. */
prts_entity prts_commander_target(const prts_commander *c, const prts_world *w, const prts_cworld *cw);
/* Teleport locomotion: the floor point the XR pointer ray picks (within PRTS_TELEPORT_RANGE).
 * True, with the feet position there in *target, if it is walkable floor with room to stand
 * and no wall between it and the commander (a ray 1 m above the higher of the two floors). */
bool prts_commander_teleport_target(const prts_commander *c, const prts_world *w, const prts_cworld *cw,
                                    prts_dvec3 *target);
/* XR head collision: the tracked head (relative to the feet) is swept from the body's centre
 * towards where it is, sliding along walls, so the view never enters solid geometry. Returns
 * the offset to add to the views (zero when nothing is in the way). */
prts_vec3 prts_commander_head_pushback(const prts_commander *c, const prts_cworld *cw, prts_dvec3 feet,
                                       prts_vec3 head);

#endif
