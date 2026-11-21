/* Unit tests: playing in XR (tasks 13.2-13.4): controller input into actions, pointer rays,
 * the body and the head against walls, teleports, comfort options. MIT License. */
#include <lauxlib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_commander.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"
#include "prts/prts_xr.h"
#include "utest.h"

#define PI_F 3.14159265f

static void set_pose(prts_xr_pose *p, float x, float y, float z, float yaw, float pitch)
{
    p->position[0] = x;
    p->position[1] = y;
    p->position[2] = z;
    /* yaw about +y, then pitch about +x */
    float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f), cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
    p->orientation[0] = cy * sp;
    p->orientation[1] = sy * cp;
    p->orientation[2] = -sy * sp;
    p->orientation[3] = cy * cp;
}

UTEST(xr, pose_math_and_hands)
{
    prts_xr_pose p;
    memset(&p, 0, sizeof(p));
    prts_vec3 f = prts_xr_forward(&p); /* a zero quaternion counts as no rotation */
    ASSERT_NEAR(-1.0f, f.z, 1e-6f);
    set_pose(&p, 0, 0, 0, 0.5f, 0.25f);
    f = prts_xr_forward(&p);
    prts_vec3 want = prts_view_forward(0.5f, 0.25f);
    ASSERT_NEAR(want.x, f.x, 1e-5f);
    ASSERT_NEAR(want.y, f.y, 1e-5f);
    ASSERT_NEAR(want.z, f.z, 1e-5f);
    float yaw = 0.0f, pitch = 0.0f;
    prts_xr_yaw_pitch(&p, &yaw, &pitch);
    ASSERT_NEAR(0.5f, yaw, 1e-5f);
    ASSERT_NEAR(0.25f, pitch, 1e-5f);
    /* looking straight down or up: the yaw comes from the top (or bottom) of the view */
    set_pose(&p, 0, 0, 0, -1.0f, -PI_F / 2);
    prts_xr_yaw_pitch(&p, &yaw, &pitch);
    ASSERT_NEAR(-1.0f, yaw, 1e-3f);
    ASSERT_NEAR(-PI_F / 2, pitch, 1e-3f);
    set_pose(&p, 0, 0, 0, 2.0f, PI_F / 2);
    prts_xr_yaw_pitch(&p, &yaw, &pitch);
    ASSERT_NEAR(2.0f, yaw, 1e-3f);

    prts_xr xr;
    prts_xr_reset(&xr);
    ASSERT_TRUE(prts_xr_hand_controller(&xr, PRTS_XR_HAND_RIGHT) == NULL);
    xr.controllers[0].connected = true; /* no handedness: the right hand */
    ASSERT_TRUE(prts_xr_hand_controller(&xr, PRTS_XR_HAND_RIGHT) == &xr.controllers[0]);
    ASSERT_TRUE(prts_xr_hand_controller(&xr, PRTS_XR_HAND_LEFT) == NULL);
    ASSERT_TRUE(prts_xr_hand_controller(&xr, PRTS_XR_HAND_NONE) == &xr.controllers[0]);
    xr.controllers[1].connected = true; /* a right-handed one too: the other is the left */
    xr.controllers[1].hand = PRTS_XR_HAND_RIGHT;
    ASSERT_TRUE(prts_xr_hand_controller(&xr, PRTS_XR_HAND_RIGHT) == &xr.controllers[1]);
    ASSERT_TRUE(prts_xr_hand_controller(&xr, PRTS_XR_HAND_LEFT) == &xr.controllers[0]);
    xr.controllers[0].hand = PRTS_XR_HAND_LEFT;
    ASSERT_TRUE(prts_xr_hand_controller(&xr, PRTS_XR_HAND_LEFT) == &xr.controllers[0]);
    prts_xr_request_exit(&xr); /* nothing to end */
    prts_xr_begin(&xr, 0, 10, 10);
    prts_xr_request_exit(&xr); /* a simulated session ends at once */
    ASSERT_FALSE(xr.active);
}

UTEST(xr, controllers_drive_actions_by_hand)
{
    /* Spec "XR controllers": controller input maps to the same actions as other schemes */
    prts_input *in = prts_input_create(NULL);
    prts_input_define(in, "use");
    prts_input_define(in, "move_forward");
    prts_input_define(in, "look_x");
    ASSERT_TRUE(prts_input_bind(in, "use", PRTS_BIND_XR_BUTTON, NULL, 8, 1.0f)); /* right trigger */
    ASSERT_TRUE(prts_input_bind(in, "use", PRTS_BIND_XR_BUTTON, NULL, 0, 1.0f)); /* left trigger */
    ASSERT_FALSE(prts_input_bind(in, "use", PRTS_BIND_XR_BUTTON, NULL, 16, 1.0f));
    ASSERT_TRUE(prts_input_bind(in, "move_forward", PRTS_BIND_XR_AXIS, NULL, 3, -1.0f)); /* left stick y */
    ASSERT_TRUE(prts_input_bind(in, "look_x", PRTS_BIND_XR_AXIS, NULL, 6, 1.0f));        /* right stick x */
    ASSERT_FALSE(prts_input_bind(in, "look_x", PRTS_BIND_XR_AXIS, NULL, 8, 1.0f));
    ASSERT_EQ(PRTS_SCHEME_XR, prts_binding_scheme(PRTS_BIND_XR_BUTTON));
    ASSERT_EQ(PRTS_SCHEME_XR, prts_binding_scheme(PRTS_BIND_XR_AXIS));
    prts_input_clear_scheme(in, "look_x", PRTS_SCHEME_GAMEPAD);
    ASSERT_EQ((size_t)1, prts_input_binding_count(in, "look_x"));

    prts_xr xr;
    prts_xr_reset(&xr);
    prts_input_state s;
    memset(&s, 0, sizeof(s));
    s.xr_connected[0] = true;
    s.xr_buttons[0][0] = 1.0f;
    prts_xr_feed_input(&xr, &s); /* no session: nothing is connected */
    ASSERT_FALSE(s.xr_connected[0]);
    ASSERT_EQ(0.0f, s.xr_buttons[0][0]);

    prts_xr_begin(&xr, 0, 100, 100);
    prts_xr_controller *right = &xr.controllers[0], *left = &xr.controllers[1];
    right->connected = left->connected = true;
    right->hand = PRTS_XR_HAND_RIGHT;
    left->hand = PRTS_XR_HAND_LEFT;
    right->button_count = 2;
    right->buttons[0] = 0.8f; /* a half-pulled trigger the browser does not count as pressed */
    right->buttons[1] = NAN;
    right->axis_count = 4;
    right->axes[2] = 0.9f;
    right->axes[3] = NAN;
    left->axis_count = 4;
    left->axes[3] = -3.0f;
    prts_xr_feed_input(&xr, &s);
    ASSERT_TRUE(s.xr_connected[0] && s.xr_connected[1]);
    ASSERT_NEAR(0.4f, s.xr_buttons[1][0], 1e-6f);
    ASSERT_EQ(0.0f, s.xr_buttons[1][1]);
    ASSERT_EQ(0.0f, s.xr_axes[1][3]);
    ASSERT_EQ(-1.0f, s.xr_axes[0][3]);
    prts_input_update(in, &s);
    ASSERT_FALSE(prts_input_down(in, "use"));
    ASSERT_NEAR(1.0f, prts_input_value(in, "move_forward"), 1e-6f);
    ASSERT_NEAR(0.9f, prts_input_value(in, "look_x"), 1e-6f);
    right->pressed[0] = true;
    right->axes[2] = 0.1f; /* inside the dead zone */
    prts_xr_feed_input(&xr, &s);
    s.last_device = PRTS_SCHEME_XR;
    prts_input_update(in, &s);
    ASSERT_TRUE(prts_input_pressed(in, "use"));
    ASSERT_EQ(0.0f, prts_input_value(in, "look_x"));
    ASSERT_EQ(PRTS_SCHEME_XR, prts_input_scheme_active(in));
    s.xr_connected[1] = false; /* a controller that went away reads as released */
    prts_input_update(in, &s);
    ASSERT_FALSE(prts_input_down(in, "use"));
    s.xr_connected[0] = false;
    prts_input_update(in, &s);
    ASSERT_EQ(0.0f, prts_input_value(in, "move_forward"));
    prts_input_destroy(in);
}

/* --- the engine with the base game ------------------------------------------------------- */

static prts_engine *boot(char *err)
{
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = PRTS_TEST_DATA;
    cfg.user_dir = PRTS_TEST_TMP "/user-xr";
    cfg.headless = true;
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine *e = prts_engine_create(&cfg, err, 1024);
    prts_log_set_min_level(PRTS_LOG_INFO);
    return e;
}

static void place(prts_engine *e, double x, double y, double z, float yaw)
{
    prts_commander *c = &e->commander;
    prts_world_set_parent(e->world, c->entity, PRTS_ENTITY_NONE);
    prts_world_set_local(e->world, c->entity, prts_dv3(x, y, z), 0.0f);
    prts_world_begin_tick(e->world);
    c->yaw = yaw;
    c->pitch = 0.0f;
}

static lua_Integer lua_int(prts_engine *e, const char *code)
{
    lua_State *L = prts_script_lua(e->script);
    luaL_dostring(L, code);
    lua_Integer v = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return v;
}

/* Reads a pose fixture: one packed frame (PRTS_XR_FRAME_FLOATS floats) per line, # comments. */
static int load_frames(const char *path, float *frames, int max)
{
    char *text = NULL;
    size_t size = 0;
    if (!prts_fs_read_file(path, &text, &size)) return 0;
    int n = 0;
    for (char *line = text; line && *line && n < max;) {
        char *next = strchr(line, '\n');
        if (next) *next++ = '\0';
        if (*line && *line != '#') {
            char *p = line;
            for (int i = 0; i < PRTS_XR_FRAME_FLOATS; i++) frames[n * PRTS_XR_FRAME_FLOATS + i] = strtof(p, &p);
            n++;
        }
        line = next;
    }
    prts_free(text);
    return n;
}

UTEST(xr, recorded_frames_pull_the_lift_lever)
{
    /* Spec "Pulling the lift lever": a recorded pointing gesture (engine/tests/fixtures/xr) aims
     * the right controller's ray at the hoist lift's call lever and pulls the trigger. */
    static float frames[64 * PRTS_XR_FRAME_FLOATS];
    int n = load_frames(PRTS_TEST_FIXTURES "/xr/lever_pull.txt", frames, 64);
    ASSERT_EQ(45, n);
    char err[1024];
    prts_engine *e = boot(err);
    ASSERT_TRUE(e != NULL);
    place(e, -6.4, 9.2, 1.65, PI_F / 2); /* the war room floor at the lift gate, facing west */
    for (int i = 0; i < 10; i++) prts_engine_frame(e, 1.0 / 30.0);
    prts_xr *xr = prts_xr_get();
    prts_xr_begin(xr, 0, 2000, 1000);
    prts_entity call = prts_world_find(e->world, "keep.lift.call2");
    bool aimed = false, pulled = false;
    for (int i = 0; i < n; i++) {
        ASSERT_TRUE(prts_xr_apply_frame(xr, frames + i * PRTS_XR_FRAME_FLOATS, PRTS_XR_FRAME_FLOATS));
        prts_engine_frame(e, 1.0 / 90.0);
        if (i == 25) aimed = prts_commander_target(&e->commander, e->world, e->cworld) == call;
        if (i == 0) {
            ASSERT_EQ(PRTS_SCHEME_XR, prts_input_scheme_active(e->input));
            ASSERT_NEAR(PI_F / 2, e->xr.turn, 1e-4f); /* the play area keeps the flat view's heading */
            ASSERT_EQ(PRTS_ENTITY_NONE, prts_commander_target(&e->commander, e->world, e->cworld));
        }
        if (i == 44) pulled = lua_int(e, "return prts.entity.data(prts.entity.find('keep.lift.platform')).target") == 3;
    }
    ASSERT_TRUE(aimed);
    ASSERT_TRUE(pulled);
    ASSERT_EQ(1, e->xr.hand); /* the right hand points */
    /* the left trigger makes the left hand the pointer */
    float *last = frames + (n - 1) * PRTS_XR_FRAME_FLOATS;
    float *lc = last + PRTS_XR_F_CONTROLLERS + PRTS_XR_F_CONTROLLER_SIZE;
    lc[19] = 1.0f;
    lc[20] = 1.0f;
    prts_xr_apply_frame(xr, last, PRTS_XR_FRAME_FLOATS);
    prts_engine_frame(e, 1.0 / 90.0);
    ASSERT_EQ(0, e->xr.hand);
    /* the headset ends the session: back to the flat controls */
    prts_xr_end(xr);
    prts_engine_frame(e, 1.0 / 90.0);
    ASSERT_EQ(PRTS_SCHEME_KBM, prts_input_scheme_active(e->input));
    ASSERT_FALSE(e->commander.xr.active);
    prts_engine_destroy(e);
    prts_xr_reset(xr);
}

/* --- the commander's body ---------------------------------------------------------------- */

typedef struct body {
    prts_world *w;
    prts_cworld *cw;
    prts_input *in;
    prts_commander c;
} body;

static void body_create(body *b)
{
    b->w = prts_world_create();
    b->cw = prts_cworld_create();
    prts_cworld_add_box(b->cw, prts_aabb_make(prts_v3(-20, -1, -20), prts_v3(20, 0, 20)), PRTS_ENTITY_NONE);
    b->in = prts_input_create(NULL);
    const char *actions[] = {"move_forward", "move_back", "move_left", "move_right", "teleport", "use"};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(actions); i++) prts_input_define(b->in, actions[i]);
    prts_commander_params p;
    prts_commander_params_default(&p);
    prts_commander_init(&b->c, prts_world_spawn(b->w, "commander"), &p);
}

static void body_ticks(body *b, int n)
{
    prts_input_state s;
    memset(&s, 0, sizeof(s));
    for (int i = 0; i < n; i++) {
        prts_input_update(b->in, &s);
        prts_world_begin_tick(b->w);
        prts_cworld_sync(b->cw, b->w);
        prts_commander_tick(&b->c, b->w, b->cw, b->in, 1.0f / 30.0f);
        /* what the engine does with the play area: the head stays where it is in the world */
        b->c.xr.head = prts_v3_sub(b->c.xr.head, b->c.followed);
        prts_input_end_tick(b->in);
        prts_input_tick_overrides(b->in);
    }
}

static void body_destroy(body *b)
{
    prts_input_destroy(b->in);
    prts_cworld_destroy(b->cw);
    prts_world_destroy(b->w);
}

UTEST(commander, xr_body_follows_the_head_and_walls_push_the_view_back)
{
    /* Spec "Leaning toward a wall" and "Head-tracked first-person view" */
    body b;
    body_create(&b);
    prts_cworld_add_box(b.cw, prts_aabb_make(prts_v3(-5, 0, -2.0f), prts_v3(5, 3, -1.5f)), PRTS_ENTITY_NONE); /* wall north */
    body_ticks(&b, 5);
    b.c.xr.active = true;
    b.c.xr.head = prts_v3(0.4f, 1.65f, 0.0f); /* a step to the east in the play area */
    body_ticks(&b, 1);
    ASSERT_NEAR(0.4f, b.c.followed.x, 1e-4f);
    ASSERT_NEAR(0.4, prts_world_pos(b.w, b.c.entity).x, 1e-4);
    ASSERT_NEAR(0.0f, b.c.xr.head.x, 1e-4f);
    b.c.xr.head = prts_v3(0.0f, 1.65f, 3.0f); /* a recentre: at most 1 m per tick */
    body_ticks(&b, 1);
    ASSERT_NEAR(1.0f, b.c.followed.z, 1e-4f);
    body_ticks(&b, 3);
    ASSERT_NEAR(3.0, prts_world_pos(b.w, b.c.entity).z, 1e-3);
    /* leaning north into the wall: the body stops at it, the head goes on */
    prts_dvec3 feet = prts_world_pos(b.w, b.c.entity);
    b.c.xr.head = prts_v3(0.0f, 1.65f, -(float)feet.z - 1.6f); /* 0.1 m into the wall (south face z = -1.5) */
    body_ticks(&b, 6);
    feet = prts_world_pos(b.w, b.c.entity);
    ASSERT_NEAR(-1.2, feet.z, 1e-3); /* the body's half width from the wall */
    prts_vec3 push = prts_commander_head_pushback(&b.c, b.cw, feet, b.c.xr.head);
    float view_z = (float)feet.z + b.c.xr.head.z + push.z;
    ASSERT_NEAR(-1.5f + 0.12f, view_z, 1e-3f); /* the view stops 12 cm short of the surface */
    ASSERT_NEAR(1.65f, b.c.xr.head.y + push.y, 1e-4f);
    /* a free head is not moved */
    push = prts_commander_head_pushback(&b.c, b.cw, feet, prts_v3(0.0f, 1.7f, 0.1f));
    ASSERT_NEAR(0.0f, prts_v3_length(push), 1e-5f);
    /* outside XR the body does not follow */
    b.c.xr.active = false;
    b.c.xr.head = prts_v3(2.0f, 1.65f, 2.0f);
    body_ticks(&b, 2);
    ASSERT_NEAR(0.0f, prts_v3_length(b.c.followed), 1e-6f);
    body_destroy(&b);
}

UTEST(commander, xr_pointer_ray_uses_things_but_not_through_walls)
{
    body b;
    body_create(&b);
    prts_entity lever = prts_world_spawn(b.w, "lever");
    prts_world_set_local(b.w, lever, prts_dv3(1.0, 1.2, -1.0), 0.0f);
    prts_world_set_usable(b.w, lever, true);
    prts_cworld_add_box(b.cw, prts_aabb_make(prts_v3(-0.1f, -0.2f, -0.1f), prts_v3(0.1f, 0.2f, 0.1f)), lever);
    prts_cworld_add_box(b.cw, prts_aabb_make(prts_v3(-2, 0, 0.4f), prts_v3(2, 3, 0.8f)), PRTS_ENTITY_NONE); /* wall south */
    body_ticks(&b, 5);
    ASSERT_EQ(PRTS_ENTITY_NONE, prts_commander_target(&b.c, b.w, b.cw)); /* the view looks north, past it */
    b.c.xr.active = b.c.xr.has_pointer = true;
    b.c.xr.pointer_origin = prts_v3(0.3f, 1.2f, -0.3f);
    b.c.xr.pointer_dir = prts_v3(1.0f, 0.0f, -1.0f);
    ASSERT_EQ(lever, prts_commander_target(&b.c, b.w, b.cw));
    b.c.xr.pointer_origin = prts_v3(0.3f, 1.2f, 0.6f); /* the hand reaches into the wall */
    ASSERT_EQ(PRTS_ENTITY_NONE, prts_commander_target(&b.c, b.w, b.cw));
    body_destroy(&b);
}

UTEST(commander, xr_teleport_to_reachable_floor_only)
{
    /* Spec "Teleport movement" */
    body b;
    body_create(&b);
    prts_aabb (*mk)(prts_vec3, prts_vec3) = prts_aabb_make;
    prts_cworld_add_box(b.cw, mk(prts_v3(-0.8f, 0, -2), prts_v3(-0.7f, 1.5f, 2)), PRTS_ENTITY_NONE); /* a low wall west */
    prts_cworld_add_box(b.cw, mk(prts_v3(3, 0, -5), prts_v3(5, 0.3f, -2)), PRTS_ENTITY_NONE);        /* a step north-east */
    prts_cworld_add_box(b.cw, mk(prts_v3(3, 0, 2), prts_v3(5, 1.2f, 5)), PRTS_ENTITY_NONE);          /* a table south-east */
    prts_cworld_add_box(b.cw, mk(prts_v3(3, 2.0f, 2), prts_v3(5, 2.2f, 5)), PRTS_ENTITY_NONE);       /* a shelf over it */
    body_ticks(&b, 5);
    prts_commander *c = &b.c;
    prts_dvec3 t;
    ASSERT_FALSE(prts_commander_teleport_target(c, b.w, b.cw, &t)); /* not in XR */
    c->xr.active = c->xr.teleport = c->xr.has_pointer = true;
    c->xr.head = prts_v3(0, 1.65f, 0);
    c->xr.pointer_origin = prts_v3(0.0f, 1.2f, 0.0f);
    c->xr.pointer_dir = prts_v3(0.0f, -1.2f, -3.0f); /* the floor 3 m ahead */
    ASSERT_TRUE(prts_commander_teleport_target(c, b.w, b.cw, &t));
    ASSERT_NEAR(-3.0, t.z, 0.06);
    ASSERT_NEAR(0.0, t.y, 1e-4);
    c->xr.pointer_dir = prts_v3(0.0f, 0.2f, -1.0f); /* up: no floor */
    ASSERT_FALSE(prts_commander_teleport_target(c, b.w, b.cw, &t));
    c->xr.pointer_dir = prts_v3(0.0f, -0.01f, -1.0f); /* almost level: nothing within range */
    ASSERT_FALSE(prts_commander_teleport_target(c, b.w, b.cw, &t));
    c->xr.pointer_dir = prts_v3(-1.0f, -0.2f, 0.0f); /* the low wall's face */
    ASSERT_FALSE(prts_commander_teleport_target(c, b.w, b.cw, &t));
    c->xr.pointer_dir = prts_v3(4.0f, -0.9f, -3.5f); /* up onto the step */
    ASSERT_TRUE(prts_commander_teleport_target(c, b.w, b.cw, &t));
    ASSERT_NEAR(0.3, t.y, 1e-4);
    c->xr.pointer_origin = prts_v3(0.0f, 1.6f, 0.0f);
    c->xr.pointer_dir = prts_v3(4.0f, -0.4f, 3.5f); /* onto the table: no room under the shelf */
    ASSERT_FALSE(prts_commander_teleport_target(c, b.w, b.cw, &t));
    c->xr.pointer_origin = prts_v3(-0.2f, 2.0f, 0.0f); /* a raised hand, over the low wall to the floor beyond */
    c->xr.pointer_dir = prts_v3(-2.8f, -2.0f, 0.0f);
    ASSERT_FALSE(prts_commander_teleport_target(c, b.w, b.cw, &t));
    c->xr.pointer_origin = prts_v3(0.0f, 1.2f, 0.0f);
    c->xr.has_pointer = false;
    ASSERT_FALSE(prts_commander_teleport_target(c, b.w, b.cw, &t));

    /* aim (hold "teleport"), then let go: the commander is there at once */
    c->xr.has_pointer = true;
    c->xr.pointer_dir = prts_v3(0.0f, -1.2f, -3.0f);
    prts_input_override(b.in, "teleport", 1.0f, 5);
    prts_input_override(b.in, "move_forward", 1.0f, 5); /* the stick that aims does not walk */
    body_ticks(&b, 5);
    ASSERT_TRUE(c->aiming);
    ASSERT_NEAR(0.0, prts_world_pos(b.w, c->entity).z, 1e-4);
    body_ticks(&b, 1);
    ASSERT_FALSE(c->aiming);
    ASSERT_NEAR(-3.0, prts_world_pos(b.w, c->entity).z, 0.06);
    ASSERT_NEAR(-3.0, prts_world_pos_interp(b.w, c->entity, 0.0).z, 0.06); /* not interpolated */
    /* letting go while aiming at nothing stays */
    c->xr.pointer_dir = prts_v3(0.0f, 0.5f, -1.0f);
    prts_input_override(b.in, "teleport", 1.0f, 2);
    body_ticks(&b, 3);
    ASSERT_NEAR(-3.0, prts_world_pos(b.w, c->entity).z, 0.06);
    /* smooth movement: the stick walks */
    c->xr.teleport = false;
    prts_input_override(b.in, "move_forward", 1.0f, 30);
    body_ticks(&b, 30);
    ASSERT_NEAR(-4.4, prts_world_pos(b.w, c->entity).z, 0.1);
    body_destroy(&b);
}

UTEST(collide, ground_below_a_point)
{
    prts_cworld *cw = prts_cworld_create();
    float h = 0.0f;
    ASSERT_FALSE(prts_cworld_ground(cw, prts_v3(0, 1, 0), 2.0f, &h));
    prts_cworld_add_box(cw, prts_aabb_make(prts_v3(-1, -1, -1), prts_v3(1, 0, 1)), PRTS_ENTITY_NONE);
    prts_cworld_add_box(cw, prts_aabb_make(prts_v3(0, 0, -1), prts_v3(1, 0.5f, 1)), PRTS_ENTITY_NONE);
    ASSERT_TRUE(prts_cworld_ground(cw, prts_v3(-0.5f, 1, 0), 2.0f, &h));
    ASSERT_EQ(0.0f, h);
    ASSERT_TRUE(prts_cworld_ground(cw, prts_v3(0.5f, 1, 0), 2.0f, &h));
    ASSERT_EQ(0.5f, h); /* the highest top */
    ASSERT_TRUE(prts_cworld_ground(cw, prts_v3(0.5f, 0.2f, 0), 2.0f, &h));
    ASSERT_EQ(0.0f, h); /* inside the upper box: the one below */
    ASSERT_FALSE(prts_cworld_ground(cw, prts_v3(-0.5f, 3, 0), 1.0f, &h)); /* too far down */
    prts_heightfield *hf = prts_heightfield_create(3, 3, 10.0f, 5.0f, -10.0f);
    prts_heightfield_set(hf, 1, 1, 1.0f);
    prts_cworld_set_heightfield(cw, hf);
    ASSERT_TRUE(prts_cworld_ground(cw, prts_v3(15.0f, 2.0f, 0.0f), 2.0f, &h));
    ASSERT_NEAR(1.0f, h, 1e-5f);
    prts_heightfield_set(hf, 2, 1, 30.0f); /* a cliff: too steep to stand on */
    ASSERT_FALSE(prts_cworld_ground(cw, prts_v3(20.0f, 40.0f, 0.0f), 60.0f, &h));
    prts_cworld_destroy(cw);
    prts_heightfield_destroy(hf);
}
