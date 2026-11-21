/* Unit tests: collision world. MIT License. */
#include <math.h>

#include "prts/prts_collide.h"
#include "utest.h"

static prts_aabb box(float x0, float y0, float z0, float x1, float y1, float z1)
{
    return prts_aabb_make(prts_v3(x0, y0, z0), prts_v3(x1, y1, z1));
}

UTEST(collide, aabb_helpers)
{
    prts_aabb a = box(1, 1, 1, 0, 0, 0);
    ASSERT_EQ(0.0f, a.min.x);
    ASSERT_TRUE(prts_aabb_overlaps(a, box(0.5f, 0.5f, 0.5f, 2, 2, 2)));
    ASSERT_FALSE(prts_aabb_overlaps(a, box(1, 0, 0, 2, 1, 1)));
}

UTEST(collide, heightfield_sampling)
{
    prts_heightfield *hf = prts_heightfield_create(3, 3, 2.0f, -2.0f, -2.0f);
    prts_heightfield_set(hf, 1, 1, 4.0f);
    prts_heightfield_set(hf, 9, 9, 99.0f);
    ASSERT_EQ(4.0f, prts_heightfield_get(hf, 1, 1));
    ASSERT_EQ(0.0f, prts_heightfield_get(hf, -5, 50));
    ASSERT_EQ(4.0f, prts_heightfield_sample(hf, 0, 0));
    ASSERT_EQ(2.0f, prts_heightfield_sample(hf, 1, 0));
    ASSERT_EQ(0.0f, prts_heightfield_sample(hf, 100, 100));
    prts_vec3 n = prts_heightfield_normal(hf, 1.0f, 0.0f);
    ASSERT_TRUE(n.x > 0.0f && n.y > 0.0f);
    prts_heightfield_destroy(hf);
    prts_heightfield_destroy(NULL);
}

UTEST(collide, sliding_stepping_and_ground)
{
    prts_cworld *cw = prts_cworld_create();
    int floor = prts_cworld_add_box(cw, box(-10, -1, -10, 10, 0, 10), PRTS_ENTITY_NONE);
    int wall = prts_cworld_add_box(cw, box(2, 0, -10, 3, 3, 10), PRTS_ENTITY_NONE);
    int step = prts_cworld_add_box(cw, box(-3, 0, -10, -2, 0.3f, 10), PRTS_ENTITY_NONE);
    int tall = prts_cworld_add_box(cw, box(-10, 0, 4, 10, 1.0f, 5), PRTS_ENTITY_NONE);
    ASSERT_EQ((size_t)4, prts_cworld_box_count(cw));
    prts_aabb me = box(-0.3f, 0, -0.3f, 0.3f, 1.8f, 0.3f);
    prts_move_result r;
    /* standing on the floor */
    prts_cworld_move(cw, &me, prts_v3(0, -0.1f, 0), 0.4f, &r);
    ASSERT_TRUE(r.on_ground);
    ASSERT_EQ(floor, r.ground_box);
    /* blocked by the wall, with sliding along it */
    prts_vec3 moved = prts_cworld_move(cw, &me, prts_v3(5, 0, 1), 0.4f, &r);
    ASSERT_NEAR(1.7f, moved.x, 1e-3f);
    ASSERT_NEAR(1.0f, moved.z, 1e-3f);
    ASSERT_TRUE(r.hit_wall);
    ASSERT_NEAR(2.0f, me.max.x, 1e-3f);
    /* stepping up onto a low step */
    me = box(-1.0f, 0, -0.3f, -0.4f, 1.8f, 0.3f);
    prts_cworld_move(cw, &me, prts_v3(-1.5f, -0.05f, 0), 0.4f, &r);
    ASSERT_TRUE(r.stepped);
    ASSERT_NEAR(0.3f, me.min.y, 1e-3f);
    /* a 1 m ledge is too high to step */
    me = box(-0.3f, 0, 3.0f, 0.3f, 1.8f, 3.6f);
    prts_cworld_move(cw, &me, prts_v3(0, -0.05f, 1.0f), 0.4f, &r);
    ASSERT_TRUE(r.hit_wall);
    ASSERT_NEAR(4.0f, me.max.z, 1e-3f);
    /* disabled boxes don't block; boxes we already overlap are ignored */
    prts_cworld_set_enabled(cw, wall, false);
    ASSERT_FALSE(prts_cworld_enabled(cw, wall));
    me = box(-0.3f, 0, -0.3f, 0.3f, 1.8f, 0.3f);
    prts_cworld_move(cw, &me, prts_v3(4, 0, 0), 0.0f, &r);
    ASSERT_NEAR(4.3f, me.max.x, 1e-3f);
    ASSERT_FALSE(r.hit_wall);
    prts_cworld_set_enabled(cw, wall, true);
    prts_cworld_move(cw, &me, prts_v3(0.5f, 0, 0), 0.0f, &r);
    ASSERT_NEAR(4.8f, me.max.x, 1e-3f);
    /* moving up into a ceiling */
    int ceil = prts_cworld_add_box(cw, box(-10, 2.5f, -10, 10, 3, 10), PRTS_ENTITY_NONE);
    me = box(-0.3f, 0, -0.3f, 0.3f, 1.8f, 0.3f);
    moved = prts_cworld_move(cw, &me, prts_v3(0, 2.0f, 0), 0.0f, &r);
    ASSERT_NEAR(0.7f, moved.y, 1e-3f);
    ASSERT_FALSE(r.on_ground);
    ASSERT_TRUE(prts_cworld_overlaps(cw, box(0, -0.5f, 0, 1, 0.5f, 1)));
    ASSERT_FALSE(prts_cworld_overlaps(cw, box(0, 1.0f, 0, 1, 2.0f, 1)));
    (void)step;
    (void)tall;
    (void)ceil;
    prts_cworld_move(cw, &me, prts_v3(0, 0, 0), 0.0f, NULL);
    /* unknown ids */
    ASSERT_EQ(PRTS_ENTITY_NONE, prts_cworld_owner(cw, -1));
    ASSERT_FALSE(prts_cworld_enabled(cw, 99));
    prts_cworld_set_enabled(cw, 99, true);
    ASSERT_EQ(0.0f, prts_cworld_box(cw, 99).max.x);
    prts_cworld_destroy(cw);
    prts_cworld_destroy(NULL);
}

UTEST(collide, owned_boxes_follow_entities)
{
    prts_world *w = prts_world_create();
    prts_entity door = prts_world_spawn(w, "door");
    prts_world_set_local(w, door, prts_dv3(5, 0, 0), 0.0f);
    prts_cworld *cw = prts_cworld_create();
    int id = prts_cworld_add_box(cw, box(0, 0, -0.05f, 1.0f, 2.0f, 0.05f), door);
    prts_cworld_sync(cw, w);
    ASSERT_NEAR(5.0f, prts_cworld_box(cw, id).min.x, 1e-5f);
    ASSERT_EQ(door, prts_cworld_owner(cw, id));
    prts_world_set_local(w, door, prts_dv3(5, 0, 0), (float)(PRTS_PI / 2));
    prts_cworld_sync(cw, w);
    ASSERT_NEAR(-1.0f, prts_cworld_box(cw, id).min.z, 1e-4f);
    prts_cworld_set_owner_enabled(cw, door, false);
    ASSERT_FALSE(prts_cworld_enabled(cw, id));
    prts_cworld_destroy(cw);
    prts_world_destroy(w);
}

UTEST(collide, use_boxes_stop_only_the_use_ray)
{
    /* a person: a use-only box the commander can point at and walk through */
    prts_world *w = prts_world_create();
    prts_entity guard = prts_world_spawn(w, "guard");
    prts_world_set_local(w, guard, prts_dv3(3, 0, 0), 0.0f);
    prts_cworld *cw = prts_cworld_create();
    int id = prts_cworld_add_use_box(cw, box(-0.3f, 0, -0.3f, 0.3f, 1.8f, 0.3f), guard);
    int post = prts_cworld_add_use_box(cw, box(-0.2f, 0, 5.8f, 0.2f, 2.0f, 6.2f), PRTS_ENTITY_NONE); /* static */
    prts_cworld_sync(cw, w);
    ASSERT_EQ(guard, prts_cworld_owner(cw, id));
    ASSERT_TRUE(prts_cworld_enabled(cw, id));
    float d = 0.0f;
    ASSERT_EQ(id, prts_cworld_raycast_use(cw, prts_v3(0, 1.5f, 0), prts_v3(1, 0, 0), 5.0f, &d));
    ASSERT_NEAR(2.7f, d, 1e-4f);
    ASSERT_EQ(post, prts_cworld_raycast_use(cw, prts_v3(0, 1.5f, 0), prts_v3(0, 0, 1), 8.0f, &d));
    ASSERT_NEAR(5.8f, d, 1e-4f);
    /* nothing else sees them: rays, occlusion (so no baked shadow), overlaps, ground, movement */
    ASSERT_EQ(-1, prts_cworld_raycast(cw, prts_v3(0, 1.5f, 0), prts_v3(1, 0, 0), 5.0f, NULL));
    ASSERT_EQ(-1, prts_cworld_raycast(cw, prts_v3(0, 1.5f, 0), prts_v3(0, 0, 1), 8.0f, NULL));
    ASSERT_FALSE(prts_cworld_occluded(cw, prts_v3(0, 1.5f, 0), prts_v3(1, 0, 0), 5.0f));
    ASSERT_FALSE(prts_cworld_occluded(cw, prts_v3(0, 1.5f, 0), prts_v3(0, 0, 1), 8.0f));
    ASSERT_FALSE(prts_cworld_overlaps(cw, box(2.9f, 0.5f, -0.1f, 3.1f, 1.0f, 0.1f)));
    float h = 0.0f;
    ASSERT_FALSE(prts_cworld_ground(cw, prts_v3(3, 2.5f, 0), 3.0f, &h));
    prts_aabb me = box(0, 0.1f, -0.3f, 0.6f, 1.9f, 0.3f);
    prts_vec3 moved = prts_cworld_move(cw, &me, prts_v3(5, 0, 0), 0.0f, NULL);
    ASSERT_NEAR(5.0f, moved.x, 1e-4f);
    /* the nearest hit wins: a wall in front hides the person from the use ray */
    int wall = prts_cworld_add_box(cw, box(1, 0, -1, 1.2f, 3, 1), PRTS_ENTITY_NONE);
    ASSERT_EQ(wall, prts_cworld_raycast_use(cw, prts_v3(0, 1.5f, 0), prts_v3(1, 0, 0), 5.0f, &d));
    ASSERT_NEAR(1.0f, d, 1e-4f);
    /* a use box follows its owner and is switched off with it */
    prts_world_set_local(w, guard, prts_dv3(0, 0, -3), 0.0f);
    prts_cworld_sync(cw, w);
    ASSERT_EQ(id, prts_cworld_raycast_use(cw, prts_v3(0, 1.5f, 0), prts_v3(0, 0, -1), 5.0f, &d));
    prts_cworld_set_owner_enabled(cw, guard, false);
    ASSERT_EQ(-1, prts_cworld_raycast_use(cw, prts_v3(0, 1.5f, 0), prts_v3(0, 0, -1), 5.0f, &d));
    prts_cworld_destroy(cw);
    prts_world_destroy(w);
}

UTEST(collide, terrain_and_raycast)
{
    prts_heightfield *hf = prts_heightfield_create(11, 11, 1.0f, -5.0f, -5.0f);
    for (int z = 0; z < 11; z++)
        for (int x = 0; x < 11; x++) prts_heightfield_set(hf, x, z, (float)x * 0.1f);
    prts_cworld *cw = prts_cworld_create();
    prts_cworld_set_heightfield(cw, hf);
    prts_aabb me = box(-0.3f, 1.0f, -0.3f, 0.3f, 2.8f, 0.3f);
    prts_move_result r;
    prts_cworld_move(cw, &me, prts_v3(0, -2.0f, 0), 0.0f, &r);
    ASSERT_TRUE(r.on_ground);
    ASSERT_EQ(-1, r.ground_box);
    ASSERT_NEAR(0.5f, me.min.y, 1e-3f);
    int b = prts_cworld_add_box(cw, box(2, 0, -1, 3, 3, 1), PRTS_ENTITY_NONE);
    float d = 0.0f;
    ASSERT_EQ(b, prts_cworld_raycast(cw, prts_v3(0, 1.5f, 0), prts_v3(1, 0, 0), 5.0f, &d));
    ASSERT_NEAR(2.0f, d, 1e-4f);
    ASSERT_EQ(-1, prts_cworld_raycast(cw, prts_v3(0, 1.5f, 0), prts_v3(-1, 0, 0), 3.0f, &d));
    ASSERT_EQ(-2, prts_cworld_raycast(cw, prts_v3(0, 1.5f, 0), prts_v3(0, -1, 0), 5.0f, &d));
    ASSERT_EQ(-1, prts_cworld_raycast(cw, prts_v3(0, 1.5f, 5), prts_v3(1, 0, 0), 4.0f, NULL));
    ASSERT_EQ(-1, prts_cworld_raycast(cw, prts_v3(0, 10.0f, 0), prts_v3(1, 0.0f, 0), 4.0f, NULL));
    ASSERT_EQ(b, prts_cworld_raycast(cw, prts_v3(5, 1.5f, 0), prts_v3(-1, 0, 0), 4.0f, NULL));
    prts_cworld_destroy(cw);
    prts_heightfield_destroy(hf);
}

static prts_heightfield *plane(float slope)
{
    /* the terrain rises along +x: h = slope * x */
    prts_heightfield *hf = prts_heightfield_create(21, 21, 1.0f, -10.0f, -10.0f);
    for (int z = 0; z < 21; z++) {
        for (int x = 0; x < 21; x++) prts_heightfield_set(hf, x, z, slope * (float)(x - 10));
    }
    return hf;
}

UTEST(collide, slope_limit_on_terrain)
{
    /* Spec: slopes up to 45 degrees can be walked; steeper ones cannot be climbed */
    prts_cworld *cw = prts_cworld_create();
    prts_aabb body = prts_aabb_make(prts_v3(-0.3f, 0.0f, -0.3f), prts_v3(0.3f, 1.8f, 0.3f));
    prts_move_result r;

    prts_heightfield *gentle = plane(tanf(30.0f * (float)PRTS_PI / 180.0f));
    prts_cworld_set_heightfield(cw, gentle);
    prts_aabb b = body;
    prts_vec3 moved = prts_cworld_move(cw, &b, prts_v3(0.5f, 0.0f, 0.0f), 0.4f, &r);
    ASSERT_NEAR(0.5f, moved.x, 1e-4f); /* up a 30-degree slope */
    ASSERT_GT(b.min.y, 0.2f);
    ASSERT_FALSE(r.hit_wall);

    prts_heightfield *steep = plane(tanf(60.0f * (float)PRTS_PI / 180.0f));
    prts_cworld_set_heightfield(cw, steep);
    b = body;
    moved = prts_cworld_move(cw, &b, prts_v3(0.5f, 0.0f, 0.3f), 0.4f, &r);
    ASSERT_NEAR(0.0f, moved.x, 1e-4f); /* no progress up a 60-degree slope */
    ASSERT_NEAR(0.3f, moved.z, 1e-4f); /* but it can move along it */
    ASSERT_TRUE(r.hit_wall);
    b = body;
    moved = prts_cworld_move(cw, &b, prts_v3(-0.5f, 0.0f, 0.0f), 0.4f, &r);
    ASSERT_NEAR(-0.5f, moved.x, 1e-4f); /* and down it */
    /* a mover in the air is not limited (it is not standing on the slope) */
    b = prts_aabb_make(prts_v3(-0.3f, 5.0f, -0.3f), prts_v3(0.3f, 6.8f, 0.3f));
    moved = prts_cworld_move(cw, &b, prts_v3(0.5f, 0.0f, 0.0f), 0.4f, &r);
    ASSERT_NEAR(0.5f, moved.x, 1e-4f);
    /* the limit can be changed */
    prts_cworld_set_max_slope(cw, 70.0f * (float)PRTS_PI / 180.0f);
    b = body;
    moved = prts_cworld_move(cw, &b, prts_v3(0.5f, 0.0f, 0.0f), 0.4f, &r);
    ASSERT_NEAR(0.5f, moved.x, 1e-4f);
    /* standing still changes nothing */
    b = body;
    moved = prts_cworld_move(cw, &b, prts_v3(0.0f, 0.0f, 0.0f), 0.4f, &r);
    ASSERT_NEAR(0.0f, moved.x, 1e-6f);
    prts_cworld_destroy(cw);
    prts_heightfield_destroy(gentle);
    prts_heightfield_destroy(steep);
}

/* Reference: the nearest box hit by a ray, checking every box (what the BVH must reproduce). */
static int brute_raycast(const prts_aabb *boxes, const bool *on, int n, prts_vec3 o, prts_vec3 d, float max_dist,
                         float *dist)
{
    int best = -1;
    float best_t = max_dist;
    for (int i = 0; i < n; i++) {
        if (!on[i]) continue;
        float tmin = 0.0f, tmax = 1e30f;
        bool hit = true;
        const float *oo = &o.x, *dd = &d.x, *lo = &boxes[i].min.x, *hi = &boxes[i].max.x;
        for (int a = 0; a < 3 && hit; a++) {
            if (fabsf(dd[a]) < 1e-8f) {
                hit = oo[a] >= lo[a] && oo[a] <= hi[a];
                continue;
            }
            float t1 = (lo[a] - oo[a]) / dd[a], t2 = (hi[a] - oo[a]) / dd[a];
            if (t1 > t2) {
                float t = t1;
                t1 = t2;
                t2 = t;
            }
            tmin = fmaxf(tmin, t1);
            tmax = fminf(tmax, t2);
            hit = tmin <= tmax;
        }
        if (hit && tmin <= best_t) {
            best_t = tmin;
            best = i;
        }
    }
    *dist = best_t;
    return best;
}

/* A small deterministic generator (a function, so several calls in one expression are fine). */
static float lcg(uint32_t *seed)
{
    *seed = *seed * 1664525u + 1013904223u;
    return (float)(*seed >> 8) / 16777216.0f;
}

UTEST(collide, bvh_matches_checking_every_box)
{
    enum { N = 300 };
    static prts_aabb boxes[N];
    static bool on[N];
    prts_cworld *cw = prts_cworld_create();
    prts_world *w = prts_world_create();
    uint32_t seed = 12345u;
#define RND() lcg(&seed)
    for (int i = 0; i < N; i++) {
        prts_vec3 c = prts_v3(RND() * 200.0f - 100.0f, RND() * 20.0f, RND() * 200.0f - 100.0f);
        prts_vec3 h = prts_v3(0.2f + RND() * 4.0f, 0.2f + RND() * 3.0f, 0.2f + RND() * 4.0f);
        boxes[i] = prts_aabb_make(prts_v3_sub(c, h), prts_v3_add(c, h));
        on[i] = true;
        prts_cworld_add_box(cw, boxes[i], PRTS_ENTITY_NONE);
    }
    /* one box owned by an entity (checked outside the tree), some boxes switched off */
    prts_entity e = prts_world_spawn(w, "mover"); /* stays at the origin, unturned */
    prts_cworld_add_box(cw, boxes[7], e); /* a copy of box 7 that follows e (which stays put) */
    prts_cworld_sync(cw, w);
    for (int i = 0; i < N; i += 17) {
        prts_cworld_set_enabled(cw, i, false);
        on[i] = i == 7; /* box 7 still exists through the owned copy */
    }
    int mismatches = 0, hits = 0;
    for (int r = 0; r < 400; r++) {
        prts_vec3 o = prts_v3(RND() * 240.0f - 120.0f, RND() * 30.0f, RND() * 240.0f - 120.0f);
        prts_vec3 d = prts_v3_normalize(prts_v3(RND() - 0.5f, (RND() - 0.5f) * 0.3f, RND() - 0.5f));
        float want_t, got_t;
        int want = brute_raycast(boxes, on, N, o, d, 150.0f, &want_t);
        int got = prts_cworld_raycast(cw, o, d, 150.0f, &got_t);
        if (got == N) got = 7; /* the owned copy of box 7 */
        if (want != got || fabsf(want_t - got_t) > 1e-4f) mismatches++;
        if (want >= 0) hits++;
        if (prts_cworld_occluded(cw, o, d, 150.0f) != (want >= 0)) mismatches++;
        prts_aabb probe = prts_aabb_make(prts_v3_sub(o, prts_v3(1, 1, 1)), prts_v3_add(o, prts_v3(1, 1, 1)));
        bool any = false;
        for (int i = 0; i < N; i++) any = any || (on[i] && prts_aabb_overlaps(probe, boxes[i]));
        if (prts_cworld_overlaps(cw, probe) != any) mismatches++;
    }
#undef RND
    ASSERT_EQ(0, mismatches);
    ASSERT_GT(hits, 50);
    /* adding a box after queries rebuilds the tree */
    prts_cworld_add_box(cw, prts_aabb_make(prts_v3(500, 0, 500), prts_v3(501, 1, 501)), PRTS_ENTITY_NONE);
    ASSERT_TRUE(prts_cworld_overlaps(cw, prts_aabb_make(prts_v3(500.5f, 0.5f, 500.5f), prts_v3(500.6f, 0.6f, 500.6f))));
    prts_cworld_destroy(cw);
    prts_world_destroy(w);
}
