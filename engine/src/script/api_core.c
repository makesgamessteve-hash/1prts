/* 1stPRTS engine: core scripting API (prts.log, time, rand, vfs, sorted_pairs). MIT License. */
#include <lauxlib.h>
#include <lua.h>
#include <stdlib.h>
#include <string.h>

#include "prts/prts_log.h"
#include "script/script_internal.h"

static void log_args(lua_State *L, prts_log_level level, int first)
{
    prts_str msg = {0};
    int n = lua_gettop(L);
    for (int i = first; i <= n; i++) {
        if (i > first) prts_str_append(&msg, "\t");
        prts_str_append(&msg, luaL_tolstring(L, i, NULL));
        lua_pop(L, 1);
    }
    prts_logf(level, "%s", prts_str_cstr(&msg));
    prts_str_free(&msg);
}

static int l_log_debug(lua_State *L) { log_args(L, PRTS_LOG_DEBUG, 1); return 0; }
static int l_log_info(lua_State *L) { log_args(L, PRTS_LOG_INFO, 1); return 0; }
static int l_log_warn(lua_State *L) { log_args(L, PRTS_LOG_WARN, 1); return 0; }
static int l_log_error(lua_State *L) { log_args(L, PRTS_LOG_ERROR, 1); return 0; }

static int l_time_tick(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)prts_script_from_lua(L)->tick);
    return 1;
}

static int l_time_seconds(lua_State *L)
{
    prts_script *s = prts_script_from_lua(L);
    lua_pushnumber(L, (lua_Number)s->tick * s->tick_seconds);
    return 1;
}

static int l_time_dt(lua_State *L)
{
    lua_pushnumber(L, prts_script_from_lua(L)->tick_seconds);
    return 1;
}

/* math.random-compatible: () -> [0,1), (m) -> [1,m], (m,n) -> [m,n]. */
static int l_random(lua_State *L)
{
    prts_rng *r = &prts_script_from_lua(L)->rng;
    switch (lua_gettop(L)) {
    case 0:
        lua_pushnumber(L, prts_rng_next_double(r));
        return 1;
    case 1: {
        lua_Integer m = luaL_checkinteger(L, 1);
        luaL_argcheck(L, m >= 1, 1, "interval is empty");
        lua_pushinteger(L, (lua_Integer)prts_rng_range(r, 1, (int64_t)m));
        return 1;
    }
    default: {
        lua_Integer lo = luaL_checkinteger(L, 1), hi = luaL_checkinteger(L, 2);
        luaL_argcheck(L, lo <= hi, 2, "interval is empty");
        lua_pushinteger(L, (lua_Integer)prts_rng_range(r, (int64_t)lo, (int64_t)hi));
        return 1;
    }
    }
}

static int l_randomseed(lua_State *L)
{
    prts_script *s = prts_script_from_lua(L);
    prts_rng_seed(&s->rng, (uint64_t)luaL_checkinteger(L, 1), 0x5c121u);
    return 0;
}

/* --- sorted_pairs: numbers first (ascending), then strings (ascending) --- */

static int key_less(lua_State *L, int a, int b)
{
    int ta = lua_type(L, a), tb = lua_type(L, b);
    if (ta != tb) return ta == LUA_TNUMBER;
    if (ta == LUA_TNUMBER || ta == LUA_TSTRING) return lua_compare(L, a, b, LUA_OPLT);
    return 0;
}

static int sorted_pairs_next(lua_State *L)
{
    /* upvalue 1: sorted key array, upvalue 2: position, upvalue 3: table */
    lua_Integer pos = lua_tointeger(L, lua_upvalueindex(2)) + 1;
    lua_pushinteger(L, pos);
    lua_replace(L, lua_upvalueindex(2));
    if (lua_rawgeti(L, lua_upvalueindex(1), pos) == LUA_TNIL) return 1;
    lua_pushvalue(L, -1);
    lua_gettable(L, lua_upvalueindex(3));
    return 2;
}

static int l_sorted_pairs(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_newtable(L); /* keys */
    int keys = lua_gettop(L);
    lua_Integer n = 0;
    lua_pushnil(L);
    while (lua_next(L, 1)) {
        lua_pop(L, 1);
        int t = lua_type(L, -1);
        if (t == LUA_TNUMBER || t == LUA_TSTRING) {
            /* insertion sort: fine for the table sizes scripts iterate in order */
            lua_Integer i = n;
            while (i > 0) {
                lua_rawgeti(L, keys, i);
                int less = key_less(L, -2, -1);
                if (!less) {
                    lua_pop(L, 1);
                    break;
                }
                lua_rawseti(L, keys, i + 1);
                i--;
            }
            lua_pushvalue(L, -1);
            lua_rawseti(L, keys, i + 1);
            n++;
        }
    }
    lua_pushinteger(L, 0);
    lua_pushvalue(L, 1);
    lua_pushcclosure(L, sorted_pairs_next, 3);
    return 1;
}

/* --- vfs --------------------------------------------------------------------- */

static int l_vfs_read(lua_State *L)
{
    prts_script *s = prts_script_from_lua(L);
    const char *path = luaL_checkstring(L, 1);
    char *data = NULL;
    size_t size = 0;
    prts_vfs_result r = prts_vfs_read(s->vfs, path, &data, &size);
    if (r != PRTS_VFS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, prts_vfs_result_name(r));
        return 2;
    }
    lua_pushlstring(L, data, size);
    prts_free(data);
    return 1;
}

static int l_vfs_exists(lua_State *L)
{
    lua_pushboolean(L, prts_vfs_exists(prts_script_from_lua(L)->vfs, luaL_checkstring(L, 1)));
    return 1;
}

typedef struct list_ctx {
    lua_State *L;
    lua_Integer n;
} list_ctx;

static void list_cb(const char *name, bool is_dir, void *user)
{
    list_ctx *c = user;
    lua_createtable(c->L, 0, 2);
    lua_pushstring(c->L, name);
    lua_setfield(c->L, -2, "name");
    lua_pushboolean(c->L, is_dir);
    lua_setfield(c->L, -2, "dir");
    lua_rawseti(c->L, -2, ++c->n);
}

static int l_vfs_list(lua_State *L)
{
    prts_script *s = prts_script_from_lua(L);
    const char *dir = luaL_optstring(L, 1, "");
    lua_newtable(L);
    list_ctx c = {L, 0};
    prts_vfs_result r = prts_vfs_list(s->vfs, dir, list_cb, &c);
    if (r == PRTS_VFS_INVALID_PATH) return luaL_error(L, "invalid path '%s'", dir);
    return 1;
}

static int l_print(lua_State *L)
{
    log_args(L, PRTS_LOG_INFO, 1);
    return 0;
}

void prts_script_open_core_api(prts_script *s)
{
    lua_State *L = s->L;
    lua_getglobal(L, "prts");
    lua_pushinteger(L, PRTS_API_VERSION);
    lua_setfield(L, -2, "api_version");
    lua_pushstring(L, PRTS_ENGINE_VERSION);
    lua_setfield(L, -2, "engine_version");
#if defined(__EMSCRIPTEN__)
    lua_pushstring(L, "web");
#else
    lua_pushstring(L, "desktop");
#endif
    lua_setfield(L, -2, "platform");
    lua_pop(L, 1);

    static const prts_script_fn root[] = {{"sorted_pairs", l_sorted_pairs}, {NULL, NULL}};
    static const prts_script_fn logf[] = {
        {"debug", l_log_debug}, {"info", l_log_info}, {"warn", l_log_warn}, {"error", l_log_error}, {NULL, NULL}};
    static const prts_script_fn timef[] = {
        {"tick", l_time_tick}, {"seconds", l_time_seconds}, {"dt", l_time_dt}, {NULL, NULL}};
    static const prts_script_fn randf[] = {{"random", l_random}, {"seed", l_randomseed}, {NULL, NULL}};
    static const prts_script_fn vfsf[] = {
        {"read", l_vfs_read}, {"exists", l_vfs_exists}, {"list", l_vfs_list}, {NULL, NULL}};
    prts_script_register(s, NULL, root);
    prts_script_register(s, "log", logf);
    prts_script_register(s, "time", timef);
    prts_script_register(s, "rand", randf);
    prts_script_register(s, "vfs", vfsf);

    lua_pushcfunction(L, l_print);
    lua_setglobal(L, "print");
    lua_getglobal(L, "math");
    lua_pushcfunction(L, l_random);
    lua_setfield(L, -2, "random");
    lua_pushcfunction(L, l_randomseed);
    lua_setfield(L, -2, "randomseed");
    lua_pop(L, 1);
}
