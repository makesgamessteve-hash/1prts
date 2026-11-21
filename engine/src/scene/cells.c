/* 1stPRTS engine: interior cells and portals (visibility and sound paths). MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_scene.h"

int prts_scene_add_cell(prts_scene *s, const char *name, prts_aabb volume)
{
    if (s->cells.len >= PRTS_MAX_CELLS) return -1;
    prts_cell c;
    memset(&c, 0, sizeof(c));
    strncpy(c.name, name, sizeof(c.name) - 1);
    c.box = volume;
    prts_array_push(&s->cells, &c);
    return (int)s->cells.len - 1;
}

int prts_scene_find_cell(const prts_scene *s, const char *name)
{
    for (size_t i = 0; i < s->cells.len; i++) {
        if (strcmp(((prts_cell *)prts_array_at(&s->cells, i))->name, name) == 0) return (int)i;
    }
    return -1;
}

const char *prts_scene_cell_name(const prts_scene *s, int cell)
{
    prts_cell *c = cell >= 0 ? prts_array_at(&s->cells, (size_t)cell) : NULL;
    return c ? c->name : "";
}

static bool contains(prts_aabb b, prts_vec3 p)
{
    return p.x >= b.min.x && p.x <= b.max.x && p.y >= b.min.y && p.y <= b.max.y && p.z >= b.min.z && p.z <= b.max.z;
}

static float volume(prts_aabb b) { return (b.max.x - b.min.x) * (b.max.y - b.min.y) * (b.max.z - b.min.z); }

int prts_scene_cell_at(const prts_scene *s, prts_vec3 p)
{
    int best = 0;
    float best_v = 0.0f;
    for (size_t i = 1; i < s->cells.len; i++) {
        prts_cell *c = prts_array_at(&s->cells, i);
        float v = volume(c->box);
        if (contains(c->box, p) && (best == 0 || v < best_v)) {
            best = (int)i;
            best_v = v;
        }
    }
    return best;
}

int prts_scene_add_portal(prts_scene *s, const char *name, int a, int b, prts_aabb opening, bool open)
{
    prts_portal p;
    memset(&p, 0, sizeof(p));
    strncpy(p.name, name, sizeof(p.name) - 1);
    p.a = a;
    p.b = b;
    p.box = opening;
    p.open = open;
    prts_array_push(&s->portals, &p);
    return (int)s->portals.len - 1;
}

static prts_portal *find_portal(const prts_scene *s, const char *name)
{
    for (size_t i = 0; i < s->portals.len; i++) {
        prts_portal *p = prts_array_at(&s->portals, i);
        if (strcmp(p->name, name) == 0) return p;
    }
    return NULL;
}

bool prts_scene_set_portal(prts_scene *s, const char *name, bool open)
{
    prts_portal *p = find_portal(s, name);
    if (!p) return false;
    p->open = open;
    return true;
}

bool prts_scene_portal_open(const prts_scene *s, const char *name)
{
    prts_portal *p = find_portal(s, name);
    return p && p->open;
}

typedef struct rect {
    float x0, y0, x1, y1;
} rect;

/* Screen rectangle (NDC) covered by a portal: empty when it is entirely behind the eye, the
 * full screen when the eye is close to it or it straddles the eye plane (conservative). */
static rect portal_rect(prts_aabb box, prts_vec3 eye, prts_mat4 vp)
{
    rect full = {-1.0f, -1.0f, 1.0f, 1.0f};
    prts_aabb grown = {prts_v3_sub(box.min, prts_v3(0.5f, 0.5f, 0.5f)), prts_v3_add(box.max, prts_v3(0.5f, 0.5f, 0.5f))};
    if (contains(grown, eye)) return full;
    rect r = {1e9f, 1e9f, -1e9f, -1e9f};
    int behind = 0;
    for (int c = 0; c < 8; c++) {
        prts_vec3 v = prts_v3((c & 1) ? box.max.x : box.min.x, (c & 2) ? box.max.y : box.min.y,
                              (c & 4) ? box.max.z : box.min.z);
        v = prts_v3_sub(v, eye);
        float x = vp.m[0] * v.x + vp.m[4] * v.y + vp.m[8] * v.z + vp.m[12];
        float y = vp.m[1] * v.x + vp.m[5] * v.y + vp.m[9] * v.z + vp.m[13];
        float w = vp.m[3] * v.x + vp.m[7] * v.y + vp.m[11] * v.z + vp.m[15];
        if (w <= 0.01f) {
            behind++;
            continue;
        }
        r.x0 = fminf(r.x0, x / w);
        r.y0 = fminf(r.y0, y / w);
        r.x1 = fmaxf(r.x1, x / w);
        r.y1 = fmaxf(r.y1, y / w);
    }
    if (behind == 8) {
        rect none = {0.0f, 0.0f, 0.0f, 0.0f};
        return none; /* entirely behind the eye */
    }
    return behind > 0 ? full : r;
}

static rect intersect(rect a, rect b)
{
    rect r = {fmaxf(a.x0, b.x0), fmaxf(a.y0, b.y0), fminf(a.x1, b.x1), fminf(a.y1, b.y1)};
    return r;
}

static bool empty(rect r) { return r.x0 >= r.x1 || r.y0 >= r.y1; }

prts_cell_mask prts_scene_visible_cells(const prts_scene *s, prts_vec3 eye, prts_mat4 view_proj)
{
    rect seen[PRTS_MAX_CELLS];
    bool has[PRTS_MAX_CELLS] = {false};
    int queue[PRTS_MAX_CELLS * 4];
    int head = 0, tail = 0;
    int start = prts_scene_cell_at(s, eye);
    seen[start] = (rect){-1.0f, -1.0f, 1.0f, 1.0f};
    has[start] = true;
    queue[tail++] = start;
    prts_cell_mask visible = PRTS_CELL_BIT(start);
    while (head < tail) {
        int c = queue[head++];
        for (size_t i = 0; i < s->portals.len; i++) {
            prts_portal *p = prts_array_at(&s->portals, i);
            if (!p->open || (p->a != c && p->b != c)) continue;
            int other = p->a == c ? p->b : p->a;
            rect r = intersect(seen[c], portal_rect(p->box, eye, view_proj));
            if (empty(r)) continue;
            if (has[other]) {
                rect grown = {fminf(seen[other].x0, r.x0), fminf(seen[other].y0, r.y0), fmaxf(seen[other].x1, r.x1),
                              fmaxf(seen[other].y1, r.y1)};
                if (grown.x0 >= seen[other].x0 && grown.y0 >= seen[other].y0 && grown.x1 <= seen[other].x1 &&
                    grown.y1 <= seen[other].y1) {
                    continue; /* nothing new to see through this portal */
                }
                r = grown;
            }
            seen[other] = r;
            has[other] = true;
            visible |= PRTS_CELL_BIT(other);
            if (tail < PRTS_MAX_CELLS * 4) queue[tail++] = other;
        }
    }
    return visible;
}

prts_cell_mask prts_scene_reachable_cells(const prts_scene *s, int cell)
{
    prts_cell_mask reached = PRTS_CELL_BIT(cell);
    bool grew = true;
    while (grew) {
        grew = false;
        for (size_t i = 0; i < s->portals.len; i++) {
            prts_portal *p = prts_array_at(&s->portals, i);
            if (!p->open) continue;
            prts_cell_mask a = PRTS_CELL_BIT(p->a), b = PRTS_CELL_BIT(p->b);
            if ((reached & a) && !(reached & b)) {
                reached |= b;
                grew = true;
            } else if ((reached & b) && !(reached & a)) {
                reached |= a;
                grew = true;
            }
        }
    }
    return reached;
}
