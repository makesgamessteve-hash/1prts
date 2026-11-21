/* 1stPRTS engine: UI helpers for automated game tests (scenario `t`) and Lua unit tests
 * (`T`, which run UI frames synchronously): clicking widgets by label, keys, typed text,
 * input schemes, touch fingers, the console and the observer camera. MIT License. */
#include <lauxlib.h>
#include <stdio.h>
#include <string.h>

#include "app/engine_internal.h"

static const char *UI_PRELUDE =
    "local t, sync = ...\n"
    "local function step(n)\n"
    "  if sync then for _ = 1, n do t._ui_frame() end else t.ticks(n) end\n"
    "end\n"
    "function t.ui_has(text, surface)\n"
    "  for _, s in ipairs(t.ui_texts(surface)) do if s:find(text, 1, true) then return true end end\n"
    "  return false\n"
    "end\n"
    "function t.ui_click(label)\n"
    "  if sync then step(1) end\n"
    "  local x, y = t._ui_find(label)\n"
    "  if not x then error(\"no button or field '\" .. tostring(label) .. \"' on screen\", 2) end\n"
    "  t._ui_pointer(x, y, false) step(2)\n"
    "  t._ui_pointer(x, y, true) step(1)\n"
    "  t._ui_pointer(x, y, false) step(1)\n"
    "  t._ui_pointer()\n"
    "end\n"
    "function t.key(name) t._key(name, true) step(1) t._key(name, false) step(1) end\n"
    "function t.type_text(s) for i = 1, #s, 16 do t._type(s:sub(i, i + 15)) step(1) end end\n"
    "if sync then t.ui_frame = function(n) step(n or 1) end end\n";

static prts_engine *E(lua_State *L) { return prts_engine_from_lua(L); }

static int surface_arg(lua_State *L, int idx)
{
    prts_engine *e = E(L);
    if (lua_isnoneornil(L, idx)) return PRTS_UI_SCREEN;
    int s = prts_ui_surface_find(e->ui, luaL_checkstring(L, idx));
    if (s < 0) luaL_error(L, "unknown UI surface '%s'", lua_tostring(L, idx));
    return s;
}

static int t_ui_find(lua_State *L)
{
    float x = 0.0f, y = 0.0f;
    if (!prts_ui_find_widget(E(L)->ui, PRTS_UI_SCREEN, luaL_checkstring(L, 1), &x, &y)) return 0;
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

static int t_ui_pointer(lua_State *L)
{
    prts_engine *e = E(L);
    e->test_pointer = !lua_isnone(L, 1);
    e->test_x = (float)luaL_optnumber(L, 1, 0.0);
    e->test_y = (float)luaL_optnumber(L, 2, 0.0);
    e->test_down = lua_toboolean(L, 3);
    return 0;
}

static int t_ui_texts(lua_State *L)
{
    prts_engine *e = E(L);
    int s = surface_arg(L, 1);
    size_t n = prts_ui_text_count(e->ui, s);
    lua_createtable(L, (int)n, 0);
    for (size_t i = 0; i < n; i++) {
        lua_pushstring(L, prts_ui_text_at(e->ui, s, i));
        lua_rawseti(L, -2, (lua_Integer)i + 1);
    }
    return 1;
}

static int t_ui_frame(lua_State *L)
{
    prts_engine_ui_test_frame(E(L));
    return 0;
}

static int t_key(lua_State *L)
{
    prts_engine *e = E(L);
    const char *name = luaL_checkstring(L, 1);
    int code = prts_platform_key_from_name(name);
    if (code <= 0 || code >= PRTS_INPUT_MAX_KEYS) return luaL_error(L, "unknown key '%s'", name);
    e->dev.keys[code] = lua_toboolean(L, 2);
    e->dev.last_device = PRTS_SCHEME_KBM;
    return 0;
}

static int t_type_text(lua_State *L)
{
    prts_engine *e = E(L);
    const char *text = luaL_checkstring(L, 1);
    size_t n = strlen(e->ui_text);
    snprintf(e->ui_text + n, sizeof(e->ui_text) - n, "%s", text);
    return 0;
}

/* t.pad_button(code, down) and t.pad_axis(code, value): a connected gamepad */
static int t_pad_button(lua_State *L)
{
    prts_engine *e = E(L);
    lua_Integer b = luaL_checkinteger(L, 1);
    if (b < 0 || b >= PRTS_INPUT_MAX_PAD_BUTTONS) return luaL_error(L, "no gamepad button %d", (int)b);
    e->dev.pad_connected = true;
    e->dev.pad_buttons[b] = lua_toboolean(L, 2);
    e->dev.last_device = PRTS_SCHEME_GAMEPAD;
    return 0;
}

static int t_pad_axis(lua_State *L)
{
    prts_engine *e = E(L);
    lua_Integer a = luaL_checkinteger(L, 1);
    if (a < 0 || a >= PRTS_INPUT_MAX_PAD_AXES) return luaL_error(L, "no gamepad axis %d", (int)a);
    e->dev.pad_connected = true;
    e->dev.pad_axes[a] = (float)luaL_checknumber(L, 2);
    e->dev.last_device = PRTS_SCHEME_GAMEPAD;
    return 0;
}

static int t_scheme(lua_State *L)
{
    prts_engine *e = E(L);
    prts_input_scheme s;
    if (!prts_input_scheme_parse(luaL_checkstring(L, 1), &s)) return luaL_error(L, "unknown scheme '%s'", lua_tostring(L, 1));
    e->dev.last_device = (int)s;
    prts_input_set_scheme(e->input, s);
    return 0;
}

/* t.touch(slot, x, y, down): finger `slot` (1..10) at x, y (fractions of the window) */
static int t_touch(lua_State *L)
{
    prts_engine *e = E(L);
    lua_Integer slot = luaL_checkinteger(L, 1);
    if (slot < 1 || slot > PRTS_INPUT_MAX_TOUCHES) return luaL_error(L, "touch slot must be 1..%d", PRTS_INPUT_MAX_TOUCHES);
    prts_touch_point *p = &e->dev.touches[slot - 1];
    p->id = slot;
    p->x = (float)luaL_checknumber(L, 2);
    p->y = (float)luaL_checknumber(L, 3);
    p->down = lua_toboolean(L, 4);
    if (p->down) e->dev.last_device = PRTS_SCHEME_TOUCH;
    return 0;
}

static int t_console_lines(lua_State *L)
{
    prts_engine *e = E(L);
    size_t n = e->console ? prts_console_line_count(e->console) : 0;
    lua_createtable(L, (int)n, 0);
    for (size_t i = 0; i < n; i++) {
        lua_pushstring(L, prts_console_line(e->console, i));
        lua_rawseti(L, -2, (lua_Integer)i + 1);
    }
    return 1;
}

static int t_console_open(lua_State *L)
{
    prts_engine *e = E(L);
    lua_pushboolean(L, e->console && prts_console_is_open(e->console));
    return 1;
}

static int t_observer(lua_State *L)
{
    lua_pushboolean(L, E(L)->observer.active);
    return 1;
}

/* t.camera() -> x, y, z, yaw of the view that is rendered (the observer camera if active) */
static int t_camera(lua_State *L)
{
    prts_engine *e = E(L);
    prts_camera cam = {prts_dv3(0, 1.7, 0), 0.0f, 0.0f};
    if (e->has_commander) {
        cam.eye = prts_commander_eye(&e->commander, e->world, 1.0);
        cam.yaw = e->commander.yaw;
    }
    prts_engine_camera_override(e, &cam);
    lua_pushnumber(L, cam.eye.x);
    lua_pushnumber(L, cam.eye.y);
    lua_pushnumber(L, cam.eye.z);
    lua_pushnumber(L, cam.yaw);
    return 4;
}

void prts_engine_open_test_ui(prts_engine *e, int table, bool synchronous)
{
    static const luaL_Reg fns[] = {
        {"_ui_find", t_ui_find},         {"_ui_pointer", t_ui_pointer},     {"ui_texts", t_ui_texts},
        {"_key", t_key},                 {"_type", t_type_text},            {"scheme", t_scheme},
        {"pad_button", t_pad_button},    {"pad_axis", t_pad_axis},
        {"touch", t_touch},              {"console_lines", t_console_lines}, {"console_open", t_console_open},
        {"observer", t_observer},        {"camera", t_camera},              {"_ui_frame", t_ui_frame},
        {NULL, NULL}};
    lua_State *L = prts_script_lua(e->script);
    table = lua_absindex(L, table);
    lua_pushvalue(L, table);
    luaL_setfuncs(L, fns, 0);
    lua_pop(L, 1);
    luaL_loadbufferx(L, UI_PRELUDE, strlen(UI_PRELUDE), "=ui_test_prelude", "t");
    lua_pushvalue(L, table);
    lua_pushboolean(L, synchronous);
    prts_script_call(e->script, 2, 0);
}
