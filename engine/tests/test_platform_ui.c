/* Tests: UI-related device input (fingers, typed text, pointer position), key capture for
 * rebinding, and the console's text input in a window. MIT License. */
#include <SDL3/SDL.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_log.h"
#include "utest.h"

#define TMP PRTS_TEST_TMP

static bool display(void) { return getenv("DISPLAY") != NULL || getenv("WAYLAND_DISPLAY") != NULL; }

static prts_engine *start(bool headless, const char *user)
{
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = PRTS_TEST_DATA;
    cfg.user_dir = user;
    cfg.headless = headless;
    cfg.start_map = "proving_ground"; /* windowed runs would show the front-end menu */
    cfg.hidden = true;
    cfg.width = 320;
    cfg.height = 180;
    char err[1024];
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    prts_log_set_min_level(PRTS_LOG_INFO);
    return e;
}

static const char *eval(prts_engine *e, const char *expr)
{
    static char out[256];
    char code[512];
    snprintf(code, sizeof(code), "__r = tostring(%s)", expr);
    if (!prts_script_run_string(e->script, code, "=test")) return "<error>";
    lua_State *L = prts_script_lua(e->script);
    lua_getglobal(L, "__r");
    snprintf(out, sizeof(out), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return out;
}

static void frames(prts_engine *e, int n)
{
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    for (int i = 0; i < n; i++) prts_engine_frame(e, 1.0 / 30.0);
    prts_log_set_min_level(PRTS_LOG_INFO);
}

UTEST(rebind, capture_takes_the_next_key_or_mouse_button)
{
    prts_engine *e = start(true, TMP "/capture");
    ASSERT_TRUE(e != NULL);
    e->prev_dev = e->dev;
    e->dev.mouse[3] = true;
    ASSERT_STREQ("3", eval(e, "prts.input.capture('kbm').mouse_buttons[1]"));
    e->dev.keys[prts_platform_key_from_name("F")] = true;
    ASSERT_STREQ("F", eval(e, "prts.input.capture('kbm').keys[1]"));
    prts_engine_destroy(e);
}

UTEST(platform, fingers_text_and_pointer_position)
{
    if (!display()) UTEST_SKIP("no display");
    prts_platform_config pc = {"test", 64, 64, true, 3, 3};
    prts_platform *p = prts_platform_create(&pc);
    ASSERT_TRUE(p != NULL);
    prts_input_state s;
    memset(&s, 0, sizeof(s));
    SDL_Event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = SDL_EVENT_FINGER_DOWN;
    ev.tfinger.fingerID = 42;
    ev.tfinger.x = 0.25f;
    ev.tfinger.y = 0.75f;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_TRUE(s.touches[0].down);
    ASSERT_EQ((int64_t)42, s.touches[0].id);
    ev.type = SDL_EVENT_FINGER_MOTION;
    ev.tfinger.x = 0.5f;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_EQ(0.5f, s.touches[0].x);
    ev.tfinger.fingerID = 43;
    ev.type = SDL_EVENT_FINGER_DOWN;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_EQ((int64_t)43, s.touches[1].id);
    ev.type = SDL_EVENT_FINGER_CANCELED;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_FALSE(s.touches[1].down);
    ev.tfinger.fingerID = 42;
    ev.type = SDL_EVENT_FINGER_UP;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_FALSE(s.touches[0].down);
    ASSERT_EQ(0, s.fingers);
    for (int i = 0; i <= PRTS_INPUT_MAX_TOUCHES; i++) { /* more fingers than slots */
        ev.type = SDL_EVENT_FINGER_DOWN;
        ev.tfinger.fingerID = (SDL_FingerID)(100 + i);
        prts_platform_apply_event(p, &ev, &s);
    }
    ASSERT_EQ(PRTS_INPUT_MAX_TOUCHES + 1, s.fingers);
    ev.type = SDL_EVENT_TEXT_INPUT;
    ev.text.text = "h\xc3\xa9";
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_STREQ("h\xc3\xa9", s.text);
    ev.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    ev.button.button = 1;
    ev.button.down = true;
    ev.button.x = 12.0f;
    ev.button.y = 34.0f;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_EQ(34.0f, s.mouse_y);
    ev.type = SDL_EVENT_MOUSE_MOTION;
    ev.motion.x = 20.0f;
    ev.motion.y = 30.0f;
    prts_platform_apply_event(p, &ev, &s);
    ASSERT_EQ(20.0f, s.mouse_x);
    prts_platform_begin_frame(&s);
    ASSERT_STREQ("", s.text);
    prts_platform_text_input(p, true);
    prts_platform_text_input(p, false);
    ASSERT_STREQ("E", prts_platform_key_name(prts_platform_key_from_name("E")));
    prts_platform_destroy(p);
}

UTEST(console, text_input_follows_the_console_in_a_window)
{
    if (!display()) UTEST_SKIP("no display");
#if !defined(PRTS_DEV)
    UTEST_SKIP("release build: no developer console");
#endif
    prts_engine *e = start(false, TMP "/gl_console");
    ASSERT_TRUE(e != NULL);
    frames(e, 1);
    ASSERT_TRUE(e->mouse_relative); /* playing: the mouse turns the view */
    prts_console_set_open(e->console, true);
    frames(e, 1);
    ASSERT_TRUE(e->text_input);
    ASSERT_FALSE(e->mouse_relative); /* the cursor is free while the console is open */
    snprintf(e->dev.text, sizeof(e->dev.text), "%s", "1+`1"); /* the console key is never typed */
    e->dev.keys[prts_platform_key_from_name("Return")] = true;
    frames(e, 1);
    e->dev.keys[prts_platform_key_from_name("Return")] = false;
    ASSERT_STREQ("1+1", prts_console_last_command(e->console));
    ASSERT_STREQ("2", prts_console_line(e->console, prts_console_line_count(e->console) - 1));
    prts_console_set_open(e->console, false);
    frames(e, 1);
    ASSERT_FALSE(e->text_input);
    prts_engine_destroy(e);
}
