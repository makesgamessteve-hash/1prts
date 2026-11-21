/* 1stPRTS engine: navigation for units: a walkability grid over a map, A* paths and the
 * agents that walk them. MIT License. */
#ifndef PRTS_NAV_H
#define PRTS_NAV_H

#include "prts_collide.h"
#include "prts_world.h"

struct prts_writer;
struct prts_reader;
struct prts_hasher;

/* --- the grid ------------------------------------------------------------------------------ */

/* A rectangle of the XZ plane in square cells. A cell is walkable if a body fits on the ground
 * under its centre (terrain or the top of a solid box); neighbours are joined if the ground
 * between them climbs no more than one cell per cell (45 degrees). Buildings placed later block
 * cells with a counter (prts_nav_block). */
typedef struct prts_nav prts_nav;

#define PRTS_NAV_MAX_EXPAND 200000 /* A* gives up after this many cells */

prts_nav *prts_nav_build(const prts_cworld *cw, float min_x, float min_z, float max_x, float max_z, float cell);
void prts_nav_destroy(prts_nav *n);
int prts_nav_cols(const prts_nav *n);
int prts_nav_rows(const prts_nav *n);
float prts_nav_cell(const prts_nav *n);
/* True if the point is inside the grid on a walkable, unblocked cell. */
bool prts_nav_walkable(const prts_nav *n, float x, float z);
/* The ground height of the cell under the point (false outside the grid or in a solid cell). */
bool prts_nav_height(const prts_nav *n, float x, float z, float *h);
/* Adds `delta` (+1 to block, -1 to free) to the block count of every cell whose centre is in
 * the rectangle. Returns the number of cells changed. */
int prts_nav_block(prts_nav *n, float min_x, float min_z, float max_x, float max_z, int delta);
/* Adds delta to the block counter of every cell whose ground lies below `height` (deep water);
 * returns how many cells it touched. */
int prts_nav_block_below(prts_nav *n, float height, int delta);
/* Reads the world again under the rectangle (a tree felled, a wall knocked down). The cells
 * read are not saved: scripts that change the world redo it after loading. */
int prts_nav_refresh(prts_nav *n, const prts_cworld *cw, float min_x, float min_z, float max_x, float max_z);
/* A smoothed path from (x0, z0) to (x1, z1): writes up to `max` points (x, z pairs) into `out`,
 * not counting the start, and returns their number; -1 if there is none. A start or goal in a
 * blocked cell is moved to the nearest walkable cell. The same request gives the same path. */
int prts_nav_path(const prts_nav *n, float x0, float z0, float x1, float z1, float *out, int max);
/* Whether a straight walk between the points stays on joined walkable cells. */
bool prts_nav_clear_line(const prts_nav *n, float x0, float z0, float x1, float z1);

void prts_nav_hash(const prts_nav *n, struct prts_hasher *h);
void prts_nav_write(const prts_nav *n, struct prts_writer *w);
/* Reads the block counts written by prts_nav_write for a grid of the same size; with `apply`
 * false it only checks them. */
bool prts_nav_read(prts_nav *n, struct prts_reader *r, bool apply);

/* --- agents -------------------------------------------------------------------------------- */

/* Units that walk paths: each agent belongs to an entity (in the world frame), follows a list of
 * points at its speed on the grid's ground, keeps apart from other agents and turns to face
 * where it goes. Agents are kept and updated in entity order. */
typedef struct prts_agents prts_agents;

typedef struct prts_agent_info {
    float speed, radius;
    bool moving;
    float tx, tz;   /* the point it is heading for (its position when idle) */
    int left;       /* points left on its path */
} prts_agent_info;

prts_agents *prts_agents_create(void);
void prts_agents_destroy(prts_agents *a);
size_t prts_agents_count(const prts_agents *a);
bool prts_agents_add(prts_agents *a, prts_entity e, float speed, float radius);
bool prts_agents_remove(prts_agents *a, prts_entity e);
bool prts_agents_info(const prts_agents *a, prts_entity e, prts_agent_info *out);
bool prts_agents_set_speed(prts_agents *a, prts_entity e, float speed);
/* Gives the agent a path (x, z pairs); 0 points stops it. False if e is not an agent. */
bool prts_agents_follow(prts_agents *a, prts_entity e, const float *xz, int count);
/* One tick: moves every agent, drops agents whose entity is gone, and writes the entities that
 * reached the end of their path into `arrived` (up to max). Returns how many arrived. */
int prts_agents_tick(prts_agents *a, prts_world *w, const prts_nav *nav, float dt, prts_entity *arrived, int max);
/* Agents within r of (x, z), nearest first (ties by entity); up to max. Returns the count. */
int prts_agents_near(const prts_agents *a, const prts_world *w, float x, float z, float r, prts_entity *out, int max);

void prts_agents_hash(const prts_agents *a, struct prts_hasher *h);
void prts_agents_write(const prts_agents *a, struct prts_writer *w);
bool prts_agents_read(prts_agents *a, struct prts_reader *r);

#endif
