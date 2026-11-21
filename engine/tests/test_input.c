/* Unit tests: input actions and the commander controller. MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_commander.h"
#include "prts/prts_input.h"
#include "utest.h"

static int fake_keys(const char *name)
{
    if (strcmp(name, "W") == 0) return 26;
    if (strcmp(name, "E") == 0) return 8;
    if (strcmp(name, "F") == 0) return 9;
    if (strcmp(name, "Huge") == 0) return 9999;
    return 0;
}

UTEST(input, actions_bindings_and_edges)
{
    prts_input *in = prts_input_create(fake_keys);
    ASSERT_EQ(0, prts_input_define(in, "use"));
    ASSERT_EQ(0, prts_input_define(in, "use"));
    ASSERT_EQ(1, prts_input_define(in, "look_x"));
    ASSERT_EQ((size_t)2, prts_input_action_count(in));
    ASSERT_STREQ("look_x", prts_input_action_name(in, 1));
    ASSERT_TRUE(prts_input_action_name(in, 7) == NULL);
    ASSERT_TRUE(prts_input_action_name(in, -1) == NULL);
    ASSERT_TRUE(prts_input_bind(in, "use", PRTS_BIND_KEY, "E", 0, 1.0f));
    ASSERT_FALSE(prts_input_bind(in, "use", PRTS_BIND_KEY, "Nope", 0, 1.0f));
    ASSERT_FALSE(prts_input_bind(in, "use", PRTS_BIND_KEY, "Huge", 0, 1.0f));
    ASSERT_FALSE(prts_input_bind(in, "use", PRTS_BIND_KEY, NULL, 0, 1.0f));
    ASSERT_FALSE(prts_input_bind(in, "nothing", PRTS_BIND_KEY, "E", 0, 1.0f));
    ASSERT_FALSE(prts_input_bind(in, "use", PRTS_BIND_MOUSE_BUTTON, NULL, 99, 1.0f));
    ASSERT_TRUE(prts_input_bind(in, "use", PRTS_BIND_MOUSE_BUTTON, NULL, 1, 0.0f));
    ASSERT_TRUE(prts_input_bind(in, "use", PRTS_BIND_PAD_BUTTON, NULL, 0, 1.0f));
    ASSERT_TRUE(prts_input_bind(in, "look_x", PRTS_BIND_MOUSE_AXIS, NULL, 0, 2.0f));
    ASSERT_TRUE(prts_input_bind(in, "look_x", PRTS_BIND_PAD_AXIS, NULL, 2, -1.0f));
    ASSERT_FALSE(prts_input_bind(in, "look_x", PRTS_BIND_PAD_AXIS, NULL, 6, 1.0f));

    prts_input_state s;
    memset(&s, 0, sizeof(s));
    s.keys[8] = true;
    s.mouse_dx = 3.0f;
    prts_input_update(in, &s);
    ASSERT_TRUE(prts_input_down(in, "use"));
    ASSERT_TRUE(prts_input_pressed(in, "use"));
    ASSERT_EQ(6.0f, prts_input_value(in, "look_x"));
    /* the press survives frames until a tick consumes it */
    prts_input_update(in, &s);
    ASSERT_TRUE(prts_input_pressed(in, "use"));
    prts_input_end_tick(in);
    ASSERT_FALSE(prts_input_pressed(in, "use"));
    /* gamepad (with dead zone) and mouse buttons */
    memset(&s, 0, sizeof(s));
    s.pad_connected = true;
    s.pad_axes[2] = 0.1f;
    s.pad_buttons[0] = true;
    s.mouse[1] = false;
    s.last_device = PRTS_SCHEME_GAMEPAD;
    prts_input_update(in, &s);
    ASSERT_EQ(0.0f, prts_input_value(in, "look_x"));
    ASSERT_TRUE(prts_input_down(in, "use"));
    s.pad_axes[2] = 0.8f;
    s.wheel = 1.0f;
    prts_input_update(in, &s);
    ASSERT_NEAR(-0.8f, prts_input_value(in, "look_x"), 1e-6f);
    ASSERT_EQ(0.0f, prts_input_value(in, "missing"));
    ASSERT_FALSE(prts_input_down(in, "missing"));
    ASSERT_FALSE(prts_input_pressed(in, "missing"));
    prts_input_clear_bindings(in, "look_x");
    prts_input_clear_bindings(in, "missing");
    prts_input_update(in, &s);
    ASSERT_EQ(0.0f, prts_input_value(in, "look_x"));
    s.pad_connected = false;
    ASSERT_TRUE(prts_input_bind(in, "look_x", PRTS_BIND_MOUSE_AXIS, NULL, 1, 1.0f));
    ASSERT_TRUE(prts_input_bind(in, "look_x", PRTS_BIND_MOUSE_AXIS, NULL, 2, 1.0f));
    s.mouse_dy = 2.0f;
    prts_input_update(in, &s);
    ASSERT_EQ(3.0f, prts_input_value(in, "look_x"));
    prts_input_destroy(in);
    prts_input_destroy(NULL);
}

UTEST(input, schemes_and_overrides)
{
    /* Spec: Prompt update (scheme change is reported once) */
    prts_input *in = prts_input_create(fake_keys);
    prts_input_define(in, "move_forward");
    prts_input_state s;
    memset(&s, 0, sizeof(s));
    ASSERT_EQ(PRTS_SCHEME_KBM, prts_input_scheme_active(in));
    s.last_device = PRTS_SCHEME_GAMEPAD;
    prts_input_update(in, &s);
    ASSERT_EQ(PRTS_SCHEME_GAMEPAD, prts_input_scheme_active(in));
    ASSERT_TRUE(prts_input_take_scheme_changed(in));
    ASSERT_FALSE(prts_input_take_scheme_changed(in));
    prts_input_set_scheme(in, PRTS_SCHEME_XR);
    ASSERT_TRUE(prts_input_take_scheme_changed(in));
    prts_input_set_scheme(in, PRTS_SCHEME_XR);
    ASSERT_FALSE(prts_input_take_scheme_changed(in));
    ASSERT_STREQ("kbm", prts_input_scheme_name(PRTS_SCHEME_KBM));
    ASSERT_STREQ("gamepad", prts_input_scheme_name(PRTS_SCHEME_GAMEPAD));
    ASSERT_STREQ("touch", prts_input_scheme_name(PRTS_SCHEME_TOUCH));
    ASSERT_STREQ("xr", prts_input_scheme_name(PRTS_SCHEME_XR));
    /* scripted input holds a value for N ticks */
    prts_input_override(in, "move_forward", 1.0f, 2);
    prts_input_override(in, "missing", 1.0f, 2);
    prts_input_update(in, &s);
    ASSERT_EQ(1.0f, prts_input_value(in, "move_forward"));
    prts_input_tick_overrides(in);
    prts_input_tick_overrides(in);
    prts_input_update(in, &s);
    ASSERT_EQ(0.0f, prts_input_value(in, "move_forward"));
    prts_input_destroy(in);
}

static prts_input *commander_input(void)
{
    prts_input *in = prts_input_create(fake_keys);
    const char *actions[] = {"move_forward", "move_back", "move_left", "move_right", "run", "sprint", "crouch", "jump"};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(actions); i++) prts_input_define(in, actions[i]);
    return in;
}

static void run_ticks(prts_commander *c, prts_world *w, prts_cworld *cw, prts_input *in, int n)
{
    prts_input_state s;
    memset(&s, 0, sizeof(s));
    for (int i = 0; i < n; i++) {
        prts_input_update(in, &s);
        prts_world_begin_tick(w);
        prts_world_tick_movers(w, 1.0f / 30.0f, NULL, NULL);
        prts_cworld_sync(cw, w);
        prts_commander_tick(c, w, cw, in, 1.0f / 30.0f);
        prts_input_end_tick(in);
        prts_input_tick_overrides(in);
    }
}

UTEST(commander, walking_speed_and_crouch)
{
    /* Spec: Walking speed; Crouching lowers the view */
    prts_world *w = prts_world_create();
    prts_cworld *cw = prts_cworld_create();
    prts_cworld_add_box(cw, prts_aabb_make(prts_v3(-100, -1, -100), prts_v3(100, 0, 100)), PRTS_ENTITY_NONE);
    prts_input *in = commander_input();
    prts_commander_params p;
    prts_commander_params_default(&p);
    prts_commander c;
    prts_commander_init(&c, prts_world_spawn(w, "commander"), &p);
    run_ticks(&c, w, cw, in, 5);
    ASSERT_TRUE(c.on_ground);
    prts_input_override(in, "move_forward", 1.0f, 300);
    run_ticks(&c, w, cw, in, 300);
    ASSERT_NEAR(-14.0, prts_world_pos(w, c.entity).z, 0.2);
    prts_input_override(in, "crouch", 1.0f, 30);
    run_ticks(&c, w, cw, in, 30);
    ASSERT_TRUE(c.crouched);
    ASSERT_NEAR(1.10f, c.eye, 1e-4f);
    run_ticks(&c, w, cw, in, 30);
    ASSERT_FALSE(c.crouched);
    ASSERT_NEAR(1.70f, c.eye, 1e-4f);
    /* run, sprint with stamina, strafing */
    prts_dvec3 start = prts_world_pos(w, c.entity);
    prts_input_override(in, "run", 1.0f, 30);
    prts_input_override(in, "move_right", 1.0f, 30);
    run_ticks(&c, w, cw, in, 30);
    ASSERT_NEAR(3.5, prts_world_pos(w, c.entity).x - start.x, 0.1);
    prts_input_override(in, "sprint", 1.0f, 30);
    prts_input_override(in, "move_back", 1.0f, 30);
    prts_input_override(in, "move_left", 1.0f, 30);
    run_ticks(&c, w, cw, in, 30);
    ASSERT_TRUE(c.stamina < p.stamina_max);
    prts_dvec3 eye = prts_commander_eye(&c, w, 1.0);
    ASSERT_NEAR(1.70, eye.y, 1e-4);
    prts_input_destroy(in);
    prts_cworld_destroy(cw);
    prts_world_destroy(w);
}

UTEST(commander, jump_ceiling_and_look)
{
    prts_world *w = prts_world_create();
    prts_cworld *cw = prts_cworld_create();
    prts_cworld_add_box(cw, prts_aabb_make(prts_v3(-10, -1, -10), prts_v3(10, 0, 10)), PRTS_ENTITY_NONE);
    prts_cworld_add_box(cw, prts_aabb_make(prts_v3(-10, 2.2f, -10), prts_v3(10, 3, 10)), PRTS_ENTITY_NONE);
    prts_input *in = commander_input();
    prts_commander_params p;
    prts_commander_params_default(&p);
    prts_commander c;
    prts_commander_init(&c, prts_world_spawn(w, "commander"), &p);
    run_ticks(&c, w, cw, in, 3);
    prts_input_override(in, "jump", 1.0f, 1);
    run_ticks(&c, w, cw, in, 1);
    ASSERT_FALSE(c.on_ground);
    run_ticks(&c, w, cw, in, 10);
    ASSERT_TRUE(prts_world_pos(w, c.entity).y < 0.4);
    run_ticks(&c, w, cw, in, 30);
    ASSERT_TRUE(c.on_ground);
    /* crouching under a low ceiling keeps you crouched */
    prts_cworld_add_box(cw, prts_aabb_make(prts_v3(-10, 1.5f, -10), prts_v3(10, 2.2f, 10)), PRTS_ENTITY_NONE);
    c.crouched = true;
    run_ticks(&c, w, cw, in, 2);
    ASSERT_TRUE(c.crouched);
    prts_commander_look(&c, 0.5f, 3.0f);
    ASSERT_NEAR(1.48f, c.pitch, 1e-6f);
    ASSERT_NEAR(0.5f, c.yaw, 1e-6f);
    ASSERT_EQ(PRTS_ENTITY_NONE, prts_commander_target(&c, w, cw));
    prts_input_destroy(in);
    prts_cworld_destroy(cw);
    prts_world_destroy(w);
}

UTEST(commander, rides_a_moving_platform_and_uses_things)
{
    /* Spec: Riding the keep's hoist lift; Walking during movement */
    prts_world *w = prts_world_create();
    prts_cworld *cw = prts_cworld_create();
    prts_cworld_add_box(cw, prts_aabb_make(prts_v3(-10, -1, -10), prts_v3(10, 0, 10)), PRTS_ENTITY_NONE);
    prts_entity lift = prts_world_spawn(w, "lift");
    prts_world_set_local(w, lift, prts_dv3(0, 0, 0), 0.0f);
    prts_cworld_add_box(cw, prts_aabb_make(prts_v3(-1.5f, 0, -1.5f), prts_v3(1.5f, 0.2f, 1.5f)), lift);
    prts_entity lever = prts_world_spawn(w, "lever");
    prts_world_set_local(w, lever, prts_dv3(0, 1.9, -1.4), 0.0f);
    prts_world_set_parent(w, lever, lift);
    prts_world_set_usable(w, lever, true);
    prts_cworld_add_box(cw, prts_aabb_make(prts_v3(-0.1f, -0.2f, -0.1f), prts_v3(0.1f, 0.2f, 0.1f)), lever);
    prts_cworld_sync(cw, w);
    prts_input *in = commander_input();
    prts_commander_params p;
    prts_commander_params_default(&p);
    prts_commander c;
    prts_commander_init(&c, prts_world_spawn(w, "commander"), &p);
    prts_world_set_local(w, c.entity, prts_dv3(0, 0.5, 0), 0.0f);
    run_ticks(&c, w, cw, in, 10);
    ASSERT_EQ(lift, prts_world_parent(w, c.entity));
    /* looking at the lever finds it */
    c.pitch = 0.0f;
    ASSERT_EQ(lever, prts_commander_target(&c, w, cw));
    prts_world_move_to(w, lift, prts_dv3(0, 8, 0), 2.0f);
    prts_input_override(in, "move_right", 1.0f, 20);
    run_ticks(&c, w, cw, in, 60);
    ASSERT_EQ(lift, prts_world_parent(w, c.entity));
    ASSERT_NEAR(4.2, prts_world_pos(w, c.entity).y, 0.05);
    ASSERT_NEAR(20.0 / 30.0 * 1.4, prts_world_local_pos(w, c.entity).x, 0.05);
    run_ticks(&c, w, cw, in, 90);
    ASSERT_NEAR(8.2, prts_world_pos(w, c.entity).y, 0.02);
    prts_input_destroy(in);
    prts_cworld_destroy(cw);
    prts_world_destroy(w);
}
