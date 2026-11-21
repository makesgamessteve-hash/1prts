/* 1stPRTS engine: collision world (boxes + terrain height grid). MIT License. */
#ifndef PRTS_COLLIDE_H
#define PRTS_COLLIDE_H

#include "prts_math.h"
#include "prts_world.h"

typedef struct prts_aabb {
    prts_vec3 min, max;
} prts_aabb;

prts_aabb prts_aabb_make(prts_vec3 a, prts_vec3 b);
bool prts_aabb_overlaps(prts_aabb a, prts_aabb b);

/* Terrain heights on a regular grid. */
typedef struct prts_heightfield {
    int nx, nz;
    float spacing;
    float origin_x, origin_z;
    float *heights;
} prts_heightfield;

prts_heightfield *prts_heightfield_create(int nx, int nz, float spacing, float origin_x, float origin_z);
void prts_heightfield_destroy(prts_heightfield *hf);
float prts_heightfield_get(const prts_heightfield *hf, int ix, int iz);
void prts_heightfield_set(prts_heightfield *hf, int ix, int iz, float h);
/* Bilinear height at a world position (clamped to the grid edge). */
float prts_heightfield_sample(const prts_heightfield *hf, float x, float z);
prts_vec3 prts_heightfield_normal(const prts_heightfield *hf, float x, float z);

typedef struct prts_cworld prts_cworld;

prts_cworld *prts_cworld_create(void);
void prts_cworld_destroy(prts_cworld *cw);
/* Adds a solid box. With an owner, the box is in the owner's local frame and follows it. */
int prts_cworld_add_box(prts_cworld *cw, prts_aabb box, prts_entity owner);
size_t prts_cworld_box_count(const prts_cworld *cw);
prts_aabb prts_cworld_box(const prts_cworld *cw, int id);
prts_entity prts_cworld_owner(const prts_cworld *cw, int id);
void prts_cworld_set_enabled(prts_cworld *cw, int id, bool enabled);
bool prts_cworld_enabled(const prts_cworld *cw, int id);
/* Enables or disables every box owned by an entity. */
void prts_cworld_set_owner_enabled(prts_cworld *cw, prts_entity owner, bool enabled);
void prts_cworld_set_heightfield(prts_cworld *cw, const prts_heightfield *hf);
/* Recomputes world-space boxes of owned boxes from their owners' transforms. */
void prts_cworld_sync(prts_cworld *cw, const prts_world *w);

typedef struct prts_move_result {
    bool on_ground;
    int ground_box; /* -1: terrain or nothing */
    bool hit_wall;
    bool stepped;
} prts_move_result;

/* Moves a box through the world with sliding and step-up. Updates *box and returns the
 * movement actually made. Boxes the mover already overlaps are ignored. */
prts_vec3 prts_cworld_move(const prts_cworld *cw, prts_aabb *box, prts_vec3 delta, float step_height,
                           prts_move_result *result);
bool prts_cworld_overlaps(const prts_cworld *cw, prts_aabb box);
/* True if anything (box or terrain) blocks the ray within max_dist (any-hit query). */
bool prts_cworld_occluded(const prts_cworld *cw, prts_vec3 origin, prts_vec3 dir, float max_dist);
/* Ray against boxes and terrain. Returns the box id, -2 for terrain, -1 for no hit. */
int prts_cworld_raycast(const prts_cworld *cw, prts_vec3 origin, prts_vec3 dir, float max_dist, float *out_dist);

#endif
