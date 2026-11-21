/* 1stPRTS engine: the UI scripting API (prts.ui) and the touch layout (prts.input.touch_*).
 * Scripts never see microui: they get windows, rows, widgets and direct drawing. MIT License. */
#include <lauxlib.h>
#include <math.h>
#include <string.h>

#include "app/engine_internal.h"

static prts_ui *UI(lua_State *L) { return prts_engine_from_lua(L)->ui; }

static int ui_fail(lua_State *L, prts_ui *ui)
{
    const char *err = prts_ui_error(ui);
    return luaL_error(L, "%s", err ? err : "the UI call failed");
}

static int check_int(lua_State *L, int idx) { return (int)lroundf((float)luaL_checknumber(L, idx)); }

static bool opt_field_bool(lua_State *L, int t, const char *key, bool def)
{
    if (!lua_istable(L, t)) return def;
    lua_getfield(L, t, key);
    bool v = lua_isnil(L, -1) ? def : lua_toboolean(L, -1);
    lua_pop(L, 1);
    return v;
}

static float opt_field_number(lua_State *L, int t, const char *key, float def)
{
    if (!lua_istable(L, t)) return def;
    lua_getfield(L, t, key);
    float v = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : def;
    lua_pop(L, 1);
    return v;
}

/* Reads { r, g, b [, a] } (0..255) at idx; anything else gives def. */
static void read_color(lua_State *L, int idx, unsigned char out[4], const unsigned char def[4])
{
    memcpy(out, def, 4);
    if (!lua_istable(L, idx)) return;
    idx = lua_absindex(L, idx);
    for (int i = 0; i < 4; i++) {
        lua_rawgeti(L, idx, i + 1);
        if (lua_isnumber(L, -1)) out[i] = (unsigned char)PRTS_CLAMP((int)lua_tonumber(L, -1), 0, 255);
        lua_pop(L, 1);
    }
}

static const unsigned char WHITE[4] = {255, 255, 255, 255};

/* Calls the function at idx with no arguments, then always runs `end`; rethrows errors, so
 * a failing script never leaves a window or panel open. */
static int call_then(lua_State *L, int idx, prts_ui *ui, void (*end)(prts_ui *))
{
    lua_pushvalue(L, idx);
    int rc = lua_pcall(L, 0, 0, 0);
    end(ui);
    if (rc != LUA_OK) return lua_error(L);
    return 0;
}

/* prts.ui.window(title, x, y, w, h, [opts], fn): opts = { title, frame, scroll, input } (booleans) */
static int l_window(lua_State *L)
{
    prts_ui *ui = UI(L);
    const char *title = luaL_checkstring(L, 1);
    int x = check_int(L, 2), y = check_int(L, 3), w = check_int(L, 4), h = check_int(L, 5);
    int fn = lua_isfunction(L, 6) ? 6 : 7;
    luaL_checktype(L, fn, LUA_TFUNCTION);
    unsigned flags = 0;
    flags |= opt_field_bool(L, 6, "title", true) ? PRTS_UI_WIN_TITLE : 0u;
    flags |= opt_field_bool(L, 6, "frame", true) ? PRTS_UI_WIN_FRAME : 0u;
    flags |= opt_field_bool(L, 6, "scroll", true) ? PRTS_UI_WIN_SCROLL : 0u;
    flags |= opt_field_bool(L, 6, "input", true) ? PRTS_UI_WIN_INPUT : 0u;
    if (!prts_ui_window_begin(ui, title, x, y, w, h, flags)) return ui_fail(L, ui);
    return call_then(L, fn, ui, prts_ui_window_end);
}

/* prts.ui.panel(name, fn, [scroll_to_end]): a scrollable area in the next layout cell */
static int l_panel(lua_State *L)
{
    prts_ui *ui = UI(L);
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    if (!prts_ui_panel_begin(ui, name, lua_toboolean(L, 3))) return ui_fail(L, ui);
    return call_then(L, 2, ui, prts_ui_panel_end);
}

/* prts.ui.row({ widths }, [height]) */
static int l_row(lua_State *L)
{
    prts_ui *ui = UI(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    int widths[16] = {0};
    int n = (int)lua_rawlen(L, 1);
    for (int i = 0; i < n && i < 16; i++) {
        lua_rawgeti(L, 1, i + 1);
        widths[i] = (int)lroundf((float)lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    if (!prts_ui_row(ui, n, widths, (int)luaL_optinteger(L, 2, 0))) return ui_fail(L, ui);
    return 0;
}

static int l_label(lua_State *L)
{
    prts_ui *ui = UI(L);
    if (!prts_ui_label(ui, luaL_checkstring(L, 1), lua_toboolean(L, 2))) return ui_fail(L, ui);
    return 0;
}

static int l_text(lua_State *L)
{
    prts_ui *ui = UI(L);
    if (!prts_ui_text(ui, luaL_checkstring(L, 1))) return ui_fail(L, ui);
    return 0;
}

static int l_button(lua_State *L)
{
    prts_ui *ui = UI(L);
    bool clicked = false;
    if (!prts_ui_button(ui, luaL_checkstring(L, 1), luaL_optstring(L, 2, NULL), &clicked)) return ui_fail(L, ui);
    lua_pushboolean(L, clicked);
    return 1;
}

static int l_checkbox(lua_State *L)
{
    prts_ui *ui = UI(L);
    bool value = lua_toboolean(L, 2), changed = false;
    if (!prts_ui_checkbox(ui, luaL_checkstring(L, 1), luaL_optstring(L, 3, NULL), &value, &changed)) {
        return ui_fail(L, ui);
    }
    lua_pushboolean(L, value);
    lua_pushboolean(L, changed);
    return 2;
}

static int l_slider(lua_State *L)
{
    prts_ui *ui = UI(L);
    float value = (float)luaL_checknumber(L, 2);
    bool changed = false;
    if (!prts_ui_slider(ui, luaL_checkstring(L, 1), &value, (float)luaL_checknumber(L, 3),
                        (float)luaL_checknumber(L, 4), (float)luaL_optnumber(L, 5, 0.0), &changed)) {
        return ui_fail(L, ui);
    }
    lua_pushnumber(L, value);
    lua_pushboolean(L, changed);
    return 2;
}

/* prts.ui.textbox(id, text, [focus]) -> text, submitted */
static int l_textbox(lua_State *L)
{
    prts_ui *ui = UI(L);
    char buf[256];
    size_t len = 0;
    const char *text = luaL_optlstring(L, 2, "", &len);
    len = PRTS_MIN(len, sizeof(buf) - 1);
    memcpy(buf, text, len);
    buf[len] = '\0';
    bool changed = false, submitted = false;
    if (!prts_ui_textbox(ui, luaL_checkstring(L, 1), buf, (int)sizeof(buf), lua_toboolean(L, 3), &changed,
                         &submitted)) {
        return ui_fail(L, ui);
    }
    lua_pushstring(L, buf);
    lua_pushboolean(L, submitted);
    return 2;
}

/* prts.ui.draw_text(text, x, y, [{ align = "left"|"center"|"right", scale = 1, color = {r, g, b, a} }]) */
static int l_draw_text(lua_State *L)
{
    prts_ui *ui = UI(L);
    const char *text = luaL_checkstring(L, 1);
    int align = PRTS_UI_ALIGN_LEFT;
    unsigned char color[4];
    memcpy(color, WHITE, 4);
    if (lua_istable(L, 4)) {
        lua_getfield(L, 4, "align");
        const char *a = lua_tostring(L, -1);
        if (a && strcmp(a, "center") == 0) align = PRTS_UI_ALIGN_CENTER;
        else if (a && strcmp(a, "right") == 0) align = PRTS_UI_ALIGN_RIGHT;
        lua_pop(L, 1);
        lua_getfield(L, 4, "color");
        read_color(L, -1, color, WHITE);
        lua_pop(L, 1);
    }
    int scale = (int)opt_field_number(L, 4, "scale", 1.0f);
    if (!prts_ui_draw_text(ui, text, (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), align, scale, color)) {
        return ui_fail(L, ui);
    }
    return 0;
}

static int l_draw_rect(lua_State *L)
{
    prts_ui *ui = UI(L);
    unsigned char color[4];
    read_color(L, 5, color, WHITE);
    if (!prts_ui_draw_rect(ui, (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2),
                           (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4), color)) {
        return ui_fail(L, ui);
    }
    return 0;
}

static int l_draw_icon(lua_State *L)
{
    prts_ui *ui = UI(L);
    prts_ui_icon icon;
    const char *name = luaL_checkstring(L, 1);
    if (!prts_ui_icon_parse(name, &icon)) return luaL_error(L, "unknown icon '%s'", name);
    unsigned char color[4];
    read_color(L, 6, color, WHITE);
    if (!prts_ui_draw_icon(ui, icon, (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3),
                           (float)luaL_checknumber(L, 4), (float)luaL_checknumber(L, 5), color)) {
        return ui_fail(L, ui);
    }
    return 0;
}

static int l_text_width(lua_State *L)
{
    lua_pushinteger(L, prts_ui_text_width(prts_ui_get_font(UI(L)), luaL_checkstring(L, 1), -1));
    return 1;
}

/* prts.ui.size() -> width, height of the surface being drawn (the screen outside a frame) */
static int l_size(lua_State *L)
{
    prts_ui *ui = UI(L);
    int s = prts_ui_current(ui), w = 0, h = 0;
    prts_ui_surface_size(ui, s < 0 ? PRTS_UI_SCREEN : s, &w, &h);
    lua_pushinteger(L, w);
    lua_pushinteger(L, h);
    return 2;
}

static int l_font(lua_State *L)
{
    const prts_ui_font *f = prts_ui_get_font(UI(L));
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, f->height);
    lua_setfield(L, -2, "height");
    lua_pushinteger(L, f->line_height);
    lua_setfield(L, -2, "line_height");
    return 1;
}

/* prts.ui.set_modal(bool): a menu takes the pointer and game input stops */
static int l_set_modal(lua_State *L)
{
    prts_engine_from_lua(L)->ui_modal = lua_toboolean(L, 1);
    return 0;
}

static int l_modal(lua_State *L)
{
    lua_pushboolean(L, prts_engine_from_lua(L)->ui_modal);
    return 1;
}

static int l_hovered(lua_State *L)
{
    lua_pushboolean(L, prts_ui_hovered(UI(L), PRTS_UI_SCREEN));
    return 1;
}

static bool field_vec3(lua_State *L, int t, const char *key, prts_vec3 *out)
{
    lua_getfield(L, t, key);
    bool ok = lua_istable(L, -1);
    if (ok) {
        float v[3];
        for (int i = 0; i < 3; i++) {
            lua_rawgeti(L, -1 - i, i + 1);
            v[i] = (float)lua_tonumber(L, -1);
        }
        lua_pop(L, 3);
        *out = prts_v3(v[0], v[1], v[2]);
    }
    lua_pop(L, 1);
    return ok;
}

/* prts.ui.surface{ id, width, height, origin = {x,y,z}, right = {x,y,z}, down = {x,y,z} }:
 * a UI drawn into a texture on a world object; scripts draw it in their 'ui' handler when
 * the event names this id. Returns the surface slot. */
static int l_surface(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "id");
    const char *id = lua_tostring(L, -1);
    lua_pop(L, 1);
    if (!id || strcmp(id, "screen") == 0) return luaL_error(L, "a surface needs an id other than 'screen'");
    int w = (int)opt_field_number(L, 1, "width", 256), h = (int)opt_field_number(L, 1, "height", 256);
    int s = prts_ui_surface_add(e->ui, id, w, h);
    if (s < 0) return luaL_error(L, "cannot add surface '%s' (size 8..1024, at most %d surfaces)", id,
                                 PRTS_UI_MAX_SURFACES - 1);
    prts_vec3 origin;
    prts_ui_quad q;
    if (field_vec3(L, 1, "origin", &origin) && field_vec3(L, 1, "right", &q.right) && field_vec3(L, 1, "down", &q.down)) {
        q.origin = prts_dv3_from_v3(origin);
        prts_ui_surface_set_quad(e->ui, s, &q);
    }
    lua_pushinteger(L, s);
    return 1;
}

/* --- touch layout ------------------------------------------------------------------------ */

/* prts.input.touch_layout{ split, stick_radius, look_speed, tap_distance, tap_seconds,
 *   buttons = { { id, x, y, radius }, ... } }: buttons start hidden. */
static int l_touch_layout(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_touch_layout l;
    prts_touch_layout_default(&l);
    l.split = opt_field_number(L, 1, "split", l.split);
    l.stick_radius = opt_field_number(L, 1, "stick_radius", l.stick_radius);
    l.look_speed = opt_field_number(L, 1, "look_speed", l.look_speed);
    l.tap_distance = opt_field_number(L, 1, "tap_distance", l.tap_distance);
    l.tap_seconds = opt_field_number(L, 1, "tap_seconds", l.tap_seconds);
    lua_getfield(L, 1, "buttons");
    if (lua_istable(L, -1)) {
        int list = lua_gettop(L);
        for (int i = 1; l.button_count < PRTS_TOUCH_MAX_BUTTONS && lua_rawgeti(L, list, i) == LUA_TTABLE; i++) {
            int b = lua_gettop(L);
            prts_touch_button *tb = &l.buttons[l.button_count++];
            lua_getfield(L, b, "id");
            const char *id = lua_tostring(L, -1);
            snprintf(tb->id, sizeof(tb->id), "%s", id ? id : "");
            lua_pop(L, 1);
            tb->x = opt_field_number(L, b, "x", 0.9f);
            tb->y = opt_field_number(L, b, "y", 0.8f);
            tb->radius = opt_field_number(L, b, "radius", 0.07f);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    prts_touch_set_layout(e->touch, &l);
    lua_pushinteger(L, prts_touch_get_layout(e->touch)->button_count);
    return 1;
}

static int l_touch_visible(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    int b = prts_touch_find_button(e->touch, luaL_checkstring(L, 1));
    if (b < 0) return luaL_error(L, "unknown touch button '%s'", lua_tostring(L, 1));
    prts_touch_set_visible(e->touch, b, lua_toboolean(L, 2));
    return 0;
}

/* prts.input.touch_state() -> { stick = { ox, oy, kx, ky } or nil, buttons = { { id, x, y,
 * radius, visible, held, touch_button }, ... } } for drawing the overlay */
static int l_touch_state(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const prts_touch_layout *l = prts_touch_get_layout(e->touch);
    lua_createtable(L, 0, 2);
    float s[4];
    if (prts_touch_stick(e->touch, &s[0], &s[1], &s[2], &s[3])) {
        lua_createtable(L, 4, 0);
        for (int i = 0; i < 4; i++) {
            lua_pushnumber(L, s[i]);
            lua_rawseti(L, -2, i + 1);
        }
        lua_setfield(L, -2, "stick");
    }
    lua_createtable(L, l->button_count, 0);
    for (int i = 0; i < l->button_count; i++) {
        const prts_touch_button *b = &l->buttons[i];
        lua_createtable(L, 0, 7);
        lua_pushstring(L, b->id);
        lua_setfield(L, -2, "id");
        lua_pushnumber(L, b->x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, b->y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, b->radius);
        lua_setfield(L, -2, "radius");
        lua_pushboolean(L, b->visible);
        lua_setfield(L, -2, "visible");
        lua_pushboolean(L, prts_touch_button_held(e->touch, i));
        lua_setfield(L, -2, "held");
        lua_pushinteger(L, i + 1);
        lua_setfield(L, -2, "touch_button");
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "buttons");
    return 1;
}

void prts_engine_open_ui_api(prts_engine *e)
{
    static const prts_script_fn ui[] = {
        {"window", l_window},       {"panel", l_panel},         {"row", l_row},
        {"label", l_label},         {"text", l_text},           {"button", l_button},
        {"checkbox", l_checkbox},   {"slider", l_slider},       {"textbox", l_textbox},
        {"draw_text", l_draw_text}, {"draw_rect", l_draw_rect}, {"draw_icon", l_draw_icon},
        {"text_width", l_text_width}, {"size", l_size},         {"font", l_font},
        {"set_modal", l_set_modal}, {"modal", l_modal},         {"hovered", l_hovered},
        {"surface", l_surface},     {NULL, NULL}};
    static const prts_script_fn input[] = {
        {"touch_layout", l_touch_layout}, {"touch_visible", l_touch_visible}, {"touch_state", l_touch_state},
        {NULL, NULL}};
    prts_script_register(e->script, "ui", ui);
    prts_script_register(e->script, "input", input);
}
