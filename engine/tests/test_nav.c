/* Unit tests: navigation grid, paths and agents. MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_nav.h"
#include "prts/prts_rng.h"
#include "prts/prts_serial.h"
#include "utest.h"

static prts_aabb box(float x0, float y0, float z0, float x1, float y1, float z1)
{
    return prts_aabb_make(prts_v3(x0, y0, z0), prts_v3(x1, y1, z1));
}

/* A 60 x 60 m floor with a wall across the middle (x = -1..1, z = -20..20). */
static prts_cworld *walled(void)
{
    prts_cworld *cw = prts_cworld_create();
    prts_cworld_add_box(cw, box(-30, -1, -30, 30, 0, 30), PRTS_ENTITY_NONE);
    prts_cworld_add_box(cw, box(-1, 0, -20, 1, 3, 20), PRTS_ENTITY_NONE);
    return cw;
}

/* Every point of the path stays off the wall and every leg is a clear line. */
static bool path_ok(const prts_nav *n, float x0, float z0, const float *p, int count)
{
    float px = x0, pz = z0;
    for (int i = 0; i < count; i++) {
        if (!prts_nav_clear_line(n, px, pz, p[2 * i], p[2 * i + 1])) return false;
        px = p[2 * i];
        pz = p[2 * i + 1];
    }
    return true;
}

UTEST(nav, paths_go_around_walls_and_buildings)
{
    /* Spec (navigation): Around a wall; A building blocks the way */
    prts_cworld *cw = walled();
    prts_nav *n = prts_nav_build(cw, -30, -30, 30, 30, 1.0f);
    ASSERT_TRUE(n != NULL);
    ASSERT_EQ(60, prts_nav_cols(n));
    ASSERT_EQ(60, prts_nav_rows(n));
    ASSERT_EQ(1.0f, prts_nav_cell(n));
    ASSERT_FALSE(prts_nav_walkable(n, 0.5f, 0.0f)); /* the wall */
    ASSERT_TRUE(prts_nav_walkable(n, -10.5f, 0.0f));
    ASSERT_FALSE(prts_nav_clear_line(n, -10, 0, 10, 0));
    float p[512], q[512];
    int count = prts_nav_path(n, -10, 0, 10, 0, p, 256);
    ASSERT_GT(count, 1);
    ASSERT_TRUE(path_ok(n, -10, 0, p, count));
    ASSERT_EQ(10.0f, p[2 * (count - 1)]); /* ends on the goal */
    bool around = false;
    for (int i = 0; i < count; i++) around = around || fabsf(p[2 * i + 1]) >= 20.0f;
    ASSERT_TRUE(around);
    /* the same request, the same path */
    ASSERT_EQ(count, prts_nav_path(n, -10, 0, 10, 0, q, 256));
    ASSERT_EQ(0, memcmp(p, q, (size_t)count * 2 * sizeof(float)));
    /* a building across the southern way round: the path takes the north */
    prts_hasher h0, h1;
    prts_hasher_init(&h0);
    prts_nav_hash(n, &h0);
    float south = 0.0f;
    for (int i = 0; i < count; i++) south = fmaxf(south, p[2 * i + 1]);
    float side = south > 0.0f ? 1.0f : -1.0f;
    ASSERT_GT(prts_nav_block(n, -2, side * 19, 2, side * 30, 1), 0);
    prts_hasher_init(&h1);
    prts_nav_hash(n, &h1);
    ASSERT_NE(h0.h, h1.h);
    count = prts_nav_path(n, -10, 0, 10, 0, q, 256);
    ASSERT_GT(count, 1);
    ASSERT_TRUE(path_ok(n, -10, 0, q, count));
    for (int i = 0; i < count; i++) ASSERT_TRUE(q[2 * i + 1] * side < 19.0f);
    /* the building gone: the first path again */
    prts_nav_block(n, -2, side * 19, 2, side * 30, -1);
    prts_nav_block(n, -2, side * 19, 2, side * 30, -1); /* never below zero */
    int again = prts_nav_path(n, -10, 0, 10, 0, q, 256);
    ASSERT_EQ(again, prts_nav_path(n, -10, 0, 10, 0, p, 256));
    /* a start inside the wall moves to the nearest walkable cell; the goal in the wall too */
    count = prts_nav_path(n, 0.2f, 0.0f, 0.4f, 5.0f, p, 256);
    ASSERT_GT(count, 0);
    ASSERT_FALSE(prts_nav_walkable(n, 0.4f, 5.0f));
    ASSERT_TRUE(prts_nav_walkable(n, p[2 * (count - 1)], p[2 * (count - 1) + 1]));
    /* already there */
    ASSERT_EQ(1, prts_nav_path(n, -10, 0, -10.2f, 0.1f, p, 256));
    ASSERT_EQ(-10.2f, p[0]);
    /* outside the grid, or no room for points */
    ASSERT_EQ(-1, prts_nav_path(n, -100, 0, 10, 0, p, 256));
    ASSERT_EQ(-1, prts_nav_path(n, -10, 0, 10, 0, p, 0));
    ASSERT_FALSE(prts_nav_walkable(n, 100, 0));
    float hgt = 7.0f;
    ASSERT_TRUE(prts_nav_height(n, -10, 0, &hgt));
    ASSERT_EQ(0.0f, hgt);
    ASSERT_FALSE(prts_nav_height(n, 0.5f, 0.0f, &hgt));
    ASSERT_FALSE(prts_nav_height(n, -100, 0, &hgt));
    prts_nav_destroy(n);
    prts_nav_destroy(NULL);
    /* bad sizes */
    ASSERT_TRUE(prts_nav_build(cw, 0, 0, 10, 10, 0.0f) == NULL);
    ASSERT_TRUE(prts_nav_build(cw, 10, 0, 0, 10, 1.0f) == NULL);
    ASSERT_TRUE(prts_nav_build(cw, 0, 0, 100000, 100000, 1.0f) == NULL);
    prts_cworld_destroy(cw);
}

UTEST(nav, cliffs_islands_and_saved_blocks)
{
    /* a raised block 3 m high in the middle: units walk at ground level, so it is solid */
    prts_cworld *cw = prts_cworld_create();
    prts_cworld_add_box(cw, box(-20, -1, -20, 20, 0, 20), PRTS_ENTITY_NONE);
    prts_cworld_add_box(cw, box(-4, 0, -4, 4, 3, 4), PRTS_ENTITY_NONE);
    prts_nav *n = prts_nav_build(cw, -20, -20, 20, 20, 1.0f);
    float h = 0.0f, p[512];
    ASSERT_FALSE(prts_nav_height(n, 0, 0, &h));
    ASSERT_FALSE(prts_nav_walkable(n, 0, 0));
    ASSERT_TRUE(prts_nav_height(n, -10, -10, &h));
    ASSERT_EQ(0.0f, h);
    ASSERT_FALSE(prts_nav_clear_line(n, -10, 0.5f, 0.5f, 0.5f));
    /* a cell with nothing under it (outside the floor) is solid */
    prts_nav *edge = prts_nav_build(cw, 15, 15, 25, 25, 1.0f);
    ASSERT_FALSE(prts_nav_walkable(edge, 22, 22));
    ASSERT_TRUE(prts_nav_walkable(edge, 17, 17));
    ASSERT_GT(prts_nav_path(edge, 17, 17, 23, 23, p, 256), 0); /* the goal snapped back onto the floor */
    /* blocks are written and read back as runs; a grid of another size refuses them */
    prts_nav_block(n, -10, -10, -5, -5, 2);
    prts_writer w = {0};
    prts_nav_write(n, &w);
    prts_nav *other = prts_nav_build(cw, -20, -20, 20, 20, 1.0f);
    prts_reader r;
    prts_reader_init(&r, w.data, w.len);
    ASSERT_TRUE(prts_nav_read(other, &r, false));
    ASSERT_TRUE(prts_nav_walkable(other, -7, -7)); /* only checked */
    prts_reader_init(&r, w.data, w.len);
    ASSERT_TRUE(prts_nav_read(other, &r, true));
    ASSERT_FALSE(prts_nav_walkable(other, -7, -7));
    prts_hasher a, b;
    prts_hasher_init(&a);
    prts_hasher_init(&b);
    prts_nav_hash(n, &a);
    prts_nav_hash(other, &b);
    ASSERT_EQ(a.h, b.h);
    prts_reader_init(&r, w.data, w.len);
    ASSERT_FALSE(prts_nav_read(edge, &r, true));
    prts_reader_init(&r, w.data, w.len - 12); /* cut short */
    ASSERT_FALSE(prts_nav_read(other, &r, true));
    /* a run longer than the grid */
    prts_writer bad = {0};
    prts_w_u32(&bad, 40);
    prts_w_u32(&bad, 40);
    prts_w_u32(&bad, 1);
    prts_w_u32(&bad, 0);
    prts_w_u32(&bad, 999999);
    prts_reader_init(&r, bad.data, bad.len);
    ASSERT_FALSE(prts_nav_read(other, &r, true));
    prts_writer_free(&bad);
    prts_writer_free(&w);
    prts_nav_destroy(edge);
    prts_nav_destroy(other);
    prts_nav_destroy(n);
    prts_cworld_destroy(cw);
}

UTEST(nav, terrain_slopes_and_cliffs)
{
    /* a heightfield: a gentle ramp in the west half, a 6 m cliff along x = 10 */
    prts_cworld *cw = prts_cworld_create();
    prts_heightfield *hf = prts_heightfield_create(41, 41, 1.0f, -20.0f, -20.0f);
    for (int iz = 0; iz < 41; iz++) {
        for (int ix = 0; ix < 41; ix++) {
            float x = (float)ix - 20.0f;
            prts_heightfield_set(hf, ix, iz, x < 10.0f ? (x + 20.0f) * 0.2f : 12.0f);
        }
    }
    prts_cworld_set_heightfield(cw, hf);
    prts_nav *n = prts_nav_build(cw, -20, -20, 20, 20, 1.0f);
    float p[512], h = 0.0f;
    ASSERT_GT(prts_nav_path(n, -18, 0, 8, 0, p, 256), 0); /* up the ramp */
    ASSERT_TRUE(prts_nav_height(n, 0.5f, 0.5f, &h)); /* halfway up the ramp */
    ASSERT_NEAR(4.1f, h, 0.05f);
    ASSERT_EQ(-1, prts_nav_path(n, -18, 0, 15, 0, p, 256)); /* not up the cliff */
    ASSERT_FALSE(prts_nav_clear_line(n, 8, 0, 15, 0));
    prts_nav_destroy(n);
    prts_cworld_destroy(cw);
    prts_heightfield_destroy(hf);
}

UTEST(nav, many_paths_reuse_the_search)
{
    prts_cworld *cw = walled();
    prts_nav *n = prts_nav_build(cw, -30, -30, 30, 30, 0.5f);
    float p[512];
    for (int i = 0; i < 200; i++) {
        float z = (float)(i % 40) - 20.0f;
        ASSERT_GT(prts_nav_path(n, -25, z, 25, -z, p, 256), 0);
    }
    prts_nav_destroy(n);
    prts_cworld_destroy(cw);
}

static int run(prts_agents *a, prts_world *w, const prts_nav *n, int ticks, int *arrived)
{
    prts_entity got[64];
    int t = 0;
    for (; t < ticks; t++) {
        prts_world_begin_tick(w);
        *arrived += prts_agents_tick(a, w, n, 1.0f / 30.0f, got, 64);
    }
    return t;
}

UTEST(agents, eight_soldiers_reach_one_point_apart)
{
    /* Spec (navigation): Eight soldiers to one point */
    prts_cworld *cw = walled();
    prts_nav *n = prts_nav_build(cw, -30, -30, 30, 30, 1.0f);
    prts_world *w = prts_world_create();
    prts_agents *a = prts_agents_create();
    prts_entity e[8];
    for (int i = 0; i < 8; i++) {
        char name[16];
        snprintf(name, sizeof(name), "soldier%d", i);
        e[i] = prts_world_spawn(w, name);
        prts_world_set_local(w, e[i], prts_dv3(-12.0 + (i % 4) * 0.8, 0, -2.0 + (i / 4) * 0.8), 0.0f);
        ASSERT_TRUE(prts_agents_add(a, e[i], 3.0f, 0.35f));
    }
    ASSERT_EQ((size_t)8, prts_agents_count(a));
    float p[512];
    for (int i = 0; i < 8; i++) {
        prts_dvec3 at = prts_world_pos(w, e[i]);
        int count = prts_nav_path(n, (float)at.x, (float)at.z, 12, 0, p, 256);
        ASSERT_GT(count, 0);
        ASSERT_TRUE(prts_agents_follow(a, e[i], p, count));
    }
    prts_agent_info info;
    ASSERT_TRUE(prts_agents_info(a, e[0], &info));
    ASSERT_TRUE(info.moving);
    ASSERT_EQ(12.0f, info.tx);
    ASSERT_GT(info.left, 0);
    int arrived = 0;
    run(a, w, n, 30 * 40, &arrived);
    ASSERT_EQ(8, arrived);
    for (int i = 0; i < 8; i++) {
        prts_dvec3 pi = prts_world_pos(w, e[i]);
        ASSERT_LT(fabs(pi.x - 12.0) + fabs(pi.z), 4.0);
        ASSERT_TRUE(prts_nav_walkable(n, (float)pi.x, (float)pi.z));
        for (int j = i + 1; j < 8; j++) {
            prts_dvec3 pj = prts_world_pos(w, e[j]);
            double d = sqrt((pi.x - pj.x) * (pi.x - pj.x) + (pi.z - pj.z) * (pi.z - pj.z));
            ASSERT_GT(d, 0.5); /* apart: no two on top of each other */
        }
    }
    ASSERT_TRUE(prts_agents_info(a, e[0], &info));
    ASSERT_FALSE(info.moving);
    ASSERT_EQ(0, info.left);
    prts_agents_destroy(a);
    prts_world_destroy(w);
    prts_nav_destroy(n);
    prts_cworld_destroy(cw);
}

UTEST(agents, queries_speed_removal_and_saves)
{
    prts_world *w = prts_world_create();
    prts_agents *a = prts_agents_create();
    prts_entity e1 = prts_world_spawn(w, "a"), e2 = prts_world_spawn(w, "b"), e3 = prts_world_spawn(w, "c");
    prts_world_set_local(w, e1, prts_dv3(5, 0, 0), 0.0f);
    prts_world_set_local(w, e2, prts_dv3(1, 0, 0), 0.0f);
    prts_world_set_local(w, e3, prts_dv3(-1, 0, 0), 0.0f);
    ASSERT_FALSE(prts_agents_add(a, PRTS_ENTITY_NONE, 1, 1));
    ASSERT_FALSE(prts_agents_add(a, e1, -1, 1));
    ASSERT_FALSE(prts_agents_add(a, e1, 1, 0));
    ASSERT_TRUE(prts_agents_add(a, e3, 2.0f, 0.4f)); /* added out of order: kept sorted */
    ASSERT_TRUE(prts_agents_add(a, e1, 2.0f, 0.4f));
    ASSERT_TRUE(prts_agents_add(a, e2, 2.0f, 0.4f));
    ASSERT_TRUE(prts_agents_add(a, e2, 3.0f, 0.5f)); /* again: changes it */
    prts_agent_info info;
    ASSERT_TRUE(prts_agents_info(a, e2, &info));
    ASSERT_EQ(3.0f, info.speed);
    ASSERT_EQ(0.5f, info.radius);
    ASSERT_TRUE(prts_agents_set_speed(a, e2, 4.0f));
    ASSERT_FALSE(prts_agents_set_speed(a, e2, -1.0f));
    ASSERT_FALSE(prts_agents_set_speed(a, 12345, 1.0f));
    ASSERT_FALSE(prts_agents_follow(a, 12345, NULL, 0));
    ASSERT_FALSE(prts_agents_info(a, 12345, &info));
    /* nearest first; equal distances by entity */
    prts_entity out[8];
    ASSERT_EQ(2, prts_agents_near(a, w, 0, 0, 2, out, 8));
    ASSERT_EQ(e2 < e3 ? e2 : e3, out[0]);
    ASSERT_EQ(3, prts_agents_near(a, w, 0, 0, 10, out, 8));
    ASSERT_EQ(e1, out[2]);
    ASSERT_EQ(1, prts_agents_near(a, w, 0, 0, 10, out, 1));
    ASSERT_EQ(0, prts_agents_near(a, w, 0, 0, 10, out, 0));
    /* walking without a grid: straight there; two on the same spot are parted */
    float to[2] = {5.0f, 0.0f};
    prts_world_set_local(w, e3, prts_dv3(5, 0, 0), 0.0f);
    ASSERT_TRUE(prts_agents_follow(a, e2, to, 1));
    int arrived = 0;
    run(a, w, NULL, 60, &arrived);
    ASSERT_EQ(1, arrived);
    prts_dvec3 p1 = prts_world_pos(w, e1), p3 = prts_world_pos(w, e3);
    ASSERT_GT(fabs(p1.x - p3.x) + fabs(p1.z - p3.z), 0.3);
    /* saves: the same agents and paths come back; damaged data is refused */
    float path[4] = {9, 9, 10, 10};
    prts_agents_follow(a, e1, path, 2);
    prts_writer wr = {0};
    prts_agents_write(a, &wr);
    prts_agents *b = prts_agents_create();
    prts_reader r;
    prts_reader_init(&r, wr.data, wr.len);
    ASSERT_TRUE(prts_agents_read(b, &r));
    prts_hasher ha, hb;
    prts_hasher_init(&ha);
    prts_hasher_init(&hb);
    prts_agents_hash(a, &ha);
    prts_agents_hash(b, &hb);
    ASSERT_EQ(ha.h, hb.h);
    ASSERT_TRUE(prts_agents_info(b, e1, &info));
    ASSERT_TRUE(info.moving);
    ASSERT_EQ(10.0f, info.tx);
    prts_reader_init(&r, wr.data, wr.len - 3);
    ASSERT_FALSE(prts_agents_read(b, &r));
    ASSERT_EQ((size_t)3, prts_agents_count(b)); /* unchanged by the failed read */
    prts_writer_free(&wr);
    /* agents of despawned entities go; removing */
    prts_world_despawn(w, e3);
    run(a, w, NULL, 1, &arrived);
    ASSERT_EQ((size_t)2, prts_agents_count(a));
    ASSERT_TRUE(prts_agents_remove(a, e2));
    ASSERT_FALSE(prts_agents_remove(a, e2));
    ASSERT_TRUE(prts_agents_follow(a, e1, NULL, 0));
    ASSERT_TRUE(prts_agents_remove(a, e1));
    ASSERT_EQ(0, prts_agents_tick(a, w, NULL, 1.0f / 30.0f, out, 8));
    ASSERT_EQ(0, prts_agents_near(a, w, 0, 0, 10, out, 8));
    prts_agents_destroy(b);
    prts_agents_destroy(a);
    prts_agents_destroy(NULL);
    prts_world_destroy(w);
}
