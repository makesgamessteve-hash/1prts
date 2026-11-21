/* 1stPRTS engine: collision world (boxes + terrain height grid). MIT License. */
#include "prts/prts_collide.h"

#include <math.h>
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

struct prts_cworld {
    prts_array boxes; /* cbox */
    const prts_heightfield *hf;
};

prts_cworld *prts_cworld_create(void)
{
    prts_cworld *cw = prts_calloc(1, sizeof(*cw));
    prts_array_init(&cw->boxes, sizeof(cbox));
    return cw;
}

void prts_cworld_destroy(prts_cworld *cw)
{
    if (!cw) return;
    prts_array_free(&cw->boxes);
    prts_free(cw);
}

int prts_cworld_add_box(prts_cworld *cw, prts_aabb box, prts_entity owner)
{
    cbox b = {box, box, owner, true};
    prts_array_push(&cw->boxes, &b);
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

/* Moves box along one axis by d, stopping at the first box in the way. */
static float move_axis(const prts_cworld *cw, prts_aabb *box, int axis, float d, int *hit)
{
    *hit = -1;
    if (d == 0.0f) return 0.0f;
    int o1 = (axis + 1) % 3, o2 = (axis + 2) % 3;
    for (size_t i = 0; i < cw->boxes.len; i++) {
        cbox *b = prts_array_at(&cw->boxes, i);
        if (!b->enabled || !overlap_axis(*box, b->world, o1) || !overlap_axis(*box, b->world, o2)) continue;
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
    for (size_t i = 0; i < cw->boxes.len; i++) {
        cbox *b = prts_array_at(&cw->boxes, i);
        if (b->enabled && prts_aabb_overlaps(box, b->world)) return true;
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
    for (size_t i = 0; i < cw->boxes.len; i++) {
        cbox *b = prts_array_at(&cw->boxes, i);
        float t;
        if (b->enabled && ray_box(origin, dir, b->world, &t) && t <= best_t) {
            best_t = t;
            best = (int)i;
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
    for (size_t i = 0; i < cw->boxes.len; i++) {
        cbox *b = prts_array_at(&cw->boxes, i);
        float t;
        if (b->enabled && ray_box(origin, dir, b->world, &t) && t <= max_dist) return true;
    }
    if (cw->hf) {
        for (float t = 1.0f; t <= max_dist; t += 2.0f) {
            prts_vec3 p = prts_v3_add(origin, prts_v3_scale(dir, t));
            if (p.y < prts_heightfield_sample(cw->hf, p.x, p.z) - 0.05f) return true;
        }
    }
    return false;
}
