/* Unit tests: generated shapes (ramps and castle towers) face the right way. MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_scene.h"
#include "utest.h"

typedef struct tri {
    prts_vec3 c, n; /* centre and (unnormalized) front normal */
} tri;

static size_t triangles(const prts_scene *s, tri *out, size_t max)
{
    size_t n = 0;
    for (size_t g = 0; g < s->groups.len; g++) {
        const prts_mesh_group *grp = prts_array_at(&s->groups, g);
        const uint32_t *ix = (const uint32_t *)grp->mesh.indices.data;
        for (size_t i = 0; i + 2 < grp->mesh.indices.len && n < max; i += 3) {
            const prts_vertex *a = prts_array_at(&grp->mesh.verts, ix[i]);
            const prts_vertex *b = prts_array_at(&grp->mesh.verts, ix[i + 1]);
            const prts_vertex *c = prts_array_at(&grp->mesh.verts, ix[i + 2]);
            prts_vec3 pa = prts_v3(a->px, a->py, a->pz), pb = prts_v3(b->px, b->py, b->pz), pc = prts_v3(c->px, c->py, c->pz);
            prts_vec3 nn = prts_v3_cross(prts_v3_sub(pb, pa), prts_v3_sub(pc, pa));
            if (prts_v3_length(nn) < 1e-9f) continue; /* degenerate pieces of a triangle face */
            out[n].c = prts_v3_scale(prts_v3_add(prts_v3_add(pa, pb), pc), 1.0f / 3.0f);
            out[n].n = nn;
            n++;
        }
    }
    return n;
}

static int stone(prts_scene *s)
{
    prts_material m;
    memset(&m, 0, sizeof(m));
    strcpy(m.id, "stone");
    m.tint = prts_v3(1, 1, 1);
    m.uv_scale = 2.0f;
    return prts_scene_material(s, &m);
}

UTEST(shapes, wedges_face_outwards_in_every_direction)
{
    static tri t[4096];
    for (int axis = 0; axis <= 2; axis += 2) {
        for (int sign = -1; sign <= 1; sign += 2) {
            prts_scene *s = prts_scene_create();
            prts_aabb b = prts_aabb_make(prts_v3(1, 0, 2), prts_v3(5, 2, 4));
            prts_scene_add_wedge(s, b, axis, sign, stone(s), PRTS_ENTITY_NONE, 2.0f);
            size_t n = triangles(s, t, 4096);
            ASSERT_GT(n, (size_t)6);
            /* a wedge is convex: every face looks away from a point inside it, near its floor */
            float lo = axis == 0 ? b.min.x : b.min.z, hi = axis == 0 ? b.max.x : b.max.z;
            float high_end = sign > 0 ? hi : lo, inside = high_end + (sign > 0 ? -0.5f : 0.5f);
            prts_vec3 p = axis == 0 ? prts_v3(inside, 0.3f, 3) : prts_v3(3, 0.3f, inside);
            for (size_t i = 0; i < n; i++) ASSERT_GT(prts_v3_dot(t[i].n, prts_v3_sub(t[i].c, p)), 0.0f);
            prts_scene_destroy(s);
        }
    }
}

UTEST(shapes, towers_face_out_and_in_with_open_sides)
{
    static tri t[8192];
    prts_scene *s = prts_scene_create();
    prts_tube tube;
    memset(&tube, 0, sizeof(tube));
    tube.base = prts_v3(10, 0, -4);
    tube.radius = 6;
    tube.inner = 4.5f;
    tube.height = 3;
    tube.sides = 16;
    tube.open = (1u << 0) | (1u << 4) | (1u << 5);
    tube.cap = true;
    prts_scene_add_tube(s, &tube, stone(s), PRTS_ENTITY_NONE);
    size_t n = triangles(s, t, 8192);
    int outer = 0, inner = 0, top = 0, jambs = 0;
    for (size_t i = 0; i < n; i++) {
        prts_vec3 r = prts_v3(t[i].c.x - 10, 0, t[i].c.z + 4);
        float dist = prts_v3_length(r);
        prts_vec3 nn = prts_v3_normalize(t[i].n);
        if (fabsf(t[i].c.y - 3) < 1e-4f && fabsf(nn.y) > 0.99f) {
            ASSERT_GT(nn.y, 0.0f);
            top++;
        } else if (prts_v3_dot(nn, prts_v3_normalize(r)) > 0.9f) {
            ASSERT_GT(dist, 5.5f); /* outer faces look out */
            outer++;
        } else if (prts_v3_dot(nn, prts_v3_normalize(r)) < -0.9f) {
            ASSERT_LT(dist, 5.0f); /* inner faces look in */
            inner++;
        } else {
            jambs++; /* the sides of the openings look along the wall */
            ASSERT_LT(fabsf(nn.y), 0.01f);
        }
    }
    ASSERT_GT(outer, 0);
    ASSERT_GT(inner, 0);
    ASSERT_GT(top, 0);
    ASSERT_GT(jambs, 0);
    /* nothing is drawn across the opening at side 0 (+x): no face centre near (16, y, -4) */
    for (size_t i = 0; i < n; i++) ASSERT_FALSE(t[i].c.x > 15.2f && fabsf(t[i].c.z + 4) < 0.8f);
    size_t closed_verts = 0, open_verts = 0;
    open_verts = ((prts_mesh_group *)prts_array_at(&s->groups, 0))->mesh.verts.len;
    prts_scene_destroy(s);
    /* a solid tower (no inner wall, no cap) has fewer faces than the hollow one */
    s = prts_scene_create();
    tube.open = 0;
    tube.inner = 0;
    tube.cap = false;
    tube.sides = 2; /* clamped to 3 */
    prts_scene_add_tube(s, &tube, stone(s), PRTS_ENTITY_NONE);
    closed_verts = ((prts_mesh_group *)prts_array_at(&s->groups, 0))->mesh.verts.len;
    ASSERT_GT(closed_verts, (size_t)0);
    ASSERT_LT(closed_verts, open_verts);
    prts_scene_destroy(s);
}
