/* 1stPRTS engine: the user interface in the frame loop: the screen overlay and world surfaces,
 * the developer console, the developer observer camera, touch controls, mouse capture and
 * the scheme-changed event. MIT License. */
#include <lauxlib.h>
#include <math.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_log.h"

#if defined(PRTS_DEV)
#define PRTS_DEV_BUILD true
#else
#define PRTS_DEV_BUILD false /* First-Person Contract: no console and no observer camera */
#endif

#define SURFACE_REACH 4.0f /* meters: how far the eye ray drives world surfaces */
#define OBSERVER_SPEED 8.0f
#define OBSERVER_FAST 30.0f
#define OBSERVER_LOOK 0.0025f

static const char *OBSERVER_LABEL = "DEVELOPER VIEW - observer camera (not part of the game)";

/* Key state by name. The names used here are string literals, so their scancodes are looked
 * up once and remembered by address. */
static bool key_down(const prts_engine *e, const char *name)
{
    static struct {
        const char *name;
        int code;
    } cache[24];
    int code = -1;
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(cache) && code < 0; i++) {
        if (cache[i].name == name) code = cache[i].code;
        if (!cache[i].name) {
            cache[i].name = name;
            code = cache[i].code = prts_platform_key_from_name(name);
        }
    }
    return code > 0 && code < PRTS_INPUT_MAX_KEYS && e->dev.keys[code];
}

/* --- font ----------------------------------------------------------------------------- */

static int int_field(lua_State *L, int t, const char *key, int def)
{
    lua_getfield(L, t, key);
    int v = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : def;
    lua_pop(L, 1);
    return v;
}

static void int_pair(lua_State *L, int t, const char *key, int *a, int *b)
{
    if (lua_getfield(L, t, key) == LUA_TTABLE) {
        lua_rawgeti(L, -1, 1);
        lua_rawgeti(L, -2, 2);
        if (lua_isinteger(L, -2) && lua_isinteger(L, -1)) {
            *a = (int)lua_tointeger(L, -2);
            *b = (int)lua_tointeger(L, -1);
        }
        lua_pop(L, 2);
    }
    lua_pop(L, 1);
}

/* Reads the metrics table (fonts/ui.lua, plain data) on top of the stack into f. */
static void read_font(lua_State *L, prts_ui_font *f)
{
    int t = lua_gettop(L);
    lua_getfield(L, t, "texture");
    if (lua_isstring(L, -1)) snprintf(f->texture, sizeof(f->texture), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    int_pair(L, t, "atlas", &f->atlas_w, &f->atlas_h);
    int_pair(L, t, "cell", &f->cell_w, &f->cell_h);
    f->height = int_field(L, t, "height", f->height);
    f->line_height = int_field(L, t, "line_height", f->line_height);
    f->columns = PRTS_MAX(1, int_field(L, t, "columns", f->columns));
    f->first = int_field(L, t, "first", f->first);
    if (lua_getfield(L, t, "advance") == LUA_TTABLE) {
        f->count = PRTS_MIN((int)lua_rawlen(L, -1), PRTS_UI_FONT_GLYPHS);
        for (int i = 0; i < f->count; i++) {
            lua_rawgeti(L, -1, i + 1);
            f->advance[i] = (unsigned char)PRTS_CLAMP((int)lua_tointeger(L, -1), 0, 255);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    if (lua_getfield(L, t, "icons") == LUA_TTABLE) {
        for (int i = 0; i < PRTS_UI_ICON_COUNT; i++) {
            if (lua_getfield(L, -1, prts_ui_icon_name((prts_ui_icon)i)) == LUA_TTABLE) {
                for (int k = 0; k < 4; k++) {
                    lua_rawgeti(L, -1, k + 1);
                    f->icons[i][k] = (int)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}

static void load_font(prts_engine *e)
{
    prts_ui_font f;
    prts_ui_font_default(&f);
    char *code = NULL;
    lua_State *L = prts_script_lua(e->script);
    if (prts_vfs_read(e->vfs, "fonts/ui.lua", &code, NULL) == PRTS_VFS_OK &&
        prts_script_eval_data(e->script, code, "=fonts/ui.lua")) {
        if (lua_istable(L, -1)) read_font(L, &f);
        lua_pop(L, 1);
    } else {
        prts_log_warn("the UI font fonts/ui.lua is missing or broken; using fallback metrics");
    }
    prts_free(code);
    prts_ui_set_font(e->ui, &f);
}

/* --- lifecycle --------------------------------------------------------------------------- */

void prts_engine_ui_create(prts_engine *e)
{
    e->developer = PRTS_DEV_BUILD && !e->cfg.no_dev;
    e->screen_w = e->cfg.width;
    e->screen_h = e->cfg.height;
    e->ui = prts_ui_create();
    e->touch = prts_touch_create();
    e->gfx_ui = prts_gfx_ui_create(e->gfx);
    load_font(e);
    if (e->developer) {
        e->console = prts_console_create(400, prts_engine_console_exec, e);
        prts_log_add_listener(prts_console_log_listener, e->console);
    }
    prts_engine_load_settings(e);
    prts_engine_open_settings_api(e);
    prts_engine_open_ui_api(e);
    prts_engine_open_app_api(e);
}

void prts_engine_ui_destroy(prts_engine *e)
{
    if (e->console) {
        prts_log_remove_listener(prts_console_log_listener, e->console);
        prts_console_destroy(e->console);
        e->console = NULL;
    }
    prts_gfx_ui_destroy(e->gfx_ui);
    prts_ui_destroy(e->ui);
    prts_touch_destroy(e->touch);
    e->gfx_ui = NULL;
    e->ui = NULL;
    e->touch = NULL;
}

/* --- observer camera (developer builds only) ---------------------------------------- */

bool prts_engine_set_observer(prts_engine *e, bool on)
{
    if (!e->developer) return false;
    if (on && !e->observer.active) {
        e->observer.pos = e->has_commander ? prts_commander_eye(&e->commander, e->world, 1.0) : prts_dv3(0, 1.7, 0);
        e->observer.yaw = e->commander.yaw;
        e->observer.pitch = e->commander.pitch;
    }
    if (on != e->observer.active) prts_log_info("observer camera %s (developer view)", on ? "on" : "off");
    e->observer.active = on;
    return true;
}

bool prts_engine_camera_override(const prts_engine *e, prts_camera *cam)
{
    if (!e->developer || !e->observer.active) return false;
    cam->eye = e->observer.pos;
    cam->yaw = e->observer.yaw;
    cam->pitch = e->observer.pitch;
    return true;
}

static void move_observer(prts_engine *e, float dt)
{
    if (e->console && prts_console_is_open(e->console)) return;
    e->observer.yaw = prts_wrap_angle(e->observer.yaw - e->ui_mouse_dx * OBSERVER_LOOK);
    e->observer.pitch = prts_clampf(e->observer.pitch - e->ui_mouse_dy * OBSERVER_LOOK, -1.5f, 1.5f);
    prts_vec3 fwd = prts_view_forward(e->observer.yaw, e->observer.pitch);
    prts_vec3 right = prts_v3_normalize(prts_v3_cross(fwd, prts_v3(0, 1, 0)));
    float f = (key_down(e, "W") ? 1.0f : 0.0f) - (key_down(e, "S") ? 1.0f : 0.0f);
    float r = (key_down(e, "D") ? 1.0f : 0.0f) - (key_down(e, "A") ? 1.0f : 0.0f);
    float u = (key_down(e, "Space") ? 1.0f : 0.0f) - (key_down(e, "Left Ctrl") ? 1.0f : 0.0f);
    prts_vec3 move = prts_v3_add(prts_v3_add(prts_v3_scale(fwd, f), prts_v3_scale(right, r)), prts_v3(0, u, 0));
    float speed = (key_down(e, "Left Shift") ? OBSERVER_FAST : OBSERVER_SPEED) * dt;
    e->observer.pos = prts_dv3_add(e->observer.pos, prts_dv3_from_v3(prts_v3_scale(move, speed)));
}

/* --- per frame ---------------------------------------------------------------------------- */

static void set_platform_modes(prts_engine *e, bool console_open)
{
    if (!e->platform) return;
    /* the mouse is captured for looking, but never in the touch scheme: fingers need absolute
     * positions, and browsers report touches under pointer lock at a fixed point */
    bool relative = !e->cfg.scenario && !e->ui_modal && !console_open && (e->has_commander || e->observer.active) &&
                    prts_input_scheme_active(e->input) != PRTS_SCHEME_TOUCH;
    if (relative != e->mouse_relative) {
        prts_platform_set_relative_mouse(e->platform, relative);
        e->mouse_relative = relative;
    }
    if (console_open != e->text_input) {
        prts_platform_text_input(e->platform, console_open);
        e->text_input = console_open;
    }
}

void prts_engine_ui_before_input(prts_engine *e, double dt)
{
    if (e->platform) prts_platform_drawable_size(e->platform, &e->screen_w, &e->screen_h);
    prts_ui_set_screen(e->ui, e->screen_w, e->screen_h);
    prts_touch_update(e->touch, &e->dev, (float)dt, (float)e->screen_w / (float)PRTS_MAX(1, e->screen_h));
    /* typed text, wheel and mouse motion of this frame wait for the UI pass (the device
     * state clears them before the ticks run); the console key is never typed */
    size_t n = strlen(e->ui_text);
    for (const char *p = e->dev.text; *p && n + 1 < sizeof(e->ui_text); p++) {
        if (*p != '`') e->ui_text[n++] = *p;
    }
    e->ui_text[n] = '\0';
    e->ui_wheel += e->dev.wheel;
    e->ui_mouse_dx += e->dev.mouse_dx;
    e->ui_mouse_dy += e->dev.mouse_dy;
    bool key = key_down(e, "`");
    if (e->console && key && !e->prev_console_key) prts_console_set_open(e->console, !prts_console_is_open(e->console));
    e->prev_console_key = key;
    bool console_open = e->console && prts_console_is_open(e->console);
    /* menus, the console and the observer camera take the controls from the commander */
    prts_input_set_blocked(e->input, e->ui_modal || console_open || e->observer.active);
    set_platform_modes(e, console_open);
}

static void emit_ui(prts_engine *e, int s)
{
    lua_State *L = prts_script_lua(e->script);
    int w = 0, h = 0;
    prts_ui_surface_size(e->ui, s, &w, &h);
    lua_pushstring(L, prts_ui_surface_name(e->ui, s));
    lua_pushinteger(L, w);
    lua_pushinteger(L, h);
    prts_script_emit(e->script, "ui", 3);
}

static const prts_touch_point *first_finger(const prts_engine *e)
{
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) {
        if (e->dev.touches[i].down) return &e->dev.touches[i];
    }
    return NULL;
}

static void build_world_surface(prts_engine *e, int s)
{
    prts_ui_input in;
    memset(&in, 0, sizeof(in));
    prts_ui_quad q;
    bool console_open = e->console && prts_console_is_open(e->console);
    if (e->has_commander && !e->ui_modal && !console_open && prts_ui_surface_quad(e->ui, s, &q)) {
        /* the eye ray (mouse and gamepad aim with the view), or the ray through a finger */
        const prts_commander *c = &e->commander;
        prts_vec3 dir = prts_view_forward(c->yaw, c->pitch);
        const prts_touch_point *f = first_finger(e);
        if (f && prts_input_scheme_active(e->input) == PRTS_SCHEME_TOUCH) {
            dir = prts_ui_screen_ray(c->yaw, c->pitch, e->render.fov_y,
                                     (float)e->screen_w / (float)PRTS_MAX(1, e->screen_h), f->x, f->y);
        }
        float u = 0.0f, v = 0.0f, dist = 0.0f;
        int w = 0, h = 0;
        prts_ui_surface_size(e->ui, s, &w, &h);
        if (prts_ui_ray_quad(&q, prts_commander_eye(c, e->world, 1.0), dir, SURFACE_REACH, &u, &v, &dist)) {
            in.has_pointer = true;
            in.x = u * (float)w;
            in.y = v * (float)h;
            in.down = prts_input_down(e->input, "use") || e->dev.mouse[1] || f != NULL;
        }
    }
    prts_ui_feed(e->ui, s, &in);
    prts_ui_begin(e->ui, s);
    emit_ui(e, s);
    prts_ui_end(e->ui);
}

static unsigned ui_keys(const prts_engine *e)
{
    unsigned k = 0;
    if (key_down(e, "Left Shift") || key_down(e, "Right Shift")) k |= PRTS_UI_KEY_SHIFT;
    if (key_down(e, "Left Ctrl") || key_down(e, "Right Ctrl")) k |= PRTS_UI_KEY_CTRL;
    if (key_down(e, "Left Alt") || key_down(e, "Right Alt")) k |= PRTS_UI_KEY_ALT;
    if (key_down(e, "Backspace")) k |= PRTS_UI_KEY_BACKSPACE;
    if (key_down(e, "Return") || key_down(e, "Keypad Enter")) k |= PRTS_UI_KEY_RETURN;
    return k;
}

static void draw_observer_label(prts_engine *e, int w, int h)
{
    static const unsigned char bg[4] = {0, 0, 0, 180}, red[4] = {230, 40, 30, 255}, text[4] = {255, 120, 90, 255};
    float fw = (float)w, fh = (float)h;
    prts_ui_draw_rect(e->ui, 0, 0, fw, 2, red);
    prts_ui_draw_rect(e->ui, 0, fh - 2, fw, 2, red);
    prts_ui_draw_rect(e->ui, 0, 0, 2, fh, red);
    prts_ui_draw_rect(e->ui, fw - 2, 0, 2, fh, red);
    float tw = (float)prts_ui_text_width(prts_ui_get_font(e->ui), OBSERVER_LABEL, -1);
    prts_ui_draw_rect(e->ui, floorf(fw * 0.5f - tw * 0.5f) - 4, fh - 20, tw + 8, 14, bg);
    prts_ui_draw_text(e->ui, OBSERVER_LABEL, fw * 0.5f, fh - 17, PRTS_UI_ALIGN_CENTER, 1, text);
}

static void build_screen(prts_engine *e)
{
    prts_ui_input in;
    memset(&in, 0, sizeof(in));
    int w = 0, h = 0;
    prts_ui_surface_size(e->ui, PRTS_UI_SCREEN, &w, &h);
    float scale = (float)prts_ui_scale_for(e->screen_h);
    const prts_touch_point *f = first_finger(e);
    if (e->test_pointer) {
        in.has_pointer = true;
        in.x = e->test_x;
        in.y = e->test_y;
        in.down = e->test_down;
    } else if (f && prts_input_scheme_active(e->input) == PRTS_SCHEME_TOUCH) {
        in.has_pointer = true;
        in.x = f->x * (float)e->screen_w / scale;
        in.y = f->y * (float)e->screen_h / scale;
        in.down = true;
    } else if (e->platform && !e->mouse_relative) {
        in.has_pointer = true;
        in.x = e->dev.mouse_x / scale;
        in.y = e->dev.mouse_y / scale;
        in.down = e->dev.mouse[1];
    }
    in.scroll = e->ui_wheel;
    in.keys = ui_keys(e);
    in.text = e->ui_text;
    prts_ui_feed(e->ui, PRTS_UI_SCREEN, &in);
    prts_ui_begin(e->ui, PRTS_UI_SCREEN);
    emit_ui(e, PRTS_UI_SCREEN);
    if (e->console) prts_console_draw(e->console, e->ui, w, h);
    if (e->developer && e->observer.active) draw_observer_label(e, w, h);
    prts_ui_end(e->ui);
}

void prts_engine_ui_frame(prts_engine *e, double dt)
{
    if (prts_input_take_scheme_changed(e->input)) {
        lua_State *L = prts_script_lua(e->script);
        lua_pushstring(L, prts_input_scheme_name(prts_input_scheme_active(e->input)));
        prts_script_emit(e->script, "input_scheme", 1);
    }
    if (e->developer && e->observer.active) move_observer(e, (float)dt);
    for (int s = 1; s < PRTS_UI_MAX_SURFACES; s++) {
        if (prts_ui_surface_name(e->ui, s)) build_world_surface(e, s);
    }
    build_screen(e);
    e->ui_text[0] = '\0';
    e->ui_wheel = e->ui_mouse_dx = e->ui_mouse_dy = 0.0f;
    e->prev_dev = e->dev;
}

void prts_engine_ui_test_frame(prts_engine *e)
{
    prts_engine_ui_before_input(e, e->clock.tick_seconds);
    prts_input_update(e->input, &e->dev);
    prts_platform_begin_frame(&e->dev);
    prts_engine_ui_frame(e, e->clock.tick_seconds);
}

void prts_engine_ui_draw_surfaces(prts_engine *e)
{
    for (int s = 1; s < PRTS_UI_MAX_SURFACES; s++) {
        const char *name = prts_ui_surface_name(e->ui, s);
        if (!name) continue;
        prts_gfx_ui_draw_surface(e->gfx_ui, s, prts_ui_batch_of(e->ui, s));
        /* materials with `surface = <id>` show it in the world */
        prts_gfx_set_named_texture(e->gfx, name, prts_gfx_ui_surface_texture(e->gfx_ui, s));
    }
}

void prts_engine_ui_draw_overlay(prts_engine *e, int w, int h)
{
    prts_gfx_ui_draw_screen(e->gfx_ui, prts_ui_batch_of(e->ui, PRTS_UI_SCREEN), w, h);
}

/* --- developer console commands ------------------------------------------------------ */

static void console_reload(prts_engine *e)
{
    lua_State *L = prts_script_lua(e->script);
    int top = lua_gettop(L);
    lua_getglobal(L, "prts");
    bool found = lua_getfield(L, -1, "dev") == LUA_TTABLE && lua_getfield(L, -1, "reload") == LUA_TFUNCTION;
    if (found) {
        lua_replace(L, top + 1);
        lua_settop(L, top + 1);
        if (prts_script_call(e->script, 0, 0)) prts_console_print(e->console, PRTS_LOG_INFO, "scripts reloaded");
    } else {
        prts_console_print(e->console, PRTS_LOG_WARN,
                           "reload: hot reload (prts.dev.reload) is not available in this build; nothing was reloaded");
    }
    lua_settop(L, top);
}

/* Runs a line of Lua in the game's shared script environment. Expressions print their values. */
static void console_lua(prts_engine *e, const char *line)
{
    lua_State *L = prts_script_lua(e->script);
    int top = lua_gettop(L);
    prts_str expr = {0};
    prts_str_appendf(&expr, "return %s", line);
    bool is_expr = luaL_loadbufferx(L, expr.data, expr.len, "=console", "t") == LUA_OK;
    prts_str_free(&expr);
    if (!is_expr) {
        lua_pop(L, 1);
        if (luaL_loadbufferx(L, line, strlen(line), "=console", "t") != LUA_OK) {
            prts_console_print(e->console, PRTS_LOG_ERROR, lua_tostring(L, -1));
            lua_pop(L, 1);
            return;
        }
    }
    if (!prts_script_call(e->script, 0, LUA_MULTRET)) return;
    prts_str out = {0};
    for (int i = top + 1; i <= lua_gettop(L); i++) {
        if (i > top + 1) prts_str_append(&out, "\t");
        prts_str_append(&out, luaL_tolstring(L, i, NULL));
        lua_pop(L, 1);
    }
    if (out.data) prts_console_print(e->console, PRTS_LOG_INFO, out.data);
    prts_str_free(&out);
    lua_settop(L, top);
}

void prts_engine_console_exec(const char *line, void *user)
{
    prts_engine *e = user;
    if (strcmp(line, "help") == 0) {
        prts_console_print(e->console, PRTS_LOG_INFO,
                           "commands: help, clear, reload, observer (developer camera); anything else runs as Lua\n"
                           "close the console with the ` key");
    } else if (strcmp(line, "clear") == 0) {
        prts_console_clear(e->console);
    } else if (strcmp(line, "reload") == 0) {
        console_reload(e);
    } else if (strcmp(line, "observer") == 0) {
        prts_engine_set_observer(e, !e->observer.active);
    } else {
        console_lua(e, line);
    }
}
