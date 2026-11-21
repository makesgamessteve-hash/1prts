/* 1stPRTS engine: fixed-tick clock, entities and nested reference frames. MIT License. */
#ifndef PRTS_WORLD_H
#define PRTS_WORLD_H

#include "prts_base.h"
#include "prts_math.h"

/* --- fixed simulation tick ---------------------------------------------------- */

typedef struct prts_clock {
    double tick_seconds;
    double accumulator;
    int max_catchup;
    uint64_t tick;
} prts_clock;

void prts_clock_init(prts_clock *c, int ticks_per_second, int max_catchup);
/* Adds real time and returns how many ticks to run now (at most max_catchup). Time beyond
 * that is dropped, so the game slows down instead of skipping simulation steps. The caller
 * counts the ticks it runs in `tick`. */
int prts_clock_advance(prts_clock *c, double real_seconds);
/* Fraction of a tick since the last tick, for interpolation (0..1). */
double prts_clock_alpha(const prts_clock *c);

/* --- entities ------------------------------------------------------------------ */

/* Entity handle: generation in the high 32 bits, slot index + 1 in the low 32 bits.
 * 0 is "no entity" and also means "the world frame" when used as a parent. */
typedef uint64_t prts_entity;
#define PRTS_ENTITY_NONE ((prts_entity)0)

typedef struct prts_world prts_world;

prts_world *prts_world_create(void);
void prts_world_destroy(prts_world *w);

prts_entity prts_world_spawn(prts_world *w, const char *name);
bool prts_world_alive(const prts_world *w, prts_entity e);
/* Destroys e and every entity parented (directly or indirectly) to it. */
void prts_world_despawn(prts_world *w, prts_entity e);
size_t prts_world_count(const prts_world *w);
const char *prts_world_name(const prts_world *w, prts_entity e);
prts_entity prts_world_find(const prts_world *w, const char *name);
/* Calls fn for each living entity in slot order. */
typedef void (*prts_world_each_fn)(prts_entity e, void *user);
void prts_world_each(const prts_world *w, prts_world_each_fn fn, void *user);

/* Transform relative to the parent frame (rotation is yaw only in M1). */
void prts_world_set_local(prts_world *w, prts_entity e, prts_dvec3 pos, float yaw);
prts_dvec3 prts_world_local_pos(const prts_world *w, prts_entity e);
float prts_world_local_yaw(const prts_world *w, prts_entity e);
/* World transform, resolved through every parent frame in double precision. */
prts_dvec3 prts_world_pos(const prts_world *w, prts_entity e);
float prts_world_yaw(const prts_world *w, prts_entity e);
void prts_world_set_pos(prts_world *w, prts_entity e, prts_dvec3 world_pos);

prts_entity prts_world_parent(const prts_world *w, prts_entity e);
/* Moves e into another frame, keeping its world position and orientation. Refuses
 * cycles and dead parents. */
bool prts_world_set_parent(prts_world *w, prts_entity e, prts_entity parent);

/* World-space velocity of bodies (not changed by reparenting). */
void prts_world_set_velocity(prts_world *w, prts_entity e, prts_vec3 v);
prts_vec3 prts_world_velocity(const prts_world *w, prts_entity e);

/* Interpolation: begin_tick stores this tick's start pose for every entity. */
void prts_world_begin_tick(prts_world *w);
/* Makes e start this tick where it is now, so a jump (a teleport) is not interpolated. */
void prts_world_skip_interp(prts_world *w, prts_entity e);
prts_dvec3 prts_world_pos_interp(const prts_world *w, prts_entity e, double alpha);
float prts_world_yaw_interp(const prts_world *w, prts_entity e, double alpha);

/* Kinematic movers (doors, lifts): constant-speed moves of the local transform. */
void prts_world_move_to(prts_world *w, prts_entity e, prts_dvec3 local_target, float speed);
void prts_world_rotate_to(prts_world *w, prts_entity e, float local_yaw_target, float speed);
bool prts_world_moving(const prts_world *w, prts_entity e);
typedef void (*prts_world_done_fn)(prts_entity e, void *user);
void prts_world_tick_movers(prts_world *w, float dt, prts_world_done_fn done, void *user);

/* Usable entities can be activated with the 'use' action. */
void prts_world_set_usable(prts_world *w, prts_entity e, bool usable);
bool prts_world_usable(const prts_world *w, prts_entity e);

/* 64-bit hash over all simulation state held by the world. */
uint64_t prts_world_hash(const prts_world *w);

/* Saves: writes every entity slot (including free slots and generations) bit for bit, and
 * reads it back, replacing the world's contents. A failed read leaves the world unchanged. */
struct prts_writer;
struct prts_reader;
void prts_world_write(const prts_world *w, struct prts_writer *out);
bool prts_world_read(prts_world *w, struct prts_reader *in);

#endif
