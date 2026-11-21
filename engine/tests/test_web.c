/* Unit tests: what the web glue relies on (host signals, XR state, engine status). MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_engine.h"
#include "prts/prts_log.h"
#include "prts/prts_platform.h"
#include "prts/prts_xr.h"
#include "utest.h"

static int gesture_calls;
static void *gesture_user;

static void on_gesture(void *user)
{
    gesture_calls++;
    gesture_user = user;
}

UTEST(host, scheme_hint_is_taken_once)
{
    prts_input_scheme s = PRTS_SCHEME_KBM;
    while (prts_platform_take_scheme_hint(&s)) {
    }
    ASSERT_FALSE(prts_platform_take_scheme_hint(&s));
    prts_platform_hint_scheme(PRTS_SCHEME_TOUCH);
    ASSERT_TRUE(prts_platform_take_scheme_hint(&s));
    ASSERT_EQ(PRTS_SCHEME_TOUCH, s);
    ASSERT_FALSE(prts_platform_take_scheme_hint(&s));
}

UTEST(host, user_gestures_reach_the_handler)
{
    /* Spec "First click starts music": the web glue reports gestures, audio registers a handler. */
    int marker = 0;
    prts_platform_set_gesture_handler(NULL, NULL);
    unsigned before = prts_platform_gesture_count();
    prts_platform_user_gesture(); /* no handler yet: only counted */
    ASSERT_EQ(before + 1, prts_platform_gesture_count());
    gesture_calls = 0;
    /* a handler registered after a gesture (audio created late) is called at once */
    prts_platform_set_gesture_handler(on_gesture, &marker);
    ASSERT_EQ(1, gesture_calls);
    ASSERT_TRUE(gesture_user == &marker);
    prts_platform_user_gesture();
    ASSERT_EQ(2, gesture_calls);
    ASSERT_EQ(before + 2, prts_platform_gesture_count());
    prts_platform_set_gesture_handler(NULL, NULL);
    prts_platform_user_gesture();
    ASSERT_EQ(2, gesture_calls);
}

static void put_pose(float *d, float x, float y, float z)
{
    d[0] = x;
    d[1] = y;
    d[2] = z;
    d[6] = 1.0f; /* identity orientation */
}

/* A recorded frame: head at 1.6 m, two eyes side by side, a left controller with the trigger
 * pressed and a thumbstick, and an empty second slot. */
static void fixture_frame(float *d)
{
    memset(d, 0, sizeof(float) * PRTS_XR_FRAME_FLOATS);
    d[PRTS_XR_F_VIEW_COUNT] = 2;
    d[PRTS_XR_F_HAS_POSE] = 1;
    put_pose(d + PRTS_XR_F_HEAD, 0.0f, 1.6f, 0.0f);
    for (int v = 0; v < 2; v++) {
        float *view = d + PRTS_XR_F_VIEWS + v * PRTS_XR_F_VIEW_SIZE;
        for (int i = 0; i < 16; i++) view[i] = view[16 + i] = (i % 5 == 0) ? 1.0f : 0.0f;
        view[12] = v == 0 ? 0.032f : -0.032f;
        view[32] = (float)(v * 800);
        view[34] = 800;
        view[35] = 800;
    }
    float *c = d + PRTS_XR_F_CONTROLLERS;
    c[0] = 1;
    c[1] = PRTS_XR_HAND_LEFT;
    c[2] = 1;
    put_pose(c + 3, -0.2f, 1.2f, -0.3f);
    c[10] = 1;
    put_pose(c + 11, -0.2f, 1.25f, -0.35f);
    c[18] = 2;
    c[19] = 1; /* button 0 pressed */
    c[20] = 1.0f;
    c[21] = 0.25f;
    c[28] = 4;
    c[32] = 0.5f; /* axes[3]: thumbstick y */
}

UTEST(xr, session_lifecycle)
{
    prts_xr xr;
    prts_xr_reset(&xr);
    prts_xr_detect(&xr); /* no XR outside the browser */
    ASSERT_FALSE(xr.supported);
    prts_xr_set_supported(&xr, true);
    ASSERT_TRUE(xr.supported);
    prts_xr_begin(&xr, 7, 1600, 800);
    ASSERT_TRUE(xr.active);
    ASSERT_EQ(7u, xr.framebuffer);
    ASSERT_EQ(1600, xr.fb_width);
    ASSERT_EQ((uint64_t)1, xr.sessions);
    float d[PRTS_XR_FRAME_FLOATS];
    fixture_frame(d);
    ASSERT_TRUE(prts_xr_apply_frame(&xr, d, PRTS_XR_FRAME_FLOATS));
    /* Spec "Session ended by the headset": state goes back to flat, counters stay */
    prts_xr_end(&xr);
    ASSERT_FALSE(xr.active);
    ASSERT_EQ(0, xr.view_count);
    ASSERT_EQ(0u, xr.framebuffer);
    ASSERT_FALSE(xr.controllers[0].connected);
    ASSERT_EQ((uint64_t)1, xr.frames);
    ASSERT_FALSE(prts_xr_apply_frame(&xr, d, PRTS_XR_FRAME_FLOATS)); /* no session */
    prts_xr_begin(&xr, 9, 10, 10);
    ASSERT_EQ((uint64_t)2, xr.sessions);
    ASSERT_TRUE(prts_xr_get() != NULL);
    ASSERT_TRUE(prts_xr_get() == prts_xr_get());
}

UTEST(xr, packed_frame_is_read)
{
    /* Spec "Entering VR": the scene is drawn once per eye from the tracked head pose. */
    prts_xr xr;
    prts_xr_reset(&xr);
    prts_xr_begin(&xr, 1, 1600, 800);
    float d[PRTS_XR_FRAME_FLOATS];
    fixture_frame(d);
    ASSERT_FALSE(prts_xr_apply_frame(&xr, d, PRTS_XR_FRAME_FLOATS - 1)); /* too short */
    ASSERT_FALSE(prts_xr_apply_frame(&xr, NULL, PRTS_XR_FRAME_FLOATS));
    ASSERT_EQ((uint64_t)0, xr.frames);
    ASSERT_TRUE(prts_xr_apply_frame(&xr, d, PRTS_XR_FRAME_FLOATS));
    ASSERT_EQ((uint64_t)1, xr.frames);
    ASSERT_TRUE(xr.has_pose);
    ASSERT_EQ(1.6f, xr.head.position[1]);
    ASSERT_EQ(1.0f, xr.head.orientation[3]);
    ASSERT_EQ(2, xr.view_count);
    ASSERT_EQ(0.032f, xr.views[0].view[12]);
    ASSERT_EQ(-0.032f, xr.views[1].view[12]);
    ASSERT_EQ(1.0f, xr.views[1].projection[15]);
    ASSERT_EQ(800, xr.views[1].viewport[0]);
    ASSERT_EQ(800, xr.views[1].viewport[2]);
    const prts_xr_controller *c = &xr.controllers[0];
    ASSERT_TRUE(c->connected);
    ASSERT_EQ(PRTS_XR_HAND_LEFT, c->hand);
    ASSERT_TRUE(c->has_grip && c->has_aim);
    ASSERT_EQ(1.2f, c->grip.position[1]);
    ASSERT_EQ(-0.35f, c->aim.position[2]);
    ASSERT_EQ(2, c->button_count);
    ASSERT_TRUE(c->pressed[0]);
    ASSERT_FALSE(c->pressed[1]);
    ASSERT_EQ(0.25f, c->buttons[1]);
    ASSERT_EQ(4, c->axis_count);
    ASSERT_EQ(0.5f, c->axes[3]);
    ASSERT_FALSE(xr.controllers[1].connected);

    /* out-of-range counts are clamped, NaN counts as zero, unknown hands become right */
    float *cd = d + PRTS_XR_F_CONTROLLERS;
    d[PRTS_XR_F_VIEW_COUNT] = 9;
    cd[1] = 7;
    cd[2] = 0;
    cd[10] = 0;
    cd[18] = 99;
    cd[19] = 1e9f;
    cd[28] = NAN;
    ASSERT_TRUE(prts_xr_apply_frame(&xr, d, PRTS_XR_FRAME_FLOATS));
    ASSERT_EQ(PRTS_XR_MAX_VIEWS, xr.view_count);
    ASSERT_EQ(PRTS_XR_HAND_RIGHT, c->hand);
    ASSERT_FALSE(c->has_grip || c->has_aim);
    ASSERT_EQ(PRTS_XR_MAX_BUTTONS, c->button_count);
    ASSERT_EQ(0, c->axis_count);

    /* tracking lost: no pose, no views, controllers still reported */
    d[PRTS_XR_F_HAS_POSE] = 0;
    d[PRTS_XR_F_VIEW_COUNT] = 1;
    ASSERT_TRUE(prts_xr_apply_frame(&xr, d, PRTS_XR_FRAME_FLOATS));
    ASSERT_FALSE(xr.has_pose);
    ASSERT_EQ(0, xr.view_count);
    ASSERT_EQ(0, xr.views[0].viewport[2]);
    ASSERT_TRUE(c->connected);
}

static prts_engine *boot_headless(const char *data, char *err)
{
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = data;
    cfg.user_dir = PRTS_TEST_TMP "/user";
    cfg.headless = true;
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine *e = prts_engine_create(&cfg, err, 1024);
    prts_log_set_min_level(PRTS_LOG_INFO);
    return e;
}

UTEST(engine, status_for_test_hooks_and_scheme_hint)
{
    /* Spec "Phone browser": the web glue's touch-first hint selects the touch scheme. */
    char err[1024];
    prts_engine *e = boot_headless(PRTS_TEST_DATA, err);
    ASSERT_TRUE(e != NULL);
    prts_engine_status s;
    prts_engine_get_status(e, &s);
    ASSERT_TRUE(s.has_commander);
    ASSERT_EQ((uint64_t)0, s.frames);
    ASSERT_EQ((int)PRTS_SCHEME_KBM, s.scheme);
    double y0 = s.commander[1];
    prts_platform_hint_scheme(PRTS_SCHEME_TOUCH);
    ASSERT_TRUE(prts_engine_frame(e, 1.0 / 30.0));
    prts_engine_get_status(e, &s);
    ASSERT_EQ((uint64_t)1, s.frames);
    ASSERT_EQ((uint64_t)1, s.tick);
    ASSERT_EQ((int)PRTS_SCHEME_TOUCH, s.scheme);
    ASSERT_TRUE(isfinite(s.commander[0]) && isfinite(s.yaw) && isfinite(s.pitch));
    ASSERT_TRUE(fabs(s.commander[1] - y0) < 1.0);
    /* the user area of the running engine, as the browser hooks use it */
    prts_vfs *vfs = prts_engine_vfs(e);
    ASSERT_EQ(PRTS_VFS_OK, prts_vfs_user_write(vfs, "settings/hook.txt", "on", 2));
    char *text = NULL;
    ASSERT_EQ(PRTS_VFS_OK, prts_vfs_user_read(vfs, "settings/hook.txt", &text, NULL));
    ASSERT_STREQ("on", text);
    prts_free(text);
    prts_engine_destroy(e);

    e = boot_headless(PRTS_TEST_FIXTURES "/data_tests", err);
    ASSERT_TRUE(e != NULL);
    prts_engine_get_status(e, &s);
    ASSERT_FALSE(s.has_commander);
    ASSERT_EQ(0.0, s.commander[0]);
    prts_engine_destroy(e);
}
