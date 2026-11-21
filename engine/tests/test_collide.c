/* Unit tests: collision world. MIT License. */
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
