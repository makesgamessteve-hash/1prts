/* 1stPRTS engine: settings (config.lua in the user area) and input bindings with player
 * overrides (prts.settings, prts.input.rebind and friends). MIT License. */
#include <lauxlib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_log.h"

static const char *REG_SETTINGS = "prts.settings";
static const char *REG_DEFAULTS = "prts.input.defaults";
static const char *CONFIG_FILE = "config.lua";
#define MAX_DEPTH 16
#define MAX_PATH_SEGMENTS 8

/* --- plain data ------------------------------------------------------------------------ */

/* Pushes a deep copy of the value at idx if it is plain data (nil, booleans, finite numbers,
 * strings, and tables of those with string or integer keys). Otherwise pushes nothing and
 * returns false. Cycles fail through the depth limit. */
static bool copy_plain(lua_State *L, int idx, int depth)
{
    idx = lua_absindex(L, idx);
    switch (lua_type(L, idx)) {
    case LUA_TNIL:
    case LUA_TBOOLEAN:
    case LUA_TSTRING: lua_pushvalue(L, idx); return true;
    case LUA_TNUMBER:
        if (!lua_isinteger(L, idx) && !isfinite(lua_tonumber(L, idx))) return false;
        lua_pushvalue(L, idx);
        return true;
    case LUA_TTABLE: break;
    default: return false;
    }
    if (depth >= MAX_DEPTH) return false;
    lua_newtable(L);
    lua_pushnil(L);
    while (lua_next(L, idx)) {
        int kt = lua_type(L, -2);
        bool key_ok = kt == LUA_TSTRING || (kt == LUA_TNUMBER && lua_isinteger(L, -2));
        if (!key_ok || !copy_plain(L, -1, depth + 1)) {
            lua_pop(L, 3);
            return false;
        }
        lua_pushvalue(L, -3); /* key */
        lua_insert(L, -2);
        lua_rawset(L, -5);
        lua_pop(L, 1);
    }
    return true;
}

/* The settings table (created by prts_engine_load_settings before any script runs). */
static void push_settings(lua_State *L) { lua_getfield(L, LUA_REGISTRYINDEX, REG_SETTINGS); }

/* Splits "video.preset" into segments; returns the count or 0 for an invalid key. */
static int split_key(const char *key, char seg[MAX_PATH_SEGMENTS][64])
{
    int n = 0;
    const char *p = key;
    for (;;) {
        const char *dot = strchr(p, '.');
        size_t len = dot ? (size_t)(dot - p) : strlen(p);
        if (len == 0 || len >= 64 || n == MAX_PATH_SEGMENTS) return 0;
        memcpy(seg[n], p, len);
        seg[n++][len] = '\0';
        if (!dot) return n;
        p = dot + 1;
    }
}

static int l_settings_get(lua_State *L)
{
    char seg[MAX_PATH_SEGMENTS][64];
    int n = split_key(luaL_checkstring(L, 1), seg);
    if (n == 0) return luaL_error(L, "invalid settings key '%s'", lua_tostring(L, 1));
    push_settings(L);
    for (int i = 0; i < n; i++) {
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            lua_pushnil(L);
            break;
        }
        lua_getfield(L, -1, seg[i]);
        lua_remove(L, -2);
    }
    if (lua_isnil(L, -1)) {
        lua_pushvalue(L, 2); /* the default, or nil */
        return 1;
    }
    copy_plain(L, -1, 0); /* callers get a copy: changes go through prts.settings.set */
    return 1;
}

/* Sets path seg[0..n-1] of the table on top of the stack to the value at index v (nil removes). */
static void set_path(lua_State *L, char seg[MAX_PATH_SEGMENTS][64], int n, int v)
{
    for (int i = 0; i < n - 1; i++) {
        if (lua_getfield(L, -1, seg[i]) != LUA_TTABLE) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, seg[i]);
        }
        lua_remove(L, -2);
    }
    lua_pushvalue(L, v);
    lua_setfield(L, -2, seg[n - 1]);
    lua_pop(L, 1);
}

static int l_settings_set(lua_State *L)
{
    char seg[MAX_PATH_SEGMENTS][64];
    int n = split_key(luaL_checkstring(L, 1), seg);
    if (n == 0) return luaL_error(L, "invalid settings key '%s'", lua_tostring(L, 1));
    lua_settop(L, 2);
    if (!copy_plain(L, 2, 0)) return luaL_error(L, "settings hold plain data only (no functions, cycles or NaN)");
    int v = lua_gettop(L);
    push_settings(L);
    set_path(L, seg, n, v);
    return 0;
}

/* --- serialization ------------------------------------------------------------------- */

typedef struct key_entry {
    bool is_int;
    lua_Integer i;
    const char *s;
} key_entry;

static int compare_keys(const void *a, const void *b)
{
    const key_entry *x = a, *y = b;
    if (x->is_int != y->is_int) return x->is_int ? -1 : 1;
    if (x->is_int) return x->i < y->i ? -1 : x->i > y->i ? 1 : 0;
    return strcmp(x->s, y->s);
}

static void write_string(prts_str *out, const char *s, size_t len)
{
    prts_str_append(out, "\"");
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\') prts_str_appendf(out, "\\%c", c);
        else if (c == '\n') prts_str_append(out, "\\n");
        else if (c < 32 || c == 127) prts_str_appendf(out, "\\%03u", (unsigned)c);
        else prts_str_append_n(out, (const char *)&s[i], 1);
    }
    prts_str_append(out, "\"");
}

static bool is_identifier(const char *s)
{
    static const char *reserved[] = {"and", "break", "do", "else", "elseif", "end", "false", "for", "function",
                                     "goto", "if", "in", "local", "nil", "not", "or", "repeat", "return", "then",
                                     "true", "until", "while"};
    if (!*s || (*s >= '0' && *s <= '9')) return false;
    for (const char *p = s; *p; p++) {
        char c = *p;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) return false;
    }
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(reserved); i++) {
        if (strcmp(s, reserved[i]) == 0) return false;
    }
    return true;
}

static void serialize(lua_State *L, int idx, prts_str *out, int indent)
{
    idx = lua_absindex(L, idx);
    switch (lua_type(L, idx)) {
    case LUA_TBOOLEAN: prts_str_append(out, lua_toboolean(L, idx) ? "true" : "false"); return;
    case LUA_TNUMBER:
        if (lua_isinteger(L, idx)) {
            prts_str_appendf(out, "%lld", (long long)lua_tointeger(L, idx));
        } else {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.17g", (double)lua_tonumber(L, idx));
            prts_str_append(out, buf);
            if (!strpbrk(buf, ".e")) prts_str_append(out, ".0");
        }
        return;
    case LUA_TSTRING: {
        size_t len = 0;
        const char *s = lua_tolstring(L, idx, &len);
        write_string(out, s, len);
        return;
    }
    default: break;
    }
    /* a table: keys in a stable order so the file does not change between saves */
    prts_array keys;
    prts_array_init(&keys, sizeof(key_entry));
    lua_pushnil(L);
    while (lua_next(L, idx)) {
        lua_pop(L, 1);
        key_entry k = {lua_isinteger(L, -1) != 0, lua_isinteger(L, -1) ? lua_tointeger(L, -1) : 0,
                       lua_isinteger(L, -1) ? NULL : lua_tostring(L, -1)};
        prts_array_push(&keys, &k);
    }
    if (keys.len > 1) qsort(keys.data, keys.len, keys.elem, compare_keys);
    prts_str_append(out, "{\n");
    for (size_t i = 0; i < keys.len; i++) {
        key_entry *k = prts_array_at(&keys, i);
        prts_str_appendf(out, "%*s", (indent + 1) * 2, "");
        if (k->is_int) {
            prts_str_appendf(out, "[%lld] = ", (long long)k->i);
            lua_rawgeti(L, idx, k->i);
        } else {
            if (is_identifier(k->s)) {
                prts_str_appendf(out, "%s = ", k->s);
            } else {
                prts_str_append(out, "[");
                write_string(out, k->s, strlen(k->s));
                prts_str_append(out, "] = ");
            }
            lua_getfield(L, idx, k->s);
        }
        serialize(L, -1, out, indent + 1);
        lua_pop(L, 1);
        prts_str_append(out, ",\n");
    }
    prts_str_appendf(out, "%*s}", indent * 2, "");
    prts_array_free(&keys);
}

static int l_settings_save(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_str out = {0};
    prts_str_append(&out, "-- 1stPRTS settings, written by the game. Plain data only; delete this file to\n"
                          "-- restore every default.\nreturn ");
    push_settings(L);
    serialize(L, -1, &out, 0);
    lua_pop(L, 1);
    prts_str_append(&out, "\n");
    prts_vfs_result r = prts_vfs_user_write(e->vfs, CONFIG_FILE, out.data, out.len);
    prts_str_free(&out);
    if (r != PRTS_VFS_OK) {
        prts_log_error("cannot save %s: %s", CONFIG_FILE, prts_vfs_result_name(r));
        lua_pushnil(L);
        lua_pushstring(L, prts_vfs_result_name(r));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

void prts_engine_load_settings(prts_engine *e)
{
    lua_State *L = prts_script_lua(e->script);
    char *code = NULL;
    lua_newtable(L);
    if (prts_vfs_user_read(e->vfs, CONFIG_FILE, &code, NULL) == PRTS_VFS_OK) {
        if (prts_script_eval_data(e->script, code, "=config.lua")) {
            if (lua_istable(L, -1) && copy_plain(L, -1, 0)) {
                lua_replace(L, -3);
                prts_log_info("settings loaded from %s", CONFIG_FILE);
            } else {
                prts_log_warn("%s does not hold a plain data table; using the defaults", CONFIG_FILE);
            }
            lua_pop(L, 1);
        } else {
            prts_log_warn("%s could not be read; using the defaults", CONFIG_FILE);
        }
    }
    prts_free(code);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_SETTINGS);
    lua_newtable(L); /* action id -> default bindings table */
    lua_setfield(L, LUA_REGISTRYINDEX, REG_DEFAULTS);
}

/* --- bindings ------------------------------------------------------------------------ */

static const struct {
    const char *field;
    prts_binding_kind kind;
    const char *name;
} BIND_FIELDS[] = {
    {"keys", PRTS_BIND_KEY, "key"},
    {"mouse_buttons", PRTS_BIND_MOUSE_BUTTON, "mouse_button"},
    {"mouse_axes", PRTS_BIND_MOUSE_AXIS, "mouse_axis"},
    {"pad_buttons", PRTS_BIND_PAD_BUTTON, "pad_button"},
    {"pad_axes", PRTS_BIND_PAD_AXIS, "pad_axis"},
    {"touch_axes", PRTS_BIND_TOUCH_AXIS, "touch_axis"},
    {"touch_buttons", PRTS_BIND_TOUCH_BUTTON, "touch_button"},
    {"xr_buttons", PRTS_BIND_XR_BUTTON, "xr_button"},
    {"xr_axes", PRTS_BIND_XR_AXIS, "xr_axis"},
};

/* Binds every list of table t that belongs to scheme (-1: all schemes). On a bad entry,
 * fills err and returns false (earlier entries stay bound). */
static bool bind_table(lua_State *L, prts_engine *e, const char *id, int t, int scheme, char *err, size_t err_size)
{
    t = lua_absindex(L, t);
    for (size_t f = 0; f < PRTS_ARRAY_COUNT(BIND_FIELDS); f++) {
        prts_binding_kind kind = BIND_FIELDS[f].kind;
        if (scheme >= 0 && (int)prts_binding_scheme(kind) != scheme) continue;
        lua_getfield(L, t, BIND_FIELDS[f].field);
        if (lua_istable(L, -1)) {
            int list = lua_gettop(L);
            for (int i = 1; lua_rawgeti(L, list, i) != LUA_TNIL; i++) {
                bool ok;
                if (kind == PRTS_BIND_KEY) {
                    ok = lua_type(L, -1) == LUA_TSTRING && prts_input_bind(e->input, id, kind, lua_tostring(L, -1), 0, 1.0f);
                } else if (lua_istable(L, -1)) {
                    int b = lua_gettop(L);
                    lua_getfield(L, b, "code");
                    lua_getfield(L, b, "scale");
                    ok = lua_isinteger(L, -2) && prts_input_bind(e->input, id, kind, NULL, (int)lua_tointeger(L, -2),
                                                                 (float)luaL_optnumber(L, -1, 1.0));
                    lua_pop(L, 2);
                } else {
                    ok = lua_isinteger(L, -1) && prts_input_bind(e->input, id, kind, NULL, (int)lua_tointeger(L, -1), 1.0f);
                }
                if (!ok) {
                    snprintf(err, err_size, "action '%s': invalid %s binding #%d", id, BIND_FIELDS[f].field, i);
                    lua_pop(L, 2);
                    return false;
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    return true;
}

static const prts_input_scheme REBINDABLE[] = {PRTS_SCHEME_KBM, PRTS_SCHEME_GAMEPAD, PRTS_SCHEME_TOUCH, PRTS_SCHEME_XR};

/* Pushes settings.bindings[scheme][id] (or nil). */
static void push_override(lua_State *L, const char *scheme, const char *id)
{
    push_settings(L);
    const char *path[3] = {"bindings", scheme, id};
    for (int i = 0; i < 3 && lua_istable(L, -1); i++) {
        lua_getfield(L, -1, path[i]);
        lua_remove(L, -2);
    }
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_pushnil(L);
    }
}

static bool push_defaults(lua_State *L, const char *id)
{
    lua_getfield(L, LUA_REGISTRYINDEX, REG_DEFAULTS);
    if (lua_getfield(L, -1, id) != LUA_TTABLE) {
        lua_pop(L, 2);
        return false;
    }
    lua_remove(L, -2);
    return true;
}

/* Rebinds one scheme of an action: the override if there is a valid one, else the defaults. */
static void apply_scheme(lua_State *L, prts_engine *e, const char *id, prts_input_scheme scheme)
{
    char err[160];
    push_override(L, prts_input_scheme_name(scheme), id);
    if (lua_istable(L, -1)) {
        prts_input_clear_scheme(e->input, id, scheme);
        if (bind_table(L, e, id, -1, (int)scheme, err, sizeof(err))) {
            lua_pop(L, 1);
            return;
        }
        prts_log_warn("ignoring the saved %s binding: %s", prts_input_scheme_name(scheme), err);
    }
    lua_pop(L, 1);
    prts_input_clear_scheme(e->input, id, scheme);
    if (push_defaults(L, id)) {
        bind_table(L, e, id, -1, (int)scheme, err, sizeof(err));
        lua_pop(L, 1);
    }
}

void prts_engine_apply_bindings(lua_State *L, prts_engine *e, const char *id, int t)
{
    t = lua_absindex(L, t);
    char err[160];
    prts_input_clear_bindings(e->input, id);
    if (!bind_table(L, e, id, t, -1, err, sizeof(err))) luaL_error(L, "%s", err);
    lua_getfield(L, t, "always");
    prts_input_set_always(e->input, id, lua_toboolean(L, -1));
    lua_pop(L, 1);
    /* remember the defaults: overrides fall back to them, and the controls menu lists them */
    lua_getfield(L, LUA_REGISTRYINDEX, REG_DEFAULTS);
    lua_pushvalue(L, t);
    lua_setfield(L, -2, id);
    lua_pop(L, 1);
    for (size_t s = 0; s < PRTS_ARRAY_COUNT(REBINDABLE); s++) {
        push_override(L, prts_input_scheme_name(REBINDABLE[s]), id);
        bool has = lua_istable(L, -1);
        lua_pop(L, 1);
        if (has) apply_scheme(L, e, id, REBINDABLE[s]);
    }
}

static prts_input_scheme check_scheme(lua_State *L, int idx)
{
    prts_input_scheme s;
    const char *name = luaL_checkstring(L, idx);
    if (!prts_input_scheme_parse(name, &s)) luaL_error(L, "unknown input scheme '%s' (kbm, gamepad, touch or xr)", name);
    return s;
}

static const char *check_action(lua_State *L, prts_engine *e, int idx)
{
    const char *id = luaL_checkstring(L, idx);
    if (prts_input_find(e->input, id) < 0) luaL_error(L, "unknown action '%s'", id);
    return id;
}

/* prts.input.rebind(action, scheme, binding): binding is a table like the defaults
 * ({ keys = { "F" } }, { pad_buttons = { 3 } }, { touch_buttons = { 2 } }, { xr_buttons = { 8 } },
 * {} for none) or,
 * for kbm, a key name. Replaces that scheme's bindings and stores the override. */
static int l_input_rebind(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *id = check_action(L, e, 1);
    prts_input_scheme scheme = check_scheme(L, 2);
    if (lua_type(L, 3) == LUA_TSTRING && scheme == PRTS_SCHEME_KBM) {
        lua_createtable(L, 0, 1);
        lua_createtable(L, 1, 0);
        lua_pushvalue(L, 3);
        lua_rawseti(L, -2, 1);
        lua_setfield(L, -2, "keys");
        lua_replace(L, 3);
    }
    luaL_checktype(L, 3, LUA_TTABLE);
    for (size_t f = 0; f < PRTS_ARRAY_COUNT(BIND_FIELDS); f++) {
        lua_getfield(L, 3, BIND_FIELDS[f].field);
        bool other = !lua_isnil(L, -1) && prts_binding_scheme(BIND_FIELDS[f].kind) != scheme;
        lua_pop(L, 1);
        if (other) return luaL_error(L, "'%s' bindings do not belong to the %s scheme", BIND_FIELDS[f].field,
                                     prts_input_scheme_name(scheme));
    }
    if (!copy_plain(L, 3, 0)) return luaL_error(L, "a binding is plain data");
    int copy = lua_gettop(L);
    char err[160];
    prts_input_clear_scheme(e->input, id, scheme);
    if (!bind_table(L, e, id, copy, (int)scheme, err, sizeof(err))) {
        apply_scheme(L, e, id, scheme); /* back to what it was */
        return luaL_error(L, "%s", err);
    }
    push_settings(L);
    char seg[MAX_PATH_SEGMENTS][64];
    snprintf(seg[0], sizeof(seg[0]), "bindings");
    snprintf(seg[1], sizeof(seg[1]), "%s", prts_input_scheme_name(scheme));
    snprintf(seg[2], sizeof(seg[2]), "%.63s", id);
    set_path(L, seg, 3, copy);
    lua_pushboolean(L, 1);
    return 1;
}

/* prts.input.reset_bindings([scheme]): forgets overrides and rebinds the defaults. */
static int l_input_reset_bindings(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    bool all = lua_isnoneornil(L, 1);
    prts_input_scheme only = all ? PRTS_SCHEME_KBM : check_scheme(L, 1);
    push_settings(L);
    if (lua_getfield(L, -1, "bindings") == LUA_TTABLE) {
        if (all) {
            lua_pushnil(L);
            lua_setfield(L, -3, "bindings");
        } else {
            lua_pushnil(L);
            lua_setfield(L, -2, prts_input_scheme_name(only));
        }
    }
    lua_pop(L, 2);
    for (size_t a = 0; a < prts_input_action_count(e->input); a++) {
        const char *id = prts_input_action_name(e->input, (int)a);
        for (size_t s = 0; s < PRTS_ARRAY_COUNT(REBINDABLE); s++) {
            if (all || REBINDABLE[s] == only) apply_scheme(L, e, id, REBINDABLE[s]);
        }
    }
    return 0;
}

static int l_input_actions(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    lua_newtable(L);
    for (size_t a = 0; a < prts_input_action_count(e->input); a++) {
        const char *id = prts_input_action_name(e->input, (int)a);
        lua_createtable(L, 0, 2);
        lua_pushstring(L, id);
        lua_setfield(L, -2, "id");
        if (push_defaults(L, id)) {
            lua_getfield(L, -1, "label");
            lua_remove(L, -2);
        } else {
            lua_pushnil(L);
        }
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            lua_pushstring(L, id);
        }
        lua_setfield(L, -2, "label");
        lua_rawseti(L, -2, (lua_Integer)a + 1);
    }
    return 1;
}

static int l_input_bindings(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *id = check_action(L, e, 1);
    lua_createtable(L, 0, 4);
    for (size_t s = 0; s < PRTS_ARRAY_COUNT(REBINDABLE); s++) {
        lua_newtable(L);
        lua_setfield(L, -2, prts_input_scheme_name(REBINDABLE[s]));
    }
    prts_binding_kind kind;
    int code;
    float scale;
    for (size_t i = 0; prts_input_binding(e->input, id, i, &kind, &code, &scale); i++) {
        lua_getfield(L, -1, prts_input_scheme_name(prts_binding_scheme(kind)));
        lua_createtable(L, 0, 4);
        lua_pushstring(L, BIND_FIELDS[(int)kind].name);
        lua_setfield(L, -2, "kind");
        lua_pushinteger(L, code);
        lua_setfield(L, -2, "code");
        lua_pushnumber(L, scale);
        lua_setfield(L, -2, "scale");
        if (kind == PRTS_BIND_KEY) {
            lua_pushstring(L, prts_platform_key_name(code));
            lua_setfield(L, -2, "name");
        }
        lua_rawseti(L, -2, (lua_Integer)lua_rawlen(L, -2) + 1);
        lua_pop(L, 1);
    }
    return 1;
}

/* Pushes a one-entry binding table { field = { value } }. */
static void push_binding(lua_State *L, const char *field, int code, float scale, bool is_axis)
{
    lua_createtable(L, 0, 1);
    lua_createtable(L, 1, 0);
    if (is_axis) {
        lua_createtable(L, 0, 2);
        lua_pushinteger(L, code);
        lua_setfield(L, -2, "code");
        lua_pushnumber(L, scale);
        lua_setfield(L, -2, "scale");
    } else {
        lua_pushinteger(L, code);
    }
    lua_rawseti(L, -2, 1);
    lua_setfield(L, -2, field);
}

/* The first of n axes pushed past 0.6 since the last UI frame (index, or -1); *sign its direction. */
static int pushed_axis(const float *now, const float *before, int n, float *sign)
{
    for (int a = 0; a < n; a++) {
        if (fabsf(now[a]) > 0.6f && fabsf(before[a]) <= 0.6f) {
            *sign = now[a] > 0.0f ? 1.0f : -1.0f;
            return a;
        }
    }
    return -1;
}

/* prts.input.capture(scheme): the first key or button of that scheme pressed since the last
 * UI frame, as a binding table for prts.input.rebind; nil if none (controls menu). */
static int l_input_capture(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_input_scheme scheme = check_scheme(L, 1);
    const prts_input_state *now = &e->dev, *before = &e->prev_dev;
    if (scheme == PRTS_SCHEME_KBM) {
        for (int k = 1; k < PRTS_INPUT_MAX_KEYS; k++) {
            const char *name = now->keys[k] && !before->keys[k] ? prts_platform_key_name(k) : "";
            if (*name) {
                lua_createtable(L, 0, 1);
                lua_createtable(L, 1, 0);
                lua_pushstring(L, name);
                lua_rawseti(L, -2, 1);
                lua_setfield(L, -2, "keys");
                return 1;
            }
        }
        for (int b = 1; b < PRTS_INPUT_MAX_BUTTONS; b++) {
            if (now->mouse[b] && !before->mouse[b]) {
                push_binding(L, "mouse_buttons", b, 1.0f, false);
                return 1;
            }
        }
    } else if (scheme == PRTS_SCHEME_GAMEPAD) {
        for (int b = 0; b < PRTS_INPUT_MAX_PAD_BUTTONS; b++) {
            if (now->pad_buttons[b] && !before->pad_buttons[b]) {
                push_binding(L, "pad_buttons", b, 1.0f, false);
                return 1;
            }
        }
        float sign = 1.0f;
        int a = pushed_axis(now->pad_axes, before->pad_axes, PRTS_INPUT_MAX_PAD_AXES, &sign);
        if (a >= 0) {
            push_binding(L, "pad_axes", a, sign, true);
            return 1;
        }
    } else if (scheme == PRTS_SCHEME_XR) {
        for (int h = 0; h < 2; h++) {
            for (int b = 0; b < PRTS_INPUT_XR_BUTTONS; b++) {
                if (now->xr_buttons[h][b] >= 1.0f && before->xr_buttons[h][b] < 1.0f) {
                    push_binding(L, "xr_buttons", h * PRTS_INPUT_XR_BUTTONS + b, 1.0f, false);
                    return 1;
                }
            }
            float sign = 1.0f;
            int a = pushed_axis(now->xr_axes[h], before->xr_axes[h], PRTS_INPUT_XR_AXES, &sign);
            if (a >= 0) {
                push_binding(L, "xr_axes", h * PRTS_INPUT_XR_AXES + a, sign, true);
                return 1;
            }
        }
    }
    return 0;
}

static int l_input_went_down(lua_State *L)
{
    lua_pushboolean(L, prts_input_went_down(prts_engine_from_lua(L)->input, luaL_checkstring(L, 1)));
    return 1;
}

void prts_engine_open_settings_api(prts_engine *e)
{
    static const prts_script_fn settings[] = {
        {"get", l_settings_get}, {"set", l_settings_set}, {"save", l_settings_save}, {NULL, NULL}};
    static const prts_script_fn input[] = {{"rebind", l_input_rebind},     {"reset_bindings", l_input_reset_bindings},
                                           {"actions", l_input_actions},   {"bindings", l_input_bindings},
                                           {"capture", l_input_capture},   {"went_down", l_input_went_down},
                                           {NULL, NULL}};
    prts_script_register(e->script, "settings", settings);
    prts_script_register(e->script, "input", input);
}
