/* 1stPRTS engine: scene content built from script data (CPU side). MIT License. */
#include "prts/prts_scene.h"

#include <math.h>
#include <string.h>

#include "prts/prts_rng.h"

void prts_mesh_init(prts_mesh_cpu *m)
{
    prts_array_init(&m->verts, sizeof(prts_vertex));
    prts_array_init(&m->indices, sizeof(uint32_t));
}

void prts_mesh_free(prts_mesh_cpu *m)
{
    prts_array_free(&m->verts);
    prts_array_free(&m->indices);
}

static void push_vertex(prts_mesh_cpu *m, prts_vec3 p, prts_vec3 n, float u, float v, prts_vec3 c)
{
    prts_vertex vx = {p.x, p.y, p.z, n.x, n.y, n.z, u, v, c.x, c.y, c.z, 1.0f, 1.0f};
    prts_array_push(&m->verts, &vx);
}

static void push_index(prts_mesh_cpu *m, uint32_t i) { prts_array_push(&m->indices, &i); }

static void add_quad(prts_mesh_cpu *m, prts_vec3 a, prts_vec3 b, prts_vec3 c, prts_vec3 d, prts_vec3 color,
                     float uv_scale, float tile, bool fitted);

void prts_mesh_add_quad(prts_mesh_cpu *m, prts_vec3 a, prts_vec3 b, prts_vec3 c, prts_vec3 d, prts_vec3 color,
                        float uv_scale, float tile)
{
    add_quad(m, a, b, c, d, color, uv_scale, tile, false);
}

static void add_quad(prts_mesh_cpu *m, prts_vec3 a, prts_vec3 b, prts_vec3 c, prts_vec3 d, prts_vec3 color,
                     float uv_scale, float tile, bool fitted)
{
    prts_vec3 eu = prts_v3_sub(b, a), ev = prts_v3_sub(d, a);
    prts_vec3 n = prts_v3_normalize(prts_v3_cross(eu, ev));
    float lu = prts_v3_length(eu), lv = prts_v3_length(ev);
    int nu = PRTS_MAX(1, (int)ceilf(lu / tile)), nv = PRTS_MAX(1, (int)ceilf(lv / tile));
    /* world-aligned texture coordinates: pick the two axes the face spans most */
    float ax = fabsf(n.x), ay = fabsf(n.y), az = fabsf(n.z);
    uint32_t base = (uint32_t)m->verts.len;
    for (int j = 0; j <= nv; j++) {
        for (int i = 0; i <= nu; i++) {
            float fu = (float)i / (float)nu, fv = (float)j / (float)nv;
            prts_vec3 bottom = prts_v3_lerp(a, b, fu), top = prts_v3_lerp(d, c, fu);
            prts_vec3 p = prts_v3_lerp(bottom, top, fv);
            float u, v;
            if (fitted) {
                u = fu * uv_scale;
                v = (1.0f - fv) * uv_scale;
            } else if (ay >= ax && ay >= az) {
                u = p.x;
                v = p.z;
            } else if (ax >= az) {
                u = p.z;
                v = p.y;
            } else {
                u = p.x;
                v = p.y;
            }
            if (fitted) push_vertex(m, p, n, u / uv_scale, v / uv_scale, color);
            else push_vertex(m, p, n, u / uv_scale, -v / uv_scale, color);
        }
    }
    for (int j = 0; j < nv; j++) {
        for (int i = 0; i < nu; i++) {
            uint32_t i0 = base + (uint32_t)(j * (nu + 1) + i), i1 = i0 + 1;
            uint32_t i2 = i0 + (uint32_t)(nu + 1), i3 = i2 + 1;
            push_index(m, i0);
            push_index(m, i1);
            push_index(m, i3);
            push_index(m, i0);
            push_index(m, i3);
            push_index(m, i2);
        }
    }
    (void)c;
}

void prts_scene_env_default(prts_scene_env *env)
{
    env->sun_dir = prts_v3_normalize(prts_v3(0.4f, 0.8f, 0.3f));
    env->sun_color = prts_v3(1.0f, 0.95f, 0.85f);
    env->ambient = prts_v3(0.35f, 0.38f, 0.45f);
    env->fog_color = prts_v3(0.6f, 0.68f, 0.78f);
    env->fog_near = 150.0f;
    env->fog_far = 1500.0f;
    env->sky_horizon = prts_v3(0.7f, 0.76f, 0.85f);
    env->sky_zenith = prts_v3(0.25f, 0.4f, 0.7f);
    env->stars = 0.0f;
    env->moons = false;
}

prts_scene *prts_scene_create(void)
{
    prts_scene *s = prts_calloc(1, sizeof(*s));
    prts_array_init(&s->materials, sizeof(prts_material));
    prts_array_init(&s->groups, sizeof(prts_mesh_group));
    prts_array_init(&s->lights, sizeof(prts_light));
    prts_array_init(&s->markers, sizeof(prts_marker));
    prts_array_init(&s->zones, sizeof(prts_zone));
    prts_array_init(&s->lines, sizeof(prts_line_set));
    prts_array_init(&s->cells, sizeof(prts_cell));
    prts_array_init(&s->surfaces, sizeof(prts_surface));
    prts_array_init(&s->sprite_defs, sizeof(prts_sprite_def));
    prts_array_init(&s->sprites, sizeof(prts_sprite));
    prts_array_init(&s->portals, sizeof(prts_portal));
    prts_scene_clear(s);
    return s;
}

void prts_scene_clear(prts_scene *s)
{
    for (size_t i = 0; i < s->groups.len; i++) prts_mesh_free(&((prts_mesh_group *)prts_array_at(&s->groups, i))->mesh);
    for (size_t i = 0; i < s->lines.len; i++) prts_array_free(&((prts_line_set *)prts_array_at(&s->lines, i))->points);
    prts_array_clear(&s->materials);
    prts_array_clear(&s->groups);
    prts_array_clear(&s->lights);
    prts_array_clear(&s->markers);
    prts_array_clear(&s->zones);
    prts_array_clear(&s->lines);
    prts_array_clear(&s->cells);
    prts_array_clear(&s->surfaces);
    prts_array_clear(&s->sprite_defs);
    prts_array_clear(&s->sprites);
    prts_array_clear(&s->portals);
    prts_cell outside;
    memset(&outside, 0, sizeof(outside));
    strcpy(outside.name, "outside");
    prts_array_push(&s->cells, &outside);
    s->current_cells = PRTS_CELLS_ALL;
    prts_heightfield_destroy(s->terrain);
    s->terrain = NULL;
    prts_scene_env_default(&s->env);
    s->revision++;
}

void prts_scene_destroy(prts_scene *s)
{
    if (!s) return;
    prts_scene_clear(s);
    prts_array_free(&s->materials);
    prts_array_free(&s->groups);
    prts_array_free(&s->lights);
    prts_array_free(&s->markers);
    prts_array_free(&s->zones);
    prts_array_free(&s->lines);
    prts_array_free(&s->cells);
    prts_array_free(&s->surfaces);
    prts_array_free(&s->sprite_defs);
    prts_array_free(&s->sprites);
    prts_array_free(&s->portals);
    prts_free(s);
}

int prts_scene_find_material(const prts_scene *s, const char *id)
{
    for (size_t i = 0; i < s->materials.len; i++) {
        if (strcmp(((prts_material *)prts_array_at(&s->materials, i))->id, id) == 0) return (int)i;
    }
    return -1;
}

int prts_scene_material(prts_scene *s, const prts_material *m)
{
    int idx = prts_scene_find_material(s, m->id);
    prts_material copy = *m;
    if (copy.uv_scale <= 0.0f) copy.uv_scale = 2.0f;
    if (idx >= 0) {
        *(prts_material *)prts_array_at(&s->materials, (size_t)idx) = copy;
    } else {
        prts_array_push(&s->materials, &copy);
        idx = (int)s->materials.len - 1;
    }
    s->revision++;
    return idx;
}

prts_mesh_group *prts_scene_group(prts_scene *s, int material, prts_entity owner)
{
    for (size_t i = 0; i < s->groups.len; i++) {
        prts_mesh_group *g = prts_array_at(&s->groups, i);
        if (g->material == material && g->owner == owner && g->cells == s->current_cells) return g;
    }
    prts_mesh_group *g = prts_array_push(&s->groups, NULL);
    g->material = material;
    g->owner = owner;
    g->cells = s->current_cells;
    prts_mesh_init(&g->mesh);
    return g;
}

static prts_material *material_at(prts_scene *s, int material)
{
    static prts_material fallback = {"", "", {1, 1, 1}, 2.0f, false, false, ""};
    prts_material *m = material >= 0 ? prts_array_at(&s->materials, (size_t)material) : NULL;
    return m ? m : &fallback;
}

static void box_faces(prts_scene *s, prts_aabb b, int material, prts_entity owner, float tile, bool fitted);

void prts_scene_add_box(prts_scene *s, prts_aabb b, int material, prts_entity owner, float tile)
{
    box_faces(s, b, material, owner, tile, false);
}

void prts_scene_add_box_fitted(prts_scene *s, prts_aabb b, int material, prts_entity owner)
{
    box_faces(s, b, material, owner, 1e6f, true);
}

static void box_faces(prts_scene *s, prts_aabb b, int material, prts_entity owner, float tile, bool fitted)
{
    prts_mesh_group *g = prts_scene_group(s, material, owner);
    prts_material *m = material_at(s, material);
    prts_vec3 c = m->tint;
    float uv = fitted ? 1.0f : m->uv_scale;
    prts_vec3 lo = b.min, hi = b.max;
    prts_vec3 p000 = lo, p100 = prts_v3(hi.x, lo.y, lo.z), p010 = prts_v3(lo.x, hi.y, lo.z);
    prts_vec3 p110 = prts_v3(hi.x, hi.y, lo.z), p001 = prts_v3(lo.x, lo.y, hi.z), p101 = prts_v3(hi.x, lo.y, hi.z);
    prts_vec3 p011 = prts_v3(lo.x, hi.y, hi.z), p111 = hi;
    add_quad(&g->mesh, p001, p101, p111, p011, c, uv, tile, fitted); /* +z */
    add_quad(&g->mesh, p100, p000, p010, p110, c, uv, tile, fitted); /* -z */
    add_quad(&g->mesh, p101, p100, p110, p111, c, uv, tile, fitted); /* +x */
    add_quad(&g->mesh, p000, p001, p011, p010, c, uv, tile, fitted); /* -x */
    add_quad(&g->mesh, p011, p111, p110, p010, c, uv, tile, fitted); /* +y */
    add_quad(&g->mesh, p000, p100, p101, p001, c, uv, tile, fitted); /* -y */
    s->revision++;
}

void prts_scene_add_prism(prts_scene *s, prts_vec3 base, float radius, float top_radius, float height, int sides,
                          int material, prts_entity owner)
{
    if (sides < 3) sides = 3;
    prts_mesh_group *g = prts_scene_group(s, material, owner);
    prts_material *m = material_at(s, material);
    for (int i = 0; i < sides; i++) {
        float a0 = (float)(2.0 * PRTS_PI * i / sides), a1 = (float)(2.0 * PRTS_PI * (i + 1) / sides);
        prts_vec3 b0 = prts_v3(base.x + cosf(a0) * radius, base.y, base.z + sinf(a0) * radius);
        prts_vec3 b1 = prts_v3(base.x + cosf(a1) * radius, base.y, base.z + sinf(a1) * radius);
        prts_vec3 t0 = prts_v3(base.x + cosf(a0) * top_radius, base.y + height, base.z + sinf(a0) * top_radius);
        prts_vec3 t1 = prts_v3(base.x + cosf(a1) * top_radius, base.y + height, base.z + sinf(a1) * top_radius);
        if (top_radius <= 0.0f) {
            /* a cone side: a degenerate quad would break normals, so use a triangle */
            prts_vec3 apex = prts_v3(base.x, base.y + height, base.z);
            prts_vec3 n = prts_v3_normalize(prts_v3_cross(prts_v3_sub(b0, b1), prts_v3_sub(apex, b1)));
            uint32_t i0 = (uint32_t)g->mesh.verts.len;
            push_vertex(&g->mesh, b1, n, 0.0f, 0.0f, m->tint);
            push_vertex(&g->mesh, b0, n, radius / m->uv_scale, 0.0f, m->tint);
            push_vertex(&g->mesh, apex, n, 0.5f * radius / m->uv_scale, -height / m->uv_scale, m->tint);
            push_index(&g->mesh, i0);
            push_index(&g->mesh, i0 + 1);
            push_index(&g->mesh, i0 + 2);
        } else {
            prts_mesh_add_quad(&g->mesh, b1, b0, t0, t1, m->tint, m->uv_scale, 2.0f);
        }
    }
    s->revision++;
}

/* --- terrain -------------------------------------------------------------- */

static float lattice(uint64_t seed, int x, int z)
{
    prts_hasher h;
    prts_hasher_init(&h);
    prts_hasher_u64(&h, seed);
    prts_hasher_u64(&h, (uint64_t)(int64_t)x);
    prts_hasher_u64(&h, (uint64_t)(int64_t)z);
    return (float)(h.h >> 40) / (float)(1u << 24);
}

static float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

static float value_noise(uint64_t seed, float x, float z)
{
    int ix = (int)floorf(x), iz = (int)floorf(z);
    float tx = smooth(x - (float)ix), tz = smooth(z - (float)iz);
    float a = lattice(seed, ix, iz), b = lattice(seed, ix + 1, iz);
    float c = lattice(seed, ix, iz + 1), d = lattice(seed, ix + 1, iz + 1);
    float ab = a + (b - a) * tx, cd = c + (d - c) * tx;
    return ab + (cd - ab) * tz;
}

void prts_scene_terrain(prts_scene *s, const prts_terrain_params *p, const prts_terrain_flat *flats, int nflats)
{
    prts_heightfield_destroy(s->terrain);
    int n = (int)(p->size / p->spacing) + 1;
    float origin = -p->size * 0.5f;
    prts_heightfield *hf = prts_heightfield_create(n, n, p->spacing, origin, origin);
    for (int iz = 0; iz < n; iz++) {
        for (int ix = 0; ix < n; ix++) {
            float x = origin + (float)ix * p->spacing, z = origin + (float)iz * p->spacing;
            float h = 0.0f, amp = 1.0f, freq = 1.0f / 160.0f, norm = 0.0f;
            for (int o = 0; o < PRTS_MAX(1, p->octaves); o++) {
                h += amp * value_noise(p->seed + (uint64_t)o * 7919u, x * freq, z * freq);
                norm += amp;
                amp *= 0.5f;
                freq *= 2.0f;
            }
            h = (h / norm - 0.5f) * 2.0f * p->height;
            for (int f = 0; f < nflats; f++) {
                float dx = x - flats[f].x, dz = z - flats[f].z;
                float d = sqrtf(dx * dx + dz * dz);
                float w = d <= flats[f].radius ? 1.0f
                        : d >= flats[f].radius + flats[f].blend ? 0.0f
                        : smooth(1.0f - (d - flats[f].radius) / flats[f].blend);
                h = h + (flats[f].height - h) * w;
            }
            prts_heightfield_set(hf, ix, iz, h);
        }
    }
    s->terrain = hf;
    /* mesh: one vertex per grid point, coloured by slope (grass on flats, earth on slopes) */
    prts_mesh_group *g = prts_scene_group(s, p->material, PRTS_ENTITY_NONE);
    prts_material *m = material_at(s, p->material);
    uint32_t base = (uint32_t)g->mesh.verts.len;
    for (int iz = 0; iz < n; iz++) {
        for (int ix = 0; ix < n; ix++) {
            float x = origin + (float)ix * p->spacing, z = origin + (float)iz * p->spacing;
            prts_vec3 nrm = prts_heightfield_normal(hf, x, z);
            float slope = 1.0f - nrm.y;
            float k = prts_clampf(slope * 4.0f, 0.0f, 1.0f);
            float shade = 0.92f + 0.16f * lattice(p->seed ^ 0xabcdu, ix, iz);
            prts_vec3 grass = prts_v3_scale(m->tint, shade), earth = prts_v3(0.55f, 0.45f, 0.32f);
            push_vertex(&g->mesh, prts_v3(x, prts_heightfield_get(hf, ix, iz), z), nrm, x / m->uv_scale,
                        z / m->uv_scale, prts_v3_lerp(grass, earth, k));
        }
    }
    for (int iz = 0; iz + 1 < n; iz++) {
        for (int ix = 0; ix + 1 < n; ix++) {
            uint32_t i0 = base + (uint32_t)(iz * n + ix), i1 = i0 + 1, i2 = i0 + (uint32_t)n, i3 = i2 + 1;
            push_index(&g->mesh, i0);
            push_index(&g->mesh, i2);
            push_index(&g->mesh, i3);
            push_index(&g->mesh, i0);
            push_index(&g->mesh, i3);
            push_index(&g->mesh, i1);
        }
    }
    s->revision++;
}

float prts_scene_height(const prts_scene *s, float x, float z)
{
    return s->terrain ? prts_heightfield_sample(s->terrain, x, z) : 0.0f;
}

void prts_scene_add_light(prts_scene *s, prts_light light)
{
    prts_array_push(&s->lights, &light);
    s->revision++;
}

void prts_scene_add_marker(prts_scene *s, const char *name, prts_dvec3 pos, float yaw)
{
    prts_marker m;
    memset(&m, 0, sizeof(m));
    strncpy(m.name, name, sizeof(m.name) - 1);
    m.pos = pos;
    m.yaw = yaw;
    prts_array_push(&s->markers, &m);
}

const prts_marker *prts_scene_find_marker(const prts_scene *s, const char *name)
{
    for (size_t i = 0; i < s->markers.len; i++) {
        prts_marker *m = prts_array_at(&s->markers, i);
        if (strcmp(m->name, name) == 0) return m;
    }
    return NULL;
}

void prts_scene_add_zone(prts_scene *s, const char *name, prts_aabb box)
{
    prts_zone z;
    memset(&z, 0, sizeof(z));
    strncpy(z.name, name, sizeof(z.name) - 1);
    z.box = box;
    prts_array_push(&s->zones, &z);
}

const prts_zone *prts_scene_find_zone(const prts_scene *s, const char *name)
{
    for (size_t i = 0; i < s->zones.len; i++) {
        prts_zone *z = prts_array_at(&s->zones, i);
        if (strcmp(z->name, name) == 0) return z;
    }
    return NULL;
}

static void push_point(prts_array *pts, prts_vec3 p) { prts_array_push(pts, &p); }

void prts_scene_add_contours(prts_scene *s, prts_entity owner, prts_vec3 center, float size_x, float size_z,
                             float relief, float x0, float z0, float x1, float z1, int levels, prts_vec3 color)
{
    prts_line_set *ls = prts_array_push(&s->lines, NULL);
    ls->owner = owner;
    ls->cells = s->current_cells;
    ls->color = color;
    prts_array_init(&ls->points, sizeof(prts_vec3));
    if (!s->terrain || levels < 1) return;
    const int grid = 64;
    const float gf = (float)grid;
    float hmin = 1e30f, hmax = -1e30f;
    for (int j = 0; j <= grid; j++) {
        for (int i = 0; i <= grid; i++) {
            float h = prts_scene_height(s, x0 + (x1 - x0) * (float)i / gf, z0 + (z1 - z0) * (float)j / gf);
            hmin = fminf(hmin, h);
            hmax = fmaxf(hmax, h);
        }
    }
    float range = fmaxf(hmax - hmin, 1e-3f);
    for (int l = 1; l <= levels; l++) {
        float iso = hmin + range * (float)l / (float)(levels + 1);
        float y = center.y + relief * (float)l / (float)(levels + 1);
        for (int j = 0; j < grid; j++) {
            for (int i = 0; i < grid; i++) {
                /* marching squares on one cell */
                float u[4] = {(float)i, (float)(i + 1), (float)(i + 1), (float)i};
                float v[4] = {(float)j, (float)j, (float)(j + 1), (float)(j + 1)};
                float h[4];
                for (int k = 0; k < 4; k++) {
                    h[k] = prts_scene_height(s, x0 + (x1 - x0) * u[k] / gf, z0 + (z1 - z0) * v[k] / gf);
                }
                prts_vec3 cross[4];
                int nc = 0;
                for (int k = 0; k < 4 && nc < 4; k++) {
                    int k2 = (k + 1) % 4;
                    if ((h[k] < iso) != (h[k2] < iso)) {
                        float t = (iso - h[k]) / (h[k2] - h[k]);
                        float cu = u[k] + (u[k2] - u[k]) * t, cv = v[k] + (v[k2] - v[k]) * t;
                        cross[nc++] = prts_v3(center.x + (cu / gf - 0.5f) * size_x, y,
                                              center.z + (cv / gf - 0.5f) * size_z);
                    }
                }
                if (nc >= 2) {
                    push_point(&ls->points, cross[0]);
                    push_point(&ls->points, cross[1]);
                }
                if (nc == 4) {
                    push_point(&ls->points, cross[2]);
                    push_point(&ls->points, cross[3]);
                }
            }
        }
    }
    /* frame around the map */
    prts_vec3 c0 = prts_v3(center.x - size_x * 0.5f, center.y, center.z - size_z * 0.5f);
    prts_vec3 c1 = prts_v3(center.x + size_x * 0.5f, center.y, center.z - size_z * 0.5f);
    prts_vec3 c2 = prts_v3(center.x + size_x * 0.5f, center.y, center.z + size_z * 0.5f);
    prts_vec3 c3 = prts_v3(center.x - size_x * 0.5f, center.y, center.z + size_z * 0.5f);
    prts_vec3 frame[8] = {c0, c1, c1, c2, c2, c3, c3, c0};
    for (int k = 0; k < 8; k++) push_point(&ls->points, frame[k]);
    s->revision++;
}

void prts_scene_bake_sun(prts_scene *s, const prts_cworld *cw, const prts_world *w)
{
    prts_vec3 sun = prts_v3_normalize(s->env.sun_dir);
    for (size_t i = 0; i < s->groups.len; i++) {
        prts_mesh_group *g = prts_array_at(&s->groups, i);
        prts_dvec3 op = prts_world_pos(w, g->owner);
        float yaw = prts_world_yaw(w, g->owner);
        prts_material *m = material_at(s, g->material);
        for (size_t v = 0; v < g->mesh.verts.len; v++) {
            prts_vertex *vx = prts_array_at(&g->mesh.verts, v);
            if (m->emissive) {
                vx->sun = vx->sky = 1.0f;
                continue;
            }
            prts_vec3 n = prts_v3_rotate_y(prts_v3(vx->nx, vx->ny, vx->nz), yaw);
            prts_vec3 p = prts_v3_rotate_y(prts_v3(vx->px, vx->py, vx->pz), yaw);
            p = prts_v3_add(prts_v3((float)op.x, (float)op.y, (float)op.z), p);
            p = prts_v3_add(p, prts_v3_scale(n, 0.05f));
            vx->sky = prts_cworld_occluded(cw, p, prts_v3(0, 1, 0), 60.0f) ? 0.0f : 1.0f;
            /* facing away from the sun: the shader gives no sun anyway */
            vx->sun = prts_v3_dot(n, sun) > 0.0f && !prts_cworld_occluded(cw, p, sun, 400.0f) ? 1.0f : 0.0f;
        }
    }
    s->revision++;
}

int prts_scene_add_surface(prts_scene *s, const prts_surface *surface)
{
    int found = prts_scene_find_surface(s, surface->name);
    if (found >= 0) {
        *(prts_surface *)prts_array_at(&s->surfaces, (size_t)found) = *surface;
    } else {
        prts_array_push(&s->surfaces, surface);
        found = (int)s->surfaces.len - 1;
    }
    s->revision++;
    return found;
}

int prts_scene_find_surface(const prts_scene *s, const char *name)
{
    for (size_t i = 0; i < s->surfaces.len; i++) {
        if (strcmp(((prts_surface *)prts_array_at(&s->surfaces, i))->name, name) == 0) return (int)i;
    }
    return -1;
}

int prts_scene_add_sprite_def(prts_scene *s, const prts_sprite_def *def)
{
    prts_array_push(&s->sprite_defs, def);
    s->revision++;
    return (int)s->sprite_defs.len - 1;
}

int prts_scene_find_sprite_def(const prts_scene *s, const char *path)
{
    for (size_t i = 0; i < s->sprite_defs.len; i++) {
        if (strcmp(((prts_sprite_def *)prts_array_at(&s->sprite_defs, i))->path, path) == 0) return (int)i;
    }
    return -1;
}

int prts_scene_find_anim(const prts_sprite_def *def, const char *name)
{
    for (int i = 0; i < def->anim_count; i++) {
        if (strcmp(def->anims[i].name, name) == 0) return i;
    }
    return -1;
}

int prts_scene_add_sprite(prts_scene *s, const prts_sprite *sprite)
{
    prts_sprite copy = *sprite;
    copy.cells = s->current_cells;
    prts_array_push(&s->sprites, &copy);
    return (int)s->sprites.len - 1;
}

int prts_sprite_direction(float facing_yaw, prts_vec3 to_viewer, int directions)
{
    if (directions <= 1) return 0;
    /* yaw convention: yaw 0 looks along -z, so the yaw pointing at the viewer is atan2(-x, -z) */
    float towards = atan2f(-to_viewer.x, -to_viewer.z);
    float diff = prts_wrap_angle(towards - facing_yaw);
    float step = (float)(2.0 * PRTS_PI) / (float)directions;
    int d = (int)lroundf(diff / step);
    return ((d % directions) + directions) % directions;
}

int prts_sprite_frame(const prts_sprite_anim *a, double seconds)
{
    if (a->frames <= 1 || a->fps <= 0.0f || seconds <= 0.0) return 0;
    return (int)fmod(floor(seconds * (double)a->fps), (double)a->frames);
}
