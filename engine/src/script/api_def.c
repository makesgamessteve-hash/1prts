/* 1stPRTS engine: prts.def, definitions registered by kind and id and merged by id. MIT License.
 *
 * Definitions are plain data (numbers, strings, booleans and nested tables of them). Defining
 * an id that already exists deep-merges the new table into the old one, so a mod can change a
 * single field of a base definition: tables merge key by key, while arrays (sequences 1..n)
 * and all other values replace the old value. The registry keeps its own copies, and
 * prts.def.get returns a copy, so a definition only changes through prts.def.define. */
#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "script/script_internal.h"

static const char *REG_DEFS = "prts.defs";
#define DEF_MAX_DEPTH 32

/* Field path for error messages ("weapons.sword", "tags[2]"). It lives on the C stack, so
 * nothing leaks when a Lua error unwinds through the copy. Long paths are cut off. */
typedef struct def_path {
    char buf[200];
    size_t len;
} def_path;

static void path_append(def_path *p, const char *fmt, const char *text)
{
    int n = snprintf(p->buf + p->len, sizeof(p->buf) - p->len, fmt, text);
    if (n > 0) p->len = PRTS_MIN(p->len + (size_t)n, sizeof(p->buf) - 1u);
}

/* Appends the key at index key (a copy: lua_tostring converts numbers in place). */
static void path_push(lua_State *L, def_path *p, int key)
{
    if (lua_type(L, key) == LUA_TSTRING) path_append(p, p->len ? ".%s" : "%s", lua_tostring(L, key));
    else if (lua_type(L, key) == LUA_TNUMBER) path_append(p, "[%s]", lua_tostring(L, key));
    else path_append(p, "[%s]", lua_toboolean(L, key) ? "true" : "false");
}

static int def_error(lua_State *L, const char *kind, const char *id, const def_path *p, const char *what)
{
    return luaL_error(L, "prts.def.define: %s '%s': field '%s' %s; definitions must be plain data "
                         "(numbers, strings, booleans and tables)", kind, id, p->buf, what);
}

/* Pushes a deep copy of the plain value at idx, raising an error that names the field if it
 * holds anything else (functions, userdata, threads, table keys, cycles). */
static void push_plain_copy(lua_State *L, int idx, const char *kind, const char *id, def_path *path, int depth)
{
    idx = lua_absindex(L, idx);
    int t = lua_type(L, idx);
    if (t != LUA_TTABLE) {
        if (t == LUA_TNUMBER || t == LUA_TSTRING || t == LUA_TBOOLEAN) {
            lua_pushvalue(L, idx);
            return;
        }
        def_error(L, kind, id, path, lua_pushfstring(L, "is a %s", lua_typename(L, t)));
    }
    if (depth > DEF_MAX_DEPTH) def_error(L, kind, id, path, "is nested too deeply (is a table inside itself?)");
    luaL_checkstack(L, 8, "definition too deep");
    lua_newtable(L);
    int out = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, idx)) {
        size_t mark = path->len;
        int kt = lua_type(L, -2);
        if (kt != LUA_TSTRING && kt != LUA_TNUMBER && kt != LUA_TBOOLEAN) {
            def_error(L, kind, id, path, lua_pushfstring(L, "has a %s as a key", lua_typename(L, kt)));
        }
        lua_pushvalue(L, -2); /* path_push may convert numbers to strings: use a copy */
        path_push(L, path, lua_gettop(L));
        lua_pop(L, 1);
        push_plain_copy(L, -1, kind, id, path, depth + 1);
        lua_pushvalue(L, -3);
        lua_insert(L, -2);
        lua_rawset(L, out);
        lua_pop(L, 1);
        path->len = mark;
        path->buf[mark] = '\0';
    }
}

/* True for sequences 1..n with n > 0 and no other keys. */
static bool is_array(lua_State *L, int idx)
{
    lua_Unsigned n = lua_rawlen(L, idx);
    if (n == 0) return false;
    lua_Unsigned count = 0;
    lua_pushnil(L);
    while (lua_next(L, idx)) {
        lua_pop(L, 1);
        count++;
    }
    return count == n;
}

/* Merges the (already copied) table src into dst, both absolute indices. */
static void merge_into(lua_State *L, int dst, int src)
{
    luaL_checkstack(L, 8, "definition too deep");
    lua_pushnil(L);
    while (lua_next(L, src)) {
        int value = lua_gettop(L);
        lua_pushvalue(L, -2);
        lua_rawget(L, dst);
        if (lua_istable(L, -1) && lua_istable(L, value) && !is_array(L, value)) {
            merge_into(L, lua_gettop(L), value);
            lua_pop(L, 2);
        } else {
            lua_pop(L, 1);
            lua_pushvalue(L, -2);
            lua_insert(L, -2);
            lua_rawset(L, dst);
        }
    }
}

static void check_kind(lua_State *L, int idx)
{
    const char *kind = luaL_checkstring(L, idx);
    bool ok = *kind != '\0';
    for (const char *p = kind; *p; p++) {
        char c = *p;
        ok = ok && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.');
    }
    if (!ok) luaL_error(L, "invalid definition kind '%s' (use letters, digits, '_' and '.')", kind);
}

/* Pushes defs[kind] (creating it if asked); returns false (pushing nothing) otherwise. */
static bool push_kind(lua_State *L, int kind, bool create)
{
    lua_getfield(L, LUA_REGISTRYINDEX, REG_DEFS);
    lua_pushvalue(L, kind);
    if (lua_rawget(L, -2) != LUA_TTABLE) {
        lua_pop(L, 1);
        if (!create) {
            lua_pop(L, 1);
            return false;
        }
        lua_newtable(L);
        lua_pushvalue(L, kind);
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);
    }
    lua_remove(L, -2);
    return true;
}

static int l_def_define(lua_State *L)
{
    check_kind(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    const char *kind = lua_tostring(L, 1);
    lua_getfield(L, 2, "id");
    if (lua_type(L, -1) != LUA_TSTRING || lua_rawlen(L, -1) == 0) {
        return luaL_error(L, "prts.def.define: a %s definition needs a non-empty string 'id'", kind);
    }
    const char *id = lua_tostring(L, -1);
    int id_idx = lua_gettop(L);
    def_path path = {{0}, 0};
    push_plain_copy(L, 2, kind, id, &path, 0);
    int copy = lua_gettop(L);
    push_kind(L, 1, true);
    int defs = lua_gettop(L);
    lua_pushvalue(L, id_idx);
    bool is_new = lua_rawget(L, defs) != LUA_TTABLE;
    if (is_new) {
        lua_pop(L, 1);
        lua_pushvalue(L, id_idx);
        lua_pushvalue(L, copy);
        lua_rawset(L, defs);
    } else {
        merge_into(L, lua_gettop(L), copy);
    }
    lua_pushboolean(L, is_new);
    return 1;
}

static int l_def_get(lua_State *L)
{
    check_kind(L, 1);
    luaL_checkstring(L, 2);
    lua_pushnil(L);
    if (!push_kind(L, 1, false)) return 1;
    lua_pushvalue(L, 2);
    if (lua_rawget(L, -2) != LUA_TTABLE) return 1; /* nil */
    def_path path = {{0}, 0};
    push_plain_copy(L, -1, lua_tostring(L, 1), lua_tostring(L, 2), &path, 0); /* stored data is plain */
    return 1;
}

static int compare_cstr(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

/* Pushes a sorted array of the string keys of the table at idx. */
static void push_sorted_keys(lua_State *L, int idx)
{
    prts_array keys;
    prts_array_init(&keys, sizeof(const char *));
    lua_pushnil(L);
    while (lua_next(L, idx)) {
        lua_pop(L, 1);
        const char *k = lua_tostring(L, -1); /* keys are strings and stay alive in the table */
        prts_array_push(&keys, &k);
    }
    if (keys.len > 1) qsort(keys.data, keys.len, keys.elem, compare_cstr);
    lua_createtable(L, (int)keys.len, 0);
    for (size_t i = 0; i < keys.len; i++) {
        lua_pushstring(L, *(const char **)prts_array_at(&keys, i));
        lua_rawseti(L, -2, (lua_Integer)i + 1);
    }
    prts_array_free(&keys);
}

static int l_def_list(lua_State *L)
{
    check_kind(L, 1);
    if (!push_kind(L, 1, false)) {
        lua_newtable(L);
        return 1;
    }
    push_sorted_keys(L, lua_gettop(L));
    return 1;
}

static int l_def_kinds(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, REG_DEFS);
    push_sorted_keys(L, lua_gettop(L));
    return 1;
}

/* prts.def.<kind>{ ... } is shorthand for prts.def.define("<kind>", { ... }). */
static int l_def_shorthand(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_insert(L, 1);
    return l_def_define(L);
}

static int l_def_index(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING) return 0;
    check_kind(L, 2);
    lua_pushvalue(L, 2);
    lua_pushcclosure(L, l_def_shorthand, 1);
    return 1;
}

void prts_script_open_def_api(prts_script *s)
{
    lua_State *L = s->L;
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_DEFS);
    static const prts_script_fn def[] = {
        {"define", l_def_define}, {"get", l_def_get}, {"list", l_def_list}, {"kinds", l_def_kinds}, {NULL, NULL}};
    prts_script_register(s, "def", def);
    lua_getglobal(L, "prts");
    lua_getfield(L, -1, "def");
    lua_newtable(L);
    lua_pushcfunction(L, l_def_index);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);
    lua_pop(L, 2);
}
