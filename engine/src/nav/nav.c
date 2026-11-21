/* 1stPRTS engine: the navigation grid and A* paths. MIT License. */
#include "prts/prts_nav.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "prts/prts_rng.h"
#include "prts/prts_serial.h"

#define CLIMB 1.0f       /* metres of rise per metre: 45 degrees */
#define BODY_LOW 0.45f   /* the body's box starts this far above the ground (small bumps) */
#define BODY_HIGH 1.8f
#define PROBE_UP 400.0f  /* the ground is the highest surface below this height */
#define SNAP_RINGS 12    /* how far (in cells) a blocked start or goal is moved */
#define DIAG 1.41421356f

struct prts_nav {
    int cols, rows;
    float min_x, min_z, cell;
    float *ground;     /* ground height per cell */
    uint8_t *solid;    /* 1: nothing fits there (from the world) */
    uint16_t *blocks;  /* block counts from buildings */
    uint64_t block_hash; /* running hash of every block change, for the state hash */
    /* A* scratch, reused between paths: a cell's g and parent are valid when its stamp is the
     * current search's */
    float *g;
    int *parent;
    uint32_t *stamp;
    uint32_t search;
    struct heap_node *heap;
    int heap_cap;
};

static int idx(const prts_nav *n, int cx, int cz) { return cz * n->cols + cx; }

static bool cell_of(const prts_nav *n, float x, float z, int *cx, int *cz)
{
    float fx = (x - n->min_x) / n->cell, fz = (z - n->min_z) / n->cell;
    if (!(fx >= 0.0f && fz >= 0.0f)) return false;
    int ix = (int)fx, iz = (int)fz;
    if (ix >= n->cols || iz >= n->rows) return false;
    *cx = ix;
    *cz = iz;
    return true;
}

static float centre_x(const prts_nav *n, int cx) { return n->min_x + ((float)cx + 0.5f) * n->cell; }
static float centre_z(const prts_nav *n, int cz) { return n->min_z + ((float)cz + 0.5f) * n->cell; }

static bool open_cell(const prts_nav *n, int cx, int cz)
{
    if (cx < 0 || cz < 0 || cx >= n->cols || cz >= n->rows) return false;
    int i = idx(n, cx, cz);
    return !n->solid[i] && n->blocks[i] == 0;
}

/* Whether a step from cell a to cell b (both open) climbs no more than the slope allows. */
static bool joined(const prts_nav *n, int a, int b, float dist)
{
    return fabsf(n->ground[a] - n->ground[b]) <= CLIMB * dist + 1e-4f;
}

/* Reads the world under one cell: its ground and whether a body fits there. */
static void sample(prts_nav *n, const prts_cworld *cw, int cx, int cz)
{
    int i = idx(n, cx, cz);
    float r = fminf(0.4f, n->cell * 0.45f);
    /* the ground is the lowest surface under the cell: units walk at ground level, not on
     * roofs and wall tops (those are for the commander and, later, garrisons) */
    float x = centre_x(n, cx), z = centre_z(n, cz), h = 0.0f, y = PROBE_UP, found = 0.0f;
    bool any = false;
    for (int k = 0; k < 16 && prts_cworld_ground(cw, prts_v3(x, y, z), y + PROBE_UP, &found); k++) {
        h = found;
        any = true;
        y = found - 0.05f;
    }
    n->ground[i] = h;
    if (!any) {
        n->solid[i] = 1;
        return;
    }
    prts_aabb body = {prts_v3(x - r, h + BODY_LOW, z - r), prts_v3(x + r, h + BODY_HIGH, z + r)};
    n->solid[i] = prts_cworld_overlaps(cw, body) ? 1 : 0;
}

prts_nav *prts_nav_build(const prts_cworld *cw, float min_x, float min_z, float max_x, float max_z, float cell)
{
    if (!(cell > 0.05f) || !(max_x > min_x) || !(max_z > min_z)) return NULL;
    int cols = (int)ceilf((max_x - min_x) / cell), rows = (int)ceilf((max_z - min_z) / cell);
    if ((long long)cols * rows > 16 * 1024 * 1024) return NULL;
    prts_nav *n = prts_calloc(1, sizeof(*n));
    n->cols = cols;
    n->rows = rows;
    n->min_x = min_x;
    n->min_z = min_z;
    n->cell = cell;
    size_t count = (size_t)cols * (size_t)rows;
    n->ground = prts_calloc(count, sizeof(float));
    n->solid = prts_calloc(count, 1);
    n->blocks = prts_calloc(count, sizeof(uint16_t));
    n->g = prts_malloc(count * sizeof(float));
    n->parent = prts_malloc(count * sizeof(int));
    n->stamp = prts_calloc(count, sizeof(uint32_t));
    n->block_hash = 0x9e3779b97f4a7c15ull;
    for (int cz = 0; cz < rows; cz++) {
        for (int cx = 0; cx < cols; cx++) sample(n, cw, cx, cz);
    }
    return n;
}

void prts_nav_destroy(prts_nav *n)
{
    if (!n) return;
    prts_free(n->ground);
    prts_free(n->solid);
    prts_free(n->blocks);
    prts_free(n->g);
    prts_free(n->parent);
    prts_free(n->stamp);
    prts_free(n->heap);
    prts_free(n);
}

int prts_nav_cols(const prts_nav *n) { return n->cols; }
int prts_nav_rows(const prts_nav *n) { return n->rows; }
float prts_nav_cell(const prts_nav *n) { return n->cell; }

bool prts_nav_walkable(const prts_nav *n, float x, float z)
{
    int cx, cz;
    return cell_of(n, x, z, &cx, &cz) && open_cell(n, cx, cz);
}

bool prts_nav_height(const prts_nav *n, float x, float z, float *h)
{
    int cx, cz;
    if (!cell_of(n, x, z, &cx, &cz) || n->solid[idx(n, cx, cz)]) return false;
    *h = n->ground[idx(n, cx, cz)];
    return true;
}

int prts_nav_refresh(prts_nav *n, const prts_cworld *cw, float min_x, float min_z, float max_x, float max_z)
{
    int changed = 0;
    for (int cz = 0; cz < n->rows; cz++) {
        float z = centre_z(n, cz);
        if (z < min_z || z > max_z) continue;
        for (int cx = 0; cx < n->cols; cx++) {
            float x = centre_x(n, cx);
            if (x < min_x || x > max_x) continue;
            sample(n, cw, cx, cz);
            changed++;
        }
    }
    prts_hasher h = {n->block_hash};
    const float args[4] = {min_x, min_z, max_x, max_z};
    prts_hasher_bytes(&h, args, sizeof(args));
    n->block_hash = h.h;
    return changed;
}

int prts_nav_block(prts_nav *n, float min_x, float min_z, float max_x, float max_z, int delta)
{
    int changed = 0;
    prts_hasher h = {n->block_hash};
    const float args[5] = {min_x, min_z, max_x, max_z, (float)delta};
    prts_hasher_bytes(&h, args, sizeof(args));
    n->block_hash = h.h;
    for (int cz = 0; cz < n->rows; cz++) {
        float z = centre_z(n, cz);
        if (z < min_z || z > max_z) continue;
        for (int cx = 0; cx < n->cols; cx++) {
            float x = centre_x(n, cx);
            if (x < min_x || x > max_x) continue;
            uint16_t *b = &n->blocks[idx(n, cx, cz)];
            int v = (int)*b + delta;
            *b = (uint16_t)(v < 0 ? 0 : (v > 65535 ? 65535 : v));
            changed++;
        }
    }
    return changed;
}

int prts_nav_block_below(prts_nav *n, float height, int delta)
{
    int changed = 0;
    prts_hasher h = {n->block_hash};
    const float args[3] = {height, (float)delta, -1.0f}; /* -1: not a rectangle */
    prts_hasher_bytes(&h, args, sizeof(args));
    n->block_hash = h.h;
    for (int i = 0; i < n->cols * n->rows; i++) {
        if (n->ground[i] >= height) continue;
        int v = (int)n->blocks[i] + delta;
        n->blocks[i] = (uint16_t)(v < 0 ? 0 : (v > 65535 ? 65535 : v));
        changed++;
    }
    return changed;
}

/* The nearest open cell to (cx, cz) within SNAP_RINGS, ring by ring, in a fixed order. */
static bool snap(const prts_nav *n, int *cx, int *cz)
{
    if (open_cell(n, *cx, *cz)) return true;
    for (int ring = 1; ring <= SNAP_RINGS; ring++) {
        int best = -1;
        float bd = 1e30f;
        for (int dz = -ring; dz <= ring; dz++) {
            for (int dx = -ring; dx <= ring; dx++) {
                if (abs(dx) != ring && abs(dz) != ring) continue;
                int x = *cx + dx, z = *cz + dz;
                float d = (float)(dx * dx + dz * dz);
                if (open_cell(n, x, z) && d < bd) {
                    bd = d;
                    best = idx(n, x, z);
                }
            }
        }
        if (best >= 0) {
            *cx = best % n->cols;
            *cz = best / n->cols;
            return true;
        }
    }
    return false;
}

/* Sampled walk along the segment: every sample on an open cell, each step joined. */
static bool clear_cells(const prts_nav *n, float x0, float z0, float x1, float z1)
{
    float dx = x1 - x0, dz = z1 - z0;
    float len = sqrtf(dx * dx + dz * dz);
    int steps = (int)ceilf(len / (n->cell * 0.25f));
    int prev = -1;
    for (int s = 0; s <= steps; s++) {
        float t = steps ? (float)s / (float)steps : 0.0f;
        int cx, cz;
        if (!cell_of(n, x0 + dx * t, z0 + dz * t, &cx, &cz) || !open_cell(n, cx, cz)) return false;
        int i = idx(n, cx, cz);
        if (prev >= 0 && prev != i && !joined(n, prev, i, n->cell * DIAG)) return false;
        prev = i;
    }
    return true;
}

bool prts_nav_clear_line(const prts_nav *n, float x0, float z0, float x1, float z1)
{
    return clear_cells(n, x0, z0, x1, z1);
}

/* --- A* ---------------------------------------------------------------------------------- */

typedef struct heap_node {
    float f;
    int i;
} heap_node;

static bool before(heap_node a, heap_node b) { return a.f < b.f || (a.f == b.f && a.i < b.i); }

static void heap_push(prts_nav *n, int *len, heap_node v)
{
    if (*len >= n->heap_cap) {
        n->heap_cap = n->heap_cap ? n->heap_cap * 2 : 1024;
        n->heap = prts_realloc(n->heap, (size_t)n->heap_cap * sizeof(heap_node));
    }
    heap_node *h = n->heap;
    int k = (*len)++;
    h[k] = v;
    while (k > 0) {
        int p = (k - 1) / 2;
        if (!before(h[k], h[p])) break;
        heap_node t = h[k];
        h[k] = h[p];
        h[p] = t;
        k = p;
    }
}

static heap_node heap_pop(heap_node *h, int *len)
{
    heap_node top = h[0];
    h[0] = h[--(*len)];
    int k = 0;
    for (;;) {
        int l = 2 * k + 1, r = l + 1, m = k;
        if (l < *len && before(h[l], h[m])) m = l;
        if (r < *len && before(h[r], h[m])) m = r;
        if (m == k) break;
        heap_node t = h[k];
        h[k] = h[m];
        h[m] = t;
        k = m;
    }
    return top;
}

static float octile(int ax, int az, int bx, int bz)
{
    float dx = (float)abs(ax - bx), dz = (float)abs(az - bz);
    return (dx + dz) + (DIAG - 2.0f) * fminf(dx, dz);
}

int prts_nav_path(const prts_nav *cn, float x0, float z0, float x1, float z1, float *out, int max)
{
    prts_nav *n = (prts_nav *)cn; /* the A* scratch is not part of the grid's value */
    int sx, sz, gx, gz;
    if (max < 1 || !cell_of(n, x0, z0, &sx, &sz) || !cell_of(n, x1, z1, &gx, &gz)) return -1;
    bool goal_exact = open_cell(n, gx, gz);
    if (!snap(n, &sx, &sz) || !snap(n, &gx, &gz)) return -1;
    int start = idx(n, sx, sz), goal = idx(n, gx, gz);
    if (n->search >= 0xfffffff0u) { /* the stamps would wrap: start over */
        memset(n->stamp, 0, (size_t)n->cols * (size_t)n->rows * sizeof(uint32_t));
        n->search = 0;
    }
    n->search += 2; /* two stamps per search: seen (open) and closed */
    const uint32_t open_mark = n->search, closed_mark = n->search + 1u;
    float *g = n->g;
    int *parent = n->parent;
    uint32_t *st = n->stamp;
    int len = 0, expanded = 0;
    g[start] = 0.0f;
    parent[start] = -1;
    st[start] = open_mark;
    heap_push(n, &len, (heap_node){octile(sx, sz, gx, gz), start});
    bool found = false;
    static const int DX[8] = {1, -1, 0, 0, 1, 1, -1, -1}, DZ[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    while (len > 0 && expanded < PRTS_NAV_MAX_EXPAND) {
        heap_node cur = heap_pop(n->heap, &len);
        if (st[cur.i] == closed_mark) continue;
        st[cur.i] = closed_mark;
        expanded++;
        if (cur.i == goal) {
            found = true;
            break;
        }
        int cx = cur.i % n->cols, cz = cur.i / n->cols;
        for (int k = 0; k < 8; k++) {
            int nx = cx + DX[k], nz = cz + DZ[k];
            if (!open_cell(n, nx, nz)) continue;
            int j = idx(n, nx, nz);
            float step = k < 4 ? 1.0f : DIAG;
            if (k >= 4) { /* no cutting corners */
                if (!open_cell(n, cx + DX[k], cz) || !open_cell(n, cx, cz + DZ[k])) continue;
            }
            if (st[j] == closed_mark || !joined(n, cur.i, j, step * n->cell)) continue;
            float ng = g[cur.i] + step;
            if (st[j] != open_mark || ng < g[j]) {
                st[j] = open_mark;
                g[j] = ng;
                parent[j] = cur.i;
                heap_push(n, &len, (heap_node){ng + octile(nx, nz, gx, gz), j});
            }
        }
    }
    int result = -1;
    if (found) {
        /* the cell chain, goal first */
        int chain_len = 0;
        for (int c = goal; c != -1; c = parent[c]) chain_len++;
        int *chain = prts_malloc((size_t)chain_len * sizeof(int));
        int k = chain_len;
        for (int c = goal; c != -1; c = parent[c]) chain[--k] = c;
        /* string pulling: from each kept point, the farthest cell in a clear line */
        float px = x0, pz = z0;
        if (!prts_nav_walkable(n, x0, z0)) {
            px = centre_x(n, sx);
            pz = centre_z(n, sz);
        }
        result = 0;
        int at = 0;
        while (at < chain_len - 1 && result < max) {
            int best = at + 1;
            for (int j = chain_len - 1; j > at + 1; j--) {
                if (clear_cells(n, px, pz, centre_x(n, chain[j] % n->cols), centre_z(n, chain[j] / n->cols))) {
                    best = j;
                    break;
                }
            }
            px = centre_x(n, chain[best] % n->cols);
            pz = centre_z(n, chain[best] / n->cols);
            out[2 * result] = px;
            out[2 * result + 1] = pz;
            result++;
            at = best;
        }
        if (result == 0) { /* already in the goal cell */
            out[0] = goal_exact ? x1 : centre_x(n, gx);
            out[1] = goal_exact ? z1 : centre_z(n, gz);
            result = 1;
        } else if (goal_exact) {
            out[2 * (result - 1)] = x1;
            out[2 * (result - 1) + 1] = z1;
        }
        prts_free(chain);
    }
    return result;
}

void prts_nav_hash(const prts_nav *n, prts_hasher *h)
{
    prts_hasher_u64(h, (uint64_t)n->cols | ((uint64_t)n->rows << 32));
    prts_hasher_u64(h, n->block_hash);
}

/* Block counts as runs: u32 runs { u16 value as u32, u32 length }, then the running hash. */
void prts_nav_write(const prts_nav *n, prts_writer *w)
{
    prts_w_u32(w, (uint32_t)n->cols);
    prts_w_u32(w, (uint32_t)n->rows);
    size_t count = (size_t)n->cols * (size_t)n->rows, runs = 0;
    for (size_t i = 0; i < count; i++) runs += (i == 0 || n->blocks[i] != n->blocks[i - 1]) ? 1u : 0u;
    prts_w_u32(w, (uint32_t)runs);
    for (size_t i = 0; i < count;) {
        size_t j = i;
        while (j < count && n->blocks[j] == n->blocks[i]) j++;
        prts_w_u32(w, n->blocks[i]);
        prts_w_u32(w, (uint32_t)(j - i));
        i = j;
    }
    prts_w_u64(w, n->block_hash);
}

bool prts_nav_read(prts_nav *n, prts_reader *r, bool apply)
{
    int cols = (int)prts_r_u32(r), rows = (int)prts_r_u32(r);
    uint32_t runs = prts_r_u32(r);
    size_t count = (size_t)n->cols * (size_t)n->rows;
    if (r->failed || cols != n->cols || rows != n->rows || runs > count) return false;
    uint16_t *b = prts_malloc(count * sizeof(uint16_t));
    size_t at = 0;
    for (uint32_t k = 0; k < runs && !r->failed; k++) {
        uint32_t v = prts_r_u32(r), len = prts_r_u32(r);
        if (v > 65535u || len > count - at) {
            r->failed = true;
            break;
        }
        for (uint32_t m = 0; m < len; m++) b[at++] = (uint16_t)v;
    }
    uint64_t hash = prts_r_u64(r);
    if (r->failed || at != count || !apply) {
        prts_free(b);
        return !r->failed && at == count;
    }
    prts_free(n->blocks);
    n->blocks = b;
    n->block_hash = hash;
    return true;
}
