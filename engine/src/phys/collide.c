/* 1stPRTS engine: collision world (boxes + terrain height grid). MIT License. */
#include "prts/prts_collide.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define EPS 1e-4f

prts_aabb prts_aabb_make(prts_vec3 a, prts_vec3 b)
{
    prts_aabb r = {prts_v3(fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z)),
                   prts_v3(fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z))};
    return r;
}

bool prts_aabb_overlaps(prts_aabb a, prts_aabb b)
{
    return a.min.x < b.max.x - EPS && a.max.x > b.min.x + EPS && a.min.y < b.max.y - EPS && a.max.y > b.min.y + EPS &&
           a.min.z < b.max.z - EPS && a.max.z > b.min.z + EPS;
}

prts_heightfield *prts_heightfield_create(int nx, int nz, float spacing, float origin_x, float origin_z)
{
    prts_heightfield *hf = prts_calloc(1, sizeof(*hf));
    hf->nx = nx;
    hf->nz = nz;
    hf->spacing = spacing;
    hf->origin_x = origin_x;
    hf->origin_z = origin_z;
    hf->heights = prts_calloc((size_t)nx * (size_t)nz, sizeof(float));
    return hf;
}

void prts_heightfield_destroy(prts_heightfield *hf)
{
    if (!hf) return;
    prts_free(hf->heights);
    prts_free(hf);
}

float prts_heightfield_get(const prts_heightfield *hf, int ix, int iz)
{
    ix = PRTS_CLAMP(ix, 0, hf->nx - 1);
    iz = PRTS_CLAMP(iz, 0, hf->nz - 1);
    return hf->heights[iz * hf->nx + ix];
}

void prts_heightfield_set(prts_heightfield *hf, int ix, int iz, float h)
{
    if (ix < 0 || iz < 0 || ix >= hf->nx || iz >= hf->nz) return;
    hf->heights[iz * hf->nx + ix] = h;
}

float prts_heightfield_sample(const prts_heightfield *hf, float x, float z)
{
    float fx = (x - hf->origin_x) / hf->spacing, fz = (z - hf->origin_z) / hf->spacing;
    fx = prts_clampf(fx, 0.0f, (float)(hf->nx - 1));
    fz = prts_clampf(fz, 0.0f, (float)(hf->nz - 1));
    int ix = (int)floorf(fx), iz = (int)floorf(fz);
    float tx = fx - (float)ix, tz = fz - (float)iz;
    float h00 = prts_heightfield_get(hf, ix, iz), h10 = prts_heightfield_get(hf, ix + 1, iz);
    float h01 = prts_heightfield_get(hf, ix, iz + 1), h11 = prts_heightfield_get(hf, ix + 1, iz + 1);
    float a = h00 + (h10 - h00) * tx, b = h01 + (h11 - h01) * tx;
    return a + (b - a) * tz;
}

prts_vec3 prts_heightfield_normal(const prts_heightfield *hf, float x, float z)
{
    float d = hf->spacing;
    float hx = prts_heightfield_sample(hf, x + d, z) - prts_heightfield_sample(hf, x - d, z);
    float hz = prts_heightfield_sample(hf, x, z + d) - prts_heightfield_sample(hf, x, z - d);
    return prts_v3_normalize(prts_v3(-hx, 2.0f * d, -hz));
}

typedef struct cbox {
    prts_aabb local;
    prts_aabb world;
    prts_entity owner;
    bool enabled;
} cbox;

/* Static boxes (no owner) live in a bounding volume hierarchy, rebuilt when boxes are added;
 * boxes owned by entities move every tick and are checked one by one. Queries collect the
 * candidate ids in ascending order, so results do not depend on the tree's shape. */
typedef struct bvh_node {
    prts_aabb box;
    int left, right;     /* children (inner nodes) */
    size_t first, count; /* items (leaves: count > 0) */
} bvh_node;

typedef struct bvh_item {
    int id;
    prts_vec3 c; /* centre */
} bvh_item;

typedef struct bvh_state {
    prts_array nodes;   /* bvh_node; 0 is the root */
    prts_array items;   /* bvh_item, ordered by the tree */
    prts_array dynamic; /* int: ids of owned boxes */
    prts_array found;   /* int: candidates of the last query */
    prts_array stack;   /* int */
    bool dirty;
} bvh_state;

struct prts_cworld {
    prts_array boxes; /* cbox */
    const prts_heightfield *hf;
    float slope_cos; /* terrain steeper than this (cosine of the angle) cannot be walked up */
    bvh_state *bvh;  /* a cache: queries on a const world may rebuild it */
};

prts_cworld *prts_cworld_create(void)
{
    prts_cworld *cw = prts_calloc(1, sizeof(*cw));
    prts_array_init(&cw->boxes, sizeof(cbox));
    prts_cworld_set_max_slope(cw, (float)PRTS_PI / 4.0f);
    cw->bvh = prts_calloc(1, sizeof(bvh_state));
    prts_array_init(&cw->bvh->nodes, sizeof(bvh_node));
    prts_array_init(&cw->bvh->items, sizeof(bvh_item));
    prts_array_init(&cw->bvh->dynamic, sizeof(int));
    prts_array_init(&cw->bvh->found, sizeof(int));
    prts_array_init(&cw->bvh->stack, sizeof(int));
    return cw;
}

void prts_cworld_destroy(prts_cworld *cw)
{
    if (!cw) return;
    prts_array_free(&cw->boxes);
    prts_array_free(&cw->bvh->nodes);
    prts_array_free(&cw->bvh->items);
    prts_array_free(&cw->bvh->dynamic);
    prts_array_free(&cw->bvh->found);
    prts_array_free(&cw->bvh->stack);
    prts_free(cw->bvh);
    prts_free(cw);
}

int prts_cworld_add_box(prts_cworld *cw, prts_aabb box, prts_entity owner)
{
    cbox b = {box, box, owner, true};
    prts_array_push(&cw->boxes, &b);
    cw->bvh->dirty = true;
    return (int)cw->boxes.len - 1;
}

size_t prts_cworld_box_count(const prts_cworld *cw) { return cw->boxes.len; }

static cbox *box_at(const prts_cworld *cw, int id) { return id >= 0 ? prts_array_at(&cw->boxes, (size_t)id) : NULL; }

prts_aabb prts_cworld_box(const prts_cworld *cw, int id)
{
    cbox *b = box_at(cw, id);
    prts_aabb none = {{0, 0, 0}, {0, 0, 0}};
    return b ? b->world : none;
}

prts_entity prts_cworld_owner(const prts_cworld *cw, int id)
{
    cbox *b = box_at(cw, id);
    return b ? b->owner : PRTS_ENTITY_NONE;
}

void prts_cworld_set_enabled(prts_cworld *cw, int id, bool enabled)
{
    cbox *b = box_at(cw, id);
    if (b) b->enabled = enabled;
}

bool prts_cworld_enabled(const prts_cworld *cw, int id)
{
    cbox *b = box_at(cw, id);
    return b && b->enabled;
}

void prts_cworld_set_owner_enabled(prts_cworld *cw, prts_entity owner, bool enabled)
{
    for (size_t i = 0; i < cw->boxes.len; i++) {
        cbox *b = prts_array_at(&cw->boxes, i);
        if (b->owner == owner) b->enabled = enabled;
    }
}

void prts_cworld_set_heightfield(prts_cworld *cw, const prts_heightfield *hf) { cw->hf = hf; }

void prts_cworld_set_max_slope(prts_cworld *cw, float radians) { cw->slope_cos = cosf(radians); }

void prts_cworld_sync(prts_cworld *cw, const prts_world *w)
{
    for (size_t i = 0; i < cw->boxes.len; i++) {
        cbox *b = prts_array_at(&cw->boxes, i);
        if (b->owner == PRTS_ENTITY_NONE) continue;
        prts_dvec3 p = prts_world_pos(w, b->owner);
        float yaw = prts_world_yaw(w, b->owner);
        prts_vec3 corners[8];
        for (int c = 0; c < 8; c++) {
            prts_vec3 v = prts_v3((c & 1) ? b->local.max.x : b->local.min.x, (c & 2) ? b->local.max.y : b->local.min.y,
                                  (c & 4) ? b->local.max.z : b->local.min.z);
            v = prts_v3_rotate_y(v, yaw);
            corners[c] = prts_v3(v.x + (float)p.x, v.y + (float)p.y, v.z + (float)p.z);
        }
        prts_aabb r = {corners[0], corners[0]};
        for (int c = 1; c < 8; c++) {
            r.min = prts_v3(fminf(r.min.x, corners[c].x), fminf(r.min.y, corners[c].y), fminf(r.min.z, corners[c].z));
            r.max = prts_v3(fmaxf(r.max.x, corners[c].x), fmaxf(r.max.y, corners[c].y), fmaxf(r.max.z, corners[c].z));
        }
        b->world = r;
    }
}

static float comp(prts_vec3 v, int axis) { return axis == 0 ? v.x : (axis == 1 ? v.y : v.z); }

static void set_comp(prts_vec3 *v, int axis, float value)
{
    if (axis == 0) v->x = value;
    else if (axis == 1) v->y = value;
    else v->z = value;
}

static bool overlap_axis(prts_aabb a, prts_aabb b, int axis)
{
    return comp(a.min, axis) < comp(b.max, axis) - EPS && comp(a.max, axis) > comp(b.min, axis) + EPS;
}

/* --- bounding volume hierarchy ------------------------------------------------------- */

static bool touches(prts_aabb a, prts_aabb b)
{
    return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
}

static prts_aabb merge(prts_aabb a, prts_aabb b)
{
    return prts_aabb_make(prts_v3(fminf(a.min.x, b.min.x), fminf(a.min.y, b.min.y), fminf(a.min.z, b.min.z)),
                          prts_v3(fmaxf(a.max.x, b.max.x), fmaxf(a.max.y, b.max.y), fmaxf(a.max.z, b.max.z)));
}

static int cmp_items(const bvh_item *a, const bvh_item *b, float ca, float cb)
{
    if (ca != cb) return ca < cb ? -1 : 1;
    return a->id < b->id ? -1 : (a->id > b->id ? 1 : 0);
}
static int cmp_x(const void *a, const void *b) { return cmp_items(a, b, ((const bvh_item *)a)->c.x, ((const bvh_item *)b)->c.x); }
static int cmp_y(const void *a, const void *b) { return cmp_items(a, b, ((const bvh_item *)a)->c.y, ((const bvh_item *)b)->c.y); }
static int cmp_z(const void *a, const void *b) { return cmp_items(a, b, ((const bvh_item *)a)->c.z, ((const bvh_item *)b)->c.z); }

#define BVH_LEAF 4u

static int build_node(const prts_cworld *cw, bvh_state *st, size_t first, size_t count)
{
    bvh_item *items = (bvh_item *)st->items.data;
    prts_aabb bounds = ((cbox *)prts_array_at(&cw->boxes, (size_t)items[first].id))->world;
    prts_aabb centres = prts_aabb_make(items[first].c, items[first].c);
    for (size_t i = first + 1; i < first + count; i++) {
        bounds = merge(bounds, ((cbox *)prts_array_at(&cw->boxes, (size_t)items[i].id))->world);
        centres = merge(centres, prts_aabb_make(items[i].c, items[i].c));
    }
    int idx = (int)st->nodes.len;
    bvh_node *n = prts_array_push(&st->nodes, NULL);
    n->box = bounds;
    n->left = n->right = -1;
    if (count <= BVH_LEAF) {
        n->first = first;
        n->count = count;
        return idx;
    }
    prts_vec3 ext = prts_v3_sub(centres.max, centres.min);
    int (*cmp)(const void *, const void *) = ext.x >= ext.y && ext.x >= ext.z ? cmp_x : (ext.y >= ext.z ? cmp_y : cmp_z);
    qsort(items + first, count, sizeof(bvh_item), cmp);
    size_t half = count / 2;
    int l = build_node(cw, st, first, half);
    int r = build_node(cw, st, first + half, count - half);
    n = prts_array_at(&st->nodes, (size_t)idx); /* the array may have moved */
    n->left = l;
    n->right = r;
    return idx;
}

static void rebuild(const prts_cworld *cw)
{
    bvh_state *st = cw->bvh;
    prts_array_clear(&st->nodes);
    prts_array_clear(&st->items);
    prts_array_clear(&st->dynamic);
    for (size_t i = 0; i < cw->boxes.len; i++) {
        const cbox *b = prts_array_at(&cw->boxes, i);
        int id = (int)i;
        if (b->owner != PRTS_ENTITY_NONE) {
            prts_array_push(&st->dynamic, &id);
            continue;
        }
        bvh_item it = {id, prts_v3_scale(prts_v3_add(b->world.min, b->world.max), 0.5f)};
        prts_array_push(&st->items, &it);
    }
    if (st->items.len) build_node(cw, st, 0, st->items.len);
    st->dirty = false;
}

static int by_id(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return x < y ? -1 : (x > y ? 1 : 0);
}

/* Ids of enabled boxes touching q, ascending. The result stays valid until the next query. */
static const prts_array *query(const prts_cworld *cw, prts_aabb q)
{
    bvh_state *st = cw->bvh;
    if (st->dirty) rebuild(cw);
    prts_array_clear(&st->found);
    prts_array_clear(&st->stack);
    if (st->nodes.len) {
        int root = 0;
        prts_array_push(&st->stack, &root);
    }
    while (st->stack.len) {
        int ni = *(int *)prts_array_at(&st->stack, st->stack.len - 1);
        st->stack.len--;
        const bvh_node *n = prts_array_at(&st->nodes, (size_t)ni);
        if (!touches(n->box, q)) continue;
        if (n->count) {
            for (size_t i = n->first; i < n->first + n->count; i++) {
                int id = ((bvh_item *)prts_array_at(&st->items, i))->id;
                const cbox *b = prts_array_at(&cw->boxes, (size_t)id);
                if (b->enabled && touches(b->world, q)) prts_array_push(&st->found, &id);
            }
        } else {
            prts_array_push(&st->stack, &n->left);
            prts_array_push(&st->stack, &n->right);
        }
    }
    for (size_t i = 0; i < st->dynamic.len; i++) {
        int id = *(int *)prts_array_at(&st->dynamic, i);
        const cbox *b = prts_array_at(&cw->boxes, (size_t)id);
        if (b->enabled && touches(b->world, q)) prts_array_push(&st->found, &id);
    }
    qsort(st->found.data, st->found.len, sizeof(int), by_id);
    return &st->found;
}

/* The bounds of the segment origin + dir * [0, max_dist] (dir normalized). */
static prts_aabb segment_bounds(prts_vec3 o, prts_vec3 dir, float max_dist)
{
    prts_vec3 e = prts_v3_add(o, prts_v3_scale(dir, max_dist));
    return prts_aabb_make(o, e);
}

/* Moves box along one axis by d, stopping at the first box in the way. */
static float move_axis(const prts_cworld *cw, prts_aabb *box, int axis, float d, int *hit)
{
    *hit = -1;
    if (d == 0.0f) return 0.0f;
    int o1 = (axis + 1) % 3, o2 = (axis + 2) % 3;
    prts_aabb swept = *box;
    set_comp(d > 0.0f ? &swept.max : &swept.min, axis, comp(d > 0.0f ? swept.max : swept.min, axis) + d);
    const prts_array *found = query(cw, swept);
    for (size_t k = 0; k < found->len; k++) {
        size_t i = (size_t)*(const int *)prts_array_at(found, k);
        cbox *b = prts_array_at(&cw->boxes, i);
        if (!overlap_axis(*box, b->world, o1) || !overlap_axis(*box, b->world, o2)) continue;
        if (d > 0.0f && comp(b->world.min, axis) >= comp(box->max, axis) - EPS) {
            float allowed = comp(b->world.min, axis) - comp(box->max, axis);
            if (allowed < d) {
                d = fmaxf(allowed, 0.0f);
                *hit = (int)i;
            }
        } else if (d < 0.0f && comp(b->world.max, axis) <= comp(box->min, axis) + EPS) {
            float allowed = comp(b->world.max, axis) - comp(box->min, axis);
            if (allowed > d) {
                d = fminf(allowed, 0.0f);
                *hit = (int)i;
            }
        }
    }
    set_comp(&box->min, axis, comp(box->min, axis) + d);
    set_comp(&box->max, axis, comp(box->max, axis) + d);
    return d;
}

/* Keeps the box on top of the terrain. Returns true if it was pushed up (standing). */
static bool clamp_terrain(const prts_cworld *cw, prts_aabb *box)
{
    if (!cw->hf) return false;
    float cx = (box->min.x + box->max.x) * 0.5f, cz = (box->min.z + box->max.z) * 0.5f;
    float h = prts_heightfield_sample(cw->hf, cx, cz);
    if (box->min.y > h + EPS) return false;
    float lift = h - box->min.y;
    box->min.y += lift;
    box->max.y += lift;
    return true;
}

static float horiz_len(prts_vec3 v) { return sqrtf(v.x * v.x + v.z * v.z); }

/* Spec: slopes up to the limit can be walked up. On steeper terrain the part of the move that
 * goes uphill is removed, so the mover slides along the slope instead of climbing it. */
static bool limit_slope(const prts_cworld *cw, prts_aabb start, prts_aabb *box)
{
    if (!cw->hf) return false;
    float sx = (start.min.x + start.max.x) * 0.5f, sz = (start.min.z + start.max.z) * 0.5f;
    if (start.min.y > prts_heightfield_sample(cw->hf, sx, sz) + 0.05f) return false; /* not on the terrain */
    float dx = box->min.x - start.min.x, dz = box->min.z - start.min.z;
    if (dx * dx + dz * dz < 1e-12f) return false;
    prts_vec3 n = prts_heightfield_normal(cw->hf, sx + dx, sz + dz);
    if (n.y >= cw->slope_cos) return false;
    float gl = sqrtf(n.x * n.x + n.z * n.z);
    float ux = -n.x / gl, uz = -n.z / gl; /* uphill, along the ground */
    float up = dx * ux + dz * uz;
    if (up <= 0.0f) return false; /* across or down the slope is fine */
    float cut_x = -up * ux, cut_z = -up * uz;
    box->min.x += cut_x;
    box->max.x += cut_x;
    box->min.z += cut_z;
    box->max.z += cut_z;
    return true;
}

prts_vec3 prts_cworld_move(const prts_cworld *cw, prts_aabb *box, prts_vec3 delta, float step_height,
                           prts_move_result *result)
{
    prts_move_result res = {false, -1, false, false};
    prts_aabb start = *box;
    int hit = -1;
    prts_aabb flat = *box;
    float mx = move_axis(cw, &flat, 0, delta.x, &hit);
    bool blocked = hit >= 0;
    float mz = move_axis(cw, &flat, 2, delta.z, &hit);
    blocked = blocked || hit >= 0;
    prts_aabb best = flat;
    if (blocked && step_height > 0.0f) {
        prts_aabb up = *box;
        float rise = move_axis(cw, &up, 1, step_height, &hit);
        float ux = move_axis(cw, &up, 0, delta.x, &hit);
        float uz = move_axis(cw, &up, 2, delta.z, &hit);
        move_axis(cw, &up, 1, -rise, &hit);
        if (horiz_len(prts_v3(ux, 0, uz)) > horiz_len(prts_v3(mx, 0, mz)) + EPS) {
            best = up;
            res.stepped = true;
        }
    }
    res.hit_wall = blocked && !res.stepped;
    *box = best;
    if (limit_slope(cw, start, box)) res.hit_wall = true;
    float my = move_axis(cw, box, 1, delta.y, &hit);
    if (delta.y < 0.0f && my > delta.y + EPS) {
        res.on_ground = true;
        res.ground_box = hit;
    }
    if (clamp_terrain(cw, box)) {
        res.on_ground = true;
        res.ground_box = -1;
    }
    if (res.stepped && !res.on_ground) res.on_ground = true;
    if (result) *result = res;
    return prts_v3_sub(box->min, start.min);
}

bool prts_cworld_overlaps(const prts_cworld *cw, prts_aabb box)
{
    const prts_array *found = query(cw, box);
    for (size_t k = 0; k < found->len; k++) {
        cbox *b = prts_array_at(&cw->boxes, (size_t)*(const int *)prts_array_at(found, k));
        if (prts_aabb_overlaps(box, b->world)) return true;
    }
    return false;
}

static bool ray_box(prts_vec3 o, prts_vec3 d, prts_aabb b, float *t_hit)
{
    float tmin = 0.0f, tmax = 1e30f;
    for (int a = 0; a < 3; a++) {
        float oa = comp(o, a), da = comp(d, a), lo = comp(b.min, a), hi = comp(b.max, a);
        if (fabsf(da) < 1e-8f) {
            if (oa < lo || oa > hi) return false;
            continue;
        }
        float t1 = (lo - oa) / da, t2 = (hi - oa) / da;
        if (t1 > t2) {
            float t = t1;
            t1 = t2;
            t2 = t;
        }
        tmin = fmaxf(tmin, t1);
        tmax = fminf(tmax, t2);
        if (tmin > tmax) return false;
    }
    *t_hit = tmin;
    return true;
}

int prts_cworld_raycast(const prts_cworld *cw, prts_vec3 origin, prts_vec3 dir, float max_dist, float *out_dist)
{
    dir = prts_v3_normalize(dir);
    int best = -1;
    float best_t = max_dist;
    const prts_array *found = query(cw, segment_bounds(origin, dir, max_dist));
    for (size_t k = 0; k < found->len; k++) {
        int i = *(const int *)prts_array_at(found, k);
        cbox *b = prts_array_at(&cw->boxes, (size_t)i);
        float t = 0.0f;
        if (ray_box(origin, dir, b->world, &t) && t <= best_t) {
            best_t = t;
            best = i;
        }
    }
    if (cw->hf) {
        for (float t = 0.0f; t <= best_t; t += 0.25f) {
            prts_vec3 p = prts_v3_add(origin, prts_v3_scale(dir, t));
            if (p.y <= prts_heightfield_sample(cw->hf, p.x, p.z)) {
                best_t = t;
                best = -2;
                break;
            }
        }
    }
    if (out_dist) *out_dist = best_t;
    return best;
}

bool prts_cworld_occluded(const prts_cworld *cw, prts_vec3 origin, prts_vec3 dir, float max_dist)
{
    dir = prts_v3_normalize(dir);
    const prts_array *found = query(cw, segment_bounds(origin, dir, max_dist));
    for (size_t k = 0; k < found->len; k++) {
        cbox *b = prts_array_at(&cw->boxes, (size_t)*(const int *)prts_array_at(found, k));
        float t = 0.0f;
        if (ray_box(origin, dir, b->world, &t) && t <= max_dist) return true;
    }
    if (cw->hf) {
        for (float t = 1.0f; t <= max_dist; t += 2.0f) {
            prts_vec3 p = prts_v3_add(origin, prts_v3_scale(dir, t));
            if (p.y < prts_heightfield_sample(cw->hf, p.x, p.z) - 0.05f) return true;
        }
    }
    return false;
}
