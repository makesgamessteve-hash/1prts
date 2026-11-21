/* 1stPRTS engine: agents, the units that walk navigation paths. MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_nav.h"
#include "prts/prts_rng.h"
#include "prts/prts_serial.h"

#define ARRIVE 0.3f       /* metres: close enough to a path point */
#define PUSH 0.5f         /* share of an overlap removed per tick */
#define HASH_CELL 4.0f    /* spatial hash for separation */
#define MAX_PATH 256

typedef struct agent {
    prts_entity e;
    float speed, radius;
    float *path; /* x, z pairs */
    int count, next;
    bool moving;
    float x, z; /* position this tick (filled before moving) */
} agent;

struct prts_agents {
    agent *a;
    size_t len, cap;
    int *bucket_head, *bucket_next;
    size_t buckets;
};

prts_agents *prts_agents_create(void) { return prts_calloc(1, sizeof(prts_agents)); }

void prts_agents_destroy(prts_agents *s)
{
    if (!s) return;
    for (size_t i = 0; i < s->len; i++) prts_free(s->a[i].path);
    prts_free(s->a);
    prts_free(s->bucket_head);
    prts_free(s->bucket_next);
    prts_free(s);
}

size_t prts_agents_count(const prts_agents *s) { return s->len; }

/* Index of e, or where it would go (agents are sorted by entity). */
static size_t find(const prts_agents *s, prts_entity e, bool *found)
{
    size_t lo = 0, hi = s->len;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (s->a[mid].e < e) lo = mid + 1;
        else hi = mid;
    }
    *found = lo < s->len && s->a[lo].e == e;
    return lo;
}

static agent *get(const prts_agents *s, prts_entity e)
{
    bool found;
    size_t i = find(s, e, &found);
    return found ? &s->a[i] : NULL;
}

bool prts_agents_add(prts_agents *s, prts_entity e, float speed, float radius)
{
    if (e == PRTS_ENTITY_NONE || !(speed >= 0.0f) || !(radius > 0.0f)) return false;
    bool found;
    size_t i = find(s, e, &found);
    if (found) {
        s->a[i].speed = speed;
        s->a[i].radius = radius;
        return true;
    }
    if (s->len == s->cap) {
        s->cap = s->cap ? s->cap * 2 : 64;
        s->a = prts_realloc(s->a, s->cap * sizeof(agent));
    }
    memmove(&s->a[i + 1], &s->a[i], (s->len - i) * sizeof(agent));
    memset(&s->a[i], 0, sizeof(agent));
    s->a[i].e = e;
    s->a[i].speed = speed;
    s->a[i].radius = radius;
    s->len++;
    return true;
}

static void remove_at(prts_agents *s, size_t i)
{
    prts_free(s->a[i].path);
    memmove(&s->a[i], &s->a[i + 1], (s->len - i - 1) * sizeof(agent));
    s->len--;
}

bool prts_agents_remove(prts_agents *s, prts_entity e)
{
    bool found;
    size_t i = find(s, e, &found);
    if (found) remove_at(s, i);
    return found;
}

bool prts_agents_info(const prts_agents *s, prts_entity e, prts_agent_info *out)
{
    const agent *g = get(s, e);
    if (!g) return false;
    out->speed = g->speed;
    out->radius = g->radius;
    out->moving = g->moving;
    out->left = g->moving ? g->count - g->next : 0;
    out->tx = g->moving ? g->path[2 * (g->count - 1)] : g->x;
    out->tz = g->moving ? g->path[2 * (g->count - 1) + 1] : g->z;
    return true;
}

bool prts_agents_set_speed(prts_agents *s, prts_entity e, float speed)
{
    agent *g = get(s, e);
    if (!g || !(speed >= 0.0f)) return false;
    g->speed = speed;
    return true;
}

bool prts_agents_follow(prts_agents *s, prts_entity e, const float *xz, int count)
{
    agent *g = get(s, e);
    if (!g) return false;
    if (count > MAX_PATH) count = MAX_PATH;
    prts_free(g->path);
    g->path = NULL;
    g->count = g->next = 0;
    g->moving = count > 0;
    if (count > 0) {
        g->path = prts_malloc((size_t)count * 2 * sizeof(float));
        memcpy(g->path, xz, (size_t)count * 2 * sizeof(float));
        g->count = count;
    }
    return true;
}

static int bucket_of(float x, float z, size_t buckets)
{
    int bx = (int)floorf(x / HASH_CELL), bz = (int)floorf(z / HASH_CELL);
    uint32_t h = (uint32_t)bx * 73856093u ^ (uint32_t)bz * 19349663u;
    return (int)(h % (uint32_t)buckets);
}

/* Moves (x, z) by (dx, dz) as far as the grid allows: whole, along x, along z, or not at all. */
static void slide(const prts_nav *nav, float *x, float *z, float dx, float dz)
{
    if (!nav || prts_nav_walkable(nav, *x + dx, *z + dz)) {
        *x += dx;
        *z += dz;
    } else if (prts_nav_walkable(nav, *x + dx, *z)) {
        *x += dx;
    } else if (prts_nav_walkable(nav, *x, *z + dz)) {
        *z += dz;
    }
}

int prts_agents_tick(prts_agents *s, prts_world *w, const prts_nav *nav, float dt, prts_entity *arrived, int max)
{
    /* agents of despawned entities go */
    for (size_t i = s->len; i-- > 0;) {
        if (!prts_world_alive(w, s->a[i].e)) remove_at(s, i);
    }
    if (s->len == 0) return 0;
    for (size_t i = 0; i < s->len; i++) {
        prts_dvec3 p = prts_world_pos(w, s->a[i].e);
        s->a[i].x = (float)p.x;
        s->a[i].z = (float)p.z;
    }
    /* the spatial hash of this tick's positions; buckets list agents in entity order */
    size_t buckets = s->len * 2 + 1;
    if (buckets > s->buckets) {
        s->bucket_head = prts_realloc(s->bucket_head, buckets * sizeof(int));
        s->buckets = buckets;
    }
    s->bucket_next = prts_realloc(s->bucket_next, s->len * sizeof(int));
    for (size_t b = 0; b < buckets; b++) s->bucket_head[b] = -1;
    for (size_t i = s->len; i-- > 0;) {
        int b = bucket_of(s->a[i].x, s->a[i].z, buckets);
        s->bucket_next[i] = s->bucket_head[b];
        s->bucket_head[b] = (int)i;
    }
    int done = 0;
    for (size_t i = 0; i < s->len; i++) {
        agent *g = &s->a[i];
        float vx = 0.0f, vz = 0.0f;
        if (g->moving) {
            float step = g->speed * dt;
            for (;;) {
                float tx = g->path[2 * g->next], tz = g->path[2 * g->next + 1];
                float dx = tx - g->x, dz = tz - g->z, d = sqrtf(dx * dx + dz * dz);
                bool last = g->next == g->count - 1;
                if (d <= fmaxf(ARRIVE, last ? step : ARRIVE)) {
                    if (last) {
                        g->moving = false;
                        if (done < max) arrived[done++] = g->e;
                        vx = dx / dt;
                        vz = dz / dt;
                        break;
                    }
                    g->next++;
                    continue;
                }
                vx = dx / d * g->speed;
                vz = dz / d * g->speed;
                break;
            }
        }
        /* separation: push out of overlapping neighbours (the 3x3 hash cells around) */
        float px = 0.0f, pz = 0.0f;
        int cx = (int)floorf(g->x / HASH_CELL), cz = (int)floorf(g->z / HASH_CELL);
        int seen[9], nseen = 0;
        for (int oz = -1; oz <= 1; oz++) {
            for (int ox = -1; ox <= 1; ox++) {
                int b = bucket_of(((float)(cx + ox) + 0.5f) * HASH_CELL, ((float)(cz + oz) + 0.5f) * HASH_CELL, buckets);
                bool dup = false;
                for (int k = 0; k < nseen; k++) dup = dup || seen[k] == b;
                if (dup) continue;
                seen[nseen++] = b;
                for (int j = s->bucket_head[b]; j != -1; j = s->bucket_next[j]) {
                    if ((size_t)j == i) continue;
                    const agent *o = &s->a[j];
                    float dx = g->x - o->x, dz = g->z - o->z, d2 = dx * dx + dz * dz;
                    float min = g->radius + o->radius;
                    if (d2 >= min * min) continue;
                    float d = sqrtf(d2);
                    if (d < 1e-4f) { /* exactly on top: part by entity order */
                        dx = (g->e < o->e) ? -1.0f : 1.0f;
                        dz = 0.0f;
                        d = 1.0f;
                    }
                    float overlap = (min - (d < 1e-4f ? 0.0f : sqrtf(d2))) * PUSH;
                    px += dx / d * overlap;
                    pz += dz / d * overlap;
                }
            }
        }
        float nx = g->x, nz = g->z;
        slide(nav, &nx, &nz, vx * dt + px, vz * dt + pz);
        float h = 0.0f;
        prts_dvec3 old = prts_world_pos(w, g->e);
        double y = nav && prts_nav_height(nav, nx, nz, &h) ? (double)h : old.y;
        float yaw = prts_world_yaw(w, g->e);
        if (vx * vx + vz * vz > 1e-6f) yaw = atan2f(-vx, -vz); /* yaw 0 looks along -z */
        prts_world_set_local(w, g->e, prts_dv3(nx, y, nz), yaw);
        prts_world_set_velocity(w, g->e, prts_v3(vx, 0.0f, vz));
    }
    return done;
}

typedef struct near_hit {
    float d2;
    prts_entity e;
} near_hit;

int prts_agents_near(const prts_agents *s, const prts_world *w, float x, float z, float r, prts_entity *out, int max)
{
    if (max <= 0 || s->len == 0) return 0;
    near_hit *hits = prts_malloc(s->len * sizeof(near_hit));
    int n = 0;
    for (size_t i = 0; i < s->len; i++) {
        if (!prts_world_alive(w, s->a[i].e)) continue;
        prts_dvec3 p = prts_world_pos(w, s->a[i].e);
        float dx = (float)p.x - x, dz = (float)p.z - z, d2 = dx * dx + dz * dz;
        if (d2 <= r * r) hits[n++] = (near_hit){d2, s->a[i].e};
    }
    /* insertion sort: nearest first, ties by entity (agents are already in entity order) */
    for (int i = 1; i < n; i++) {
        near_hit v = hits[i];
        int j = i - 1;
        while (j >= 0 && (hits[j].d2 > v.d2 || (hits[j].d2 == v.d2 && hits[j].e > v.e))) {
            hits[j + 1] = hits[j];
            j--;
        }
        hits[j + 1] = v;
    }
    int count = n < max ? n : max;
    for (int i = 0; i < count; i++) out[i] = hits[i].e;
    prts_free(hits);
    return count;
}

void prts_agents_hash(const prts_agents *s, prts_hasher *h)
{
    prts_hasher_u64(h, s->len);
    for (size_t i = 0; i < s->len; i++) {
        const agent *g = &s->a[i];
        prts_hasher_u64(h, g->e);
        prts_hasher_f64(h, g->speed);
        prts_hasher_f64(h, g->radius);
        prts_hasher_u64(h, (uint64_t)g->moving | ((uint64_t)(uint32_t)g->next << 8) | ((uint64_t)(uint32_t)g->count << 32));
        if (g->count) prts_hasher_bytes(h, g->path, (size_t)g->count * 2 * sizeof(float));
    }
}

void prts_agents_write(const prts_agents *s, prts_writer *w)
{
    prts_w_u32(w, (uint32_t)s->len);
    for (size_t i = 0; i < s->len; i++) {
        const agent *g = &s->a[i];
        prts_w_u64(w, g->e);
        prts_w_f32(w, g->speed);
        prts_w_f32(w, g->radius);
        prts_w_u8(w, g->moving ? 1u : 0u);
        prts_w_u32(w, (uint32_t)g->next);
        prts_w_u32(w, (uint32_t)g->count);
        for (int k = 0; k < g->count * 2; k++) prts_w_f32(w, g->path[k]);
    }
}

bool prts_agents_read(prts_agents *s, prts_reader *r)
{
    prts_agents *in = prts_agents_create();
    uint32_t count = prts_r_u32(r);
    for (uint32_t i = 0; i < count && !r->failed; i++) {
        prts_entity e = prts_r_u64(r);
        float speed = prts_r_f32(r), radius = prts_r_f32(r);
        bool moving = prts_r_u8(r) != 0;
        uint32_t next = prts_r_u32(r), len = prts_r_u32(r);
        if (r->failed || len > MAX_PATH || next > len || !prts_agents_add(in, e, speed, radius)) {
            r->failed = true;
            break;
        }
        agent *g = get(in, e);
        g->count = (int)len;
        g->next = (int)next;
        g->moving = moving && len > 0 && next < len;
        if (len) {
            g->path = prts_malloc((size_t)len * 2 * sizeof(float));
            for (uint32_t k = 0; k < len * 2; k++) g->path[k] = prts_r_f32(r);
        }
    }
    if (r->failed) {
        prts_agents_destroy(in);
        return false;
    }
    for (size_t i = 0; i < s->len; i++) prts_free(s->a[i].path);
    prts_free(s->a);
    s->a = in->a;
    s->len = in->len;
    s->cap = in->cap;
    in->a = NULL;
    in->len = in->cap = 0;
    prts_agents_destroy(in);
    return true;
}
