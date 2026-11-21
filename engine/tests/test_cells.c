/* Unit tests: interior cells and portals. MIT License. */
#include <string.h>

#include "prts/prts_scene.h"
#include "utest.h"

static prts_aabb box(float x0, float y0, float z0, float x1, float y1, float z1)
{
    return prts_aabb_make(prts_v3(x0, y0, z0), prts_v3(x1, y1, z1));
}

/* A hall (x -10..10, z -10..0) with a side room (x -10..10, z -20..-10) behind a door in the
 * wall at z = -10, and a window from the hall to the outside at z = 0. */
static prts_scene *two_rooms(int *hall, int *side)
{
    prts_scene *s = prts_scene_create();
    *hall = prts_scene_add_cell(s, "hall", box(-10, 0, -10, 10, 4, 0));
    *side = prts_scene_add_cell(s, "side", box(-10, 0, -20, 10, 4, -10));
    prts_scene_add_portal(s, "door", *hall, *side, box(-6, 0, -10.2f, -5, 2.5f, -9.8f), false);
    prts_scene_add_portal(s, "window", 0, *hall, box(1, 1, -0.2f, 2, 2, 0.2f), true);
    return s;
}

static prts_mat4 looking(float yaw)
{
    return prts_mat4_mul(prts_mat4_perspective(1.2f, 16.0f / 9.0f, 0.05f, 1000.0f), prts_mat4_view_yaw_pitch(yaw, 0));
}

UTEST(cells, closed_door_hides_a_room)
{
    /* Spec: Closed door hides a room */
    int hall, side;
    prts_scene *s = two_rooms(&hall, &side);
    prts_vec3 eye = prts_v3(0, 1.7f, -3);
    ASSERT_EQ(hall, prts_scene_cell_at(s, eye));
    prts_cell_mask vis = prts_scene_visible_cells(s, eye, looking(0)); /* facing the door (-z) */
    ASSERT_TRUE(vis & PRTS_CELL_BIT(hall));
    ASSERT_FALSE(vis & PRTS_CELL_BIT(side));
    ASSERT_TRUE(prts_scene_set_portal(s, "door", true));
    ASSERT_TRUE(prts_scene_portal_open(s, "door"));
    vis = prts_scene_visible_cells(s, eye, looking(0));
    ASSERT_TRUE(vis & PRTS_CELL_BIT(side));
    /* turned away from the door, the side room is out of view even when open */
    vis = prts_scene_visible_cells(s, eye, looking((float)PRTS_PI));
    ASSERT_FALSE(vis & PRTS_CELL_BIT(side));
    ASSERT_TRUE(vis & PRTS_CELL_BIT(0)); /* the window is behind us now, towards +z */
    ASSERT_FALSE(prts_scene_set_portal(s, "nope", true));
    ASSERT_FALSE(prts_scene_portal_open(s, "nope"));
    prts_scene_destroy(s);
}

UTEST(cells, outside_sees_in_through_openings)
{
    int hall, side;
    prts_scene *s = two_rooms(&hall, &side);
    prts_scene_set_portal(s, "door", true);
    prts_vec3 eye = prts_v3(1.5f, 1.5f, 8); /* outside, looking at the window */
    ASSERT_EQ(0, prts_scene_cell_at(s, eye));
    prts_cell_mask vis = prts_scene_visible_cells(s, eye, looking(0));
    ASSERT_TRUE(vis & PRTS_CELL_BIT(hall));
    /* the door is not inside the window's narrow screen rectangle seen from here */
    ASSERT_FALSE(vis & PRTS_CELL_BIT(side));
    /* standing in the window opening, the rectangle is the whole screen */
    vis = prts_scene_visible_cells(s, prts_v3(1.5f, 1.5f, 0.1f), looking(0));
    ASSERT_TRUE(vis & PRTS_CELL_BIT(side));
    prts_scene_destroy(s);
}

UTEST(cells, names_reachability_and_limits)
{
    int hall, side;
    prts_scene *s = two_rooms(&hall, &side);
    ASSERT_EQ(0, prts_scene_find_cell(s, "outside"));
    ASSERT_EQ(side, prts_scene_find_cell(s, "side"));
    ASSERT_EQ(-1, prts_scene_find_cell(s, "attic"));
    ASSERT_STREQ("hall", prts_scene_cell_name(s, hall));
    ASSERT_STREQ("", prts_scene_cell_name(s, 99));
    ASSERT_STREQ("", prts_scene_cell_name(s, -1));
    /* sound: with the door shut the side room can't be reached from outside */
    ASSERT_EQ(PRTS_CELL_BIT(0) | PRTS_CELL_BIT(hall), prts_scene_reachable_cells(s, 0));
    prts_scene_set_portal(s, "door", true);
    ASSERT_TRUE(prts_scene_reachable_cells(s, side) & PRTS_CELL_BIT(0));
    /* nested volumes: the smallest containing cell wins */
    int closet = prts_scene_add_cell(s, "closet", box(-10, 0, -10, -8, 4, -8));
    ASSERT_EQ(closet, prts_scene_cell_at(s, prts_v3(-9, 1, -9)));
    for (int i = (int)s->cells.len; i < PRTS_MAX_CELLS; i++) prts_scene_add_cell(s, "filler", box(0, 0, 0, 1, 1, 1));
    ASSERT_EQ(-1, prts_scene_add_cell(s, "one too many", box(0, 0, 0, 1, 1, 1)));
    prts_scene_clear(s);
    ASSERT_EQ((size_t)1, s->cells.len); /* outside always exists */
    prts_scene_destroy(s);
}

UTEST(cells, geometry_is_grouped_by_cells)
{
    prts_scene *s = prts_scene_create();
    int hall = prts_scene_add_cell(s, "hall", box(-10, 0, -10, 10, 4, 0));
    prts_material m;
    memset(&m, 0, sizeof(m));
    strcpy(m.id, "stone");
    m.tint = prts_v3(1, 1, 1);
    int mat = prts_scene_material(s, &m);
    prts_scene_add_box(s, box(0, 0, 0, 1, 1, 1), mat, PRTS_ENTITY_NONE, 2.0f);
    s->current_cells = PRTS_CELL_BIT(hall);
    prts_scene_add_box(s, box(2, 0, 0, 3, 1, 1), mat, PRTS_ENTITY_NONE, 2.0f);
    prts_scene_add_box(s, box(4, 0, 0, 5, 1, 1), mat, PRTS_ENTITY_NONE, 2.0f);
    ASSERT_EQ((size_t)2, s->groups.len);
    ASSERT_EQ(PRTS_CELLS_ALL, ((prts_mesh_group *)prts_array_at(&s->groups, 0))->cells);
    ASSERT_EQ(PRTS_CELL_BIT(hall), ((prts_mesh_group *)prts_array_at(&s->groups, 1))->cells);
    prts_scene_destroy(s);
}
