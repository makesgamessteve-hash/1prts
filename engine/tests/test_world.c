/* Unit tests: clock, entities, reference frames, movers. MIT License. */
#include <math.h>

#include "prts/prts_world.h"
#include "utest.h"

UTEST(clock, fixed_ticks_independent_of_frame_rate)
{
    /* Spec: Different frame rates, same simulation */
    prts_clock a, b;
    prts_clock_init(&a, 30, 5);
    prts_clock_init(&b, 30, 5);
    int ticks_a = 0, ticks_b = 0;
    for (int i = 0; i < 300; i++) ticks_a += prts_clock_advance(&a, 1.0 / 30.0);
    for (int i = 0; i < 1440; i++) ticks_b += prts_clock_advance(&b, 1.0 / 144.0);
    ASSERT_TRUE(abs(ticks_a - 300) <= 1);
    ASSERT_TRUE(abs(ticks_b - 300) <= 1);
    ASSERT_EQ((uint64_t)0, a.tick); /* counting ticks is the caller's job */
    double alpha = prts_clock_alpha(&b);
    ASSERT_TRUE(alpha >= 0.0 && alpha < 1.0);
}

UTEST(clock, slow_frame_is_capped)
{
    /* Spec: Very slow frame */
    prts_clock c;
    prts_clock_init(&c, 30, 5);
    ASSERT_EQ(5, prts_clock_advance(&c, 1.0));
    ASSERT_TRUE(prts_clock_alpha(&c) < 1.0);
    ASSERT_EQ(0, prts_clock_advance(&c, -1.0));
    ASSERT_EQ(0, prts_clock_advance(&c, 0.0));
}

UTEST(world, spawn_names_and_slot_reuse)
{
    /* Spec: Slot reuse */
    prts_world *w = prts_world_create();
    prts_entity a = prts_world_spawn(w, "door");
    prts_entity b = prts_world_spawn(w, NULL);
    ASSERT_TRUE(prts_world_alive(w, a));
    ASSERT_EQ((size_t)2, prts_world_count(w));
    ASSERT_STREQ("door", prts_world_name(w, a));
    ASSERT_STREQ("", prts_world_name(w, b));
    ASSERT_EQ(a, prts_world_find(w, "door"));
    ASSERT_EQ(PRTS_ENTITY_NONE, prts_world_find(w, "nothing"));
    prts_world_despawn(w, a);
    prts_world_despawn(w, a);
    ASSERT_FALSE(prts_world_alive(w, a));
    prts_entity c = prts_world_spawn(w, "lift");
    ASSERT_NE(a, c);
    ASSERT_EQ(a & 0xffffffffu, c & 0xffffffffu);
    ASSERT_FALSE(prts_world_alive(w, a));
    ASSERT_TRUE(prts_world_alive(w, c));
    ASSERT_FALSE(prts_world_alive(w, PRTS_ENTITY_NONE));
    ASSERT_STREQ("", prts_world_name(w, a));
    prts_world_destroy(w);
    prts_world_destroy(NULL);
}

static int g_count;
static void count_cb(prts_entity e, void *user)
{
    (void)e;
    (void)user;
    g_count++;
}

UTEST(world, despawn_children_and_each)
{
    prts_world *w = prts_world_create();
    prts_entity ship = prts_world_spawn(w, "galleon");
    prts_entity cannon = prts_world_spawn(w, "cannon");
    prts_entity crew = prts_world_spawn(w, "gunner");
    prts_entity other = prts_world_spawn(w, "tree");
    ASSERT_TRUE(prts_world_set_parent(w, cannon, ship));
    ASSERT_TRUE(prts_world_set_parent(w, crew, cannon));
    g_count = 0;
    prts_world_each(w, count_cb, NULL);
    ASSERT_EQ(4, g_count);
    prts_world_despawn(w, ship);
    ASSERT_FALSE(prts_world_alive(w, crew));
    ASSERT_TRUE(prts_world_alive(w, other));
    ASSERT_EQ((size_t)1, prts_world_count(w));
    prts_world_destroy(w);
}

UTEST(world, nested_frames_and_reparenting)
{
    prts_world *w = prts_world_create();
    prts_entity lift = prts_world_spawn(w, "lift");
    prts_entity cmdr = prts_world_spawn(w, "commander");
    prts_world_set_local(w, lift, prts_dv3(10, 0, 5), 0.0f);
    prts_world_set_local(w, cmdr, prts_dv3(10.5, 0.2, 5), 0.0f);
    prts_world_set_velocity(w, cmdr, prts_v3(1, 0, 0));
    /* Spec: Changing parent without a jump */
    ASSERT_TRUE(prts_world_set_parent(w, cmdr, lift));
    ASSERT_TRUE(prts_world_set_parent(w, cmdr, lift));
    ASSERT_NEAR(10.5, prts_world_pos(w, cmdr).x, 1e-9);
    ASSERT_NEAR(0.5, prts_world_local_pos(w, cmdr).x, 1e-9);
    ASSERT_EQ(1.0f, prts_world_velocity(w, cmdr).x);
    ASSERT_EQ(lift, prts_world_parent(w, cmdr));
    /* Spec: Riding a moving frame */
    prts_world_set_local(w, lift, prts_dv3(10, 10, 5), 0.0f);
    ASSERT_NEAR(10.2, prts_world_pos(w, cmdr).y, 1e-9);
    ASSERT_NEAR(0.2, prts_world_local_pos(w, cmdr).y, 1e-9);
    /* rotation of the parent carries the child around */
    prts_world_set_local(w, lift, prts_dv3(10, 10, 5), (float)(PRTS_PI / 2));
    ASSERT_NEAR(10.0, prts_world_pos(w, cmdr).x, 1e-6);
    ASSERT_NEAR(4.5, prts_world_pos(w, cmdr).z, 1e-6);
    ASSERT_NEAR(PRTS_PI / 2, prts_world_yaw(w, cmdr), 1e-6);
    ASSERT_EQ(0.0f, prts_world_local_yaw(w, cmdr));
    /* cycles and dead parents are refused */
    ASSERT_FALSE(prts_world_set_parent(w, lift, cmdr));
    ASSERT_FALSE(prts_world_set_parent(w, lift, lift));
    prts_entity dead = prts_world_spawn(w, NULL);
    prts_world_despawn(w, dead);
    ASSERT_FALSE(prts_world_set_parent(w, cmdr, dead));
    ASSERT_FALSE(prts_world_set_parent(w, dead, lift));
    /* back to the world frame keeps the world pose */
    prts_dvec3 before = prts_world_pos(w, cmdr);
    ASSERT_TRUE(prts_world_set_parent(w, cmdr, PRTS_ENTITY_NONE));
    ASSERT_NEAR(before.x, prts_world_pos(w, cmdr).x, 1e-9);
    ASSERT_NEAR(PRTS_PI / 2, prts_world_local_yaw(w, cmdr), 1e-6);
    prts_world_set_pos(w, cmdr, prts_dv3(1, 2, 3));
    ASSERT_NEAR(3.0, prts_world_local_pos(w, cmdr).z, 1e-9);
    /* operations on dead handles are harmless */
    prts_world_set_local(w, dead, prts_dv3(1, 1, 1), 0);
    prts_world_set_pos(w, dead, prts_dv3(1, 1, 1));
    prts_world_set_velocity(w, dead, prts_v3(1, 1, 1));
    ASSERT_EQ(0.0, prts_world_pos(w, dead).x);
    ASSERT_EQ(0.0, prts_world_local_pos(w, dead).x);
    ASSERT_EQ(0.0f, prts_world_local_yaw(w, dead));
    ASSERT_EQ(0.0f, prts_world_yaw(w, dead));
    ASSERT_EQ(0.0f, prts_world_velocity(w, dead).x);
    ASSERT_EQ(PRTS_ENTITY_NONE, prts_world_parent(w, dead));
    ASSERT_EQ(0.0, prts_world_pos_interp(w, dead, 0.5).x);
    ASSERT_EQ(0.0f, prts_world_yaw_interp(w, dead, 0.5));
    prts_world_destroy(w);
}

UTEST(world, interpolation_between_ticks)
{
    /* Spec: High refresh display */
    prts_world *w = prts_world_create();
    prts_entity car = prts_world_spawn(w, "lift");
    prts_world_begin_tick(w);
    prts_world_set_local(w, car, prts_dv3(0, 3, 0), 0.4f);
    ASSERT_NEAR(0.75, prts_world_pos_interp(w, car, 0.25).y, 1e-9);
    ASSERT_NEAR(2.25, prts_world_pos_interp(w, car, 0.75).y, 1e-9);
    ASSERT_NEAR(0.2f, prts_world_yaw_interp(w, car, 0.5), 1e-6f);
    prts_world_destroy(w);
}

static int g_done;
static void done_cb(prts_entity e, void *user)
{
    (void)e;
    (void)user;
    g_done++;
}

UTEST(world, movers_and_usable)
{
    prts_world *w = prts_world_create();
    prts_entity lift = prts_world_spawn(w, "lift");
    prts_entity door = prts_world_spawn(w, "door");
    prts_world_move_to(w, lift, prts_dv3(0, 8, 0), 2.0f);
    prts_world_rotate_to(w, door, -1.5f, 3.0f);
    ASSERT_TRUE(prts_world_moving(w, lift));
    g_done = 0;
    for (int i = 0; i < 30; i++) prts_world_tick_movers(w, 1.0f / 30.0f, done_cb, NULL);
    ASSERT_NEAR(2.0, prts_world_local_pos(w, lift).y, 1e-6);
    ASSERT_FALSE(prts_world_moving(w, door));
    ASSERT_EQ(-1.5f, prts_world_local_yaw(w, door));
    ASSERT_EQ(1, g_done);
    for (int i = 0; i < 200; i++) prts_world_tick_movers(w, 1.0f / 30.0f, done_cb, NULL);
    ASSERT_EQ(8.0, prts_world_local_pos(w, lift).y);
    ASSERT_EQ(2, g_done);
    prts_world_rotate_to(w, door, 0.5f, 100.0f);
    prts_world_tick_movers(w, 1.0f / 30.0f, NULL, NULL);
    ASSERT_EQ(0.5f, prts_world_local_yaw(w, door));
    prts_world_move_to(w, lift, prts_world_local_pos(w, lift), 1.0f);
    prts_world_tick_movers(w, 1.0f / 30.0f, NULL, NULL);
    ASSERT_FALSE(prts_world_moving(w, lift));
    prts_world_set_usable(w, door, true);
    ASSERT_TRUE(prts_world_usable(w, door));
    ASSERT_FALSE(prts_world_usable(w, lift));
    prts_world_despawn(w, lift);
    prts_world_move_to(w, lift, prts_dv3(0, 0, 0), 1.0f);
    prts_world_rotate_to(w, lift, 0.0f, 1.0f);
    prts_world_set_usable(w, lift, true);
    ASSERT_FALSE(prts_world_moving(w, lift));
    ASSERT_FALSE(prts_world_usable(w, lift));
    prts_world_destroy(w);
}

UTEST(world, state_hash_detects_divergence)
{
    /* Spec: Divergence is detected */
    prts_world *a = prts_world_create(), *b = prts_world_create();
    prts_entity ea = prts_world_spawn(a, "x"), eb = prts_world_spawn(b, "x");
    prts_world_move_to(a, ea, prts_dv3(1, 0, 0), 1.0f);
    prts_world_move_to(b, eb, prts_dv3(1, 0, 0), 1.0f);
    prts_world_rotate_to(a, ea, 1.0f, 1.0f);
    prts_world_rotate_to(b, eb, 1.0f, 1.0f);
    ASSERT_EQ(prts_world_hash(a), prts_world_hash(b));
    prts_world_set_local(b, eb, prts_dv3(0.001, 0, 0), 0.0f);
    ASSERT_NE(prts_world_hash(a), prts_world_hash(b));
    prts_entity dead = prts_world_spawn(a, NULL);
    prts_world_despawn(a, dead);
    (void)prts_world_hash(a);
    prts_world_destroy(a);
    prts_world_destroy(b);
}
