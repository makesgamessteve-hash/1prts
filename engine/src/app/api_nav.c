/* 1stPRTS engine: prts.nav and prts.agent, navigation for units. MIT License. */
#include <lauxlib.h>
#include <math.h>

#include "app/engine_internal.h"
#include "prts/prts_nav.h"

#define MAX_POINTS 256

static prts_engine *E(lua_State *L) { return prts_engine_from_lua(L); }

static prts_nav *need_nav(lua_State *L)
{
    prts_engine *e = E(L);
    if (!e->nav) luaL_error(L, "no navigation grid (call prts.nav.build after the map is finished)");
    return e->nav;
}

/* {x, z} at field `key` of table t */
static void xz_field(lua_State *L, int t, const char *key, float *x, float *z)
{
    lua_getfield(L, t, key);
    if (!lua_istable(L, -1)) luaL_error(L, "field '%s' must be { x, z }", key);
    lua_rawgeti(L, -1, 1);
    lua_rawgeti(L, -2, 2);
    *x = (float)luaL_checknumber(L, -2);
    *z = (float)luaL_checknumber(L, -1);
    lua_pop(L, 3);
}

/* prts.nav.build{ min = {x, z}, max = {x, z}, cell = 2 } -> cols, rows */
static int l_nav_build(lua_State *L)
{
    prts_engine *e = E(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    float x0, z0, x1, z1;
    xz_field(L, 1, "min", &x0, &z0);
    xz_field(L, 1, "max", &x1, &z1);
    lua_getfield(L, 1, "cell");
    float cell = (float)luaL_optnumber(L, -1, 2.0);
    lua_pop(L, 1);
    prts_cworld_sync(e->cworld, e->world);
    prts_nav *n = prts_nav_build(e->cworld, x0, z0, x1, z1, cell);
    if (!n) return luaL_error(L, "cannot build a navigation grid of that size");
    prts_nav_destroy(e->nav);
    e->nav = n;
    lua_pushinteger(L, prts_nav_cols(n));
    lua_pushinteger(L, prts_nav_rows(n));
    return 2;
}

static void push_points(lua_State *L, const float *pts, int count)
{
    lua_createtable(L, count, 0);
    for (int i = 0; i < count; i++) {
        lua_createtable(L, 2, 0);
        lua_pushnumber(L, pts[2 * i]);
        lua_rawseti(L, -2, 1);
        lua_pushnumber(L, pts[2 * i + 1]);
        lua_rawseti(L, -2, 2);
        lua_rawseti(L, -2, i + 1);
    }
}

/* prts.nav.path(x0, z0, x1, z1) -> { {x, z}, ... } | nil */
static int l_nav_path(lua_State *L)
{
    prts_nav *n = need_nav(L);
    float pts[MAX_POINTS * 2];
    int count = prts_nav_path(n, (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2),
                              (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4), pts, MAX_POINTS);
    if (count < 0) return 0;
    push_points(L, pts, count);
    return 1;
}

static int l_nav_walkable(lua_State *L)
{
    prts_nav *n = need_nav(L);
    lua_pushboolean(L, prts_nav_walkable(n, (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2)));
    return 1;
}

static int l_nav_height(lua_State *L)
{
    prts_nav *n = need_nav(L);
    float h = 0.0f;
    if (!prts_nav_height(n, (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), &h)) return 0;
    lua_pushnumber(L, h);
    return 1;
}

static int block(lua_State *L, int delta)
{
    prts_nav *n = need_nav(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    float x0, z0, x1, z1;
    xz_field(L, 1, "min", &x0, &z0);
    xz_field(L, 1, "max", &x1, &z1);
    lua_pushinteger(L, prts_nav_block(n, x0, z0, x1, z1, delta));
    return 1;
}

static int l_nav_block(lua_State *L) { return block(L, 1); }
static int l_nav_unblock(lua_State *L) { return block(L, -1); }

static int l_nav_clear_line(lua_State *L)
{
    prts_nav *n = need_nav(L);
    lua_pushboolean(L, prts_nav_clear_line(n, (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2),
                                           (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4)));
    return 1;
}

static int l_nav_info(lua_State *L)
{
    prts_engine *e = E(L);
    if (!e->nav) return 0;
    lua_createtable(L, 0, 3);
    lua_pushinteger(L, prts_nav_cols(e->nav));
    lua_setfield(L, -2, "cols");
    lua_pushinteger(L, prts_nav_rows(e->nav));
    lua_setfield(L, -2, "rows");
    lua_pushnumber(L, prts_nav_cell(e->nav));
    lua_setfield(L, -2, "cell");
    return 1;
}

/* --- prts.agent ---------------------------------------------------------------------------- */

/* prts.agent.add(e, { speed = 1.4, radius = 0.35 }) */
static int l_agent_add(lua_State *L)
{
    prts_engine *e = E(L);
    prts_entity ent = prts_engine_check_entity(L, 1);
    float speed = 1.4f, radius = 0.35f;
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "speed");
        speed = (float)luaL_optnumber(L, -1, speed);
        lua_getfield(L, 2, "radius");
        radius = (float)luaL_optnumber(L, -1, radius);
        lua_pop(L, 2);
    }
    if (!(speed >= 0.0f && speed <= 100.0f) || !(radius > 0.0f && radius <= 50.0f)) {
        return luaL_error(L, "speed must be 0..100 and radius more than 0 and up to 50");
    }
    prts_agents_add(e->agents, ent, speed, radius);
    return 0;
}

static int l_agent_remove(lua_State *L)
{
    lua_pushboolean(L, prts_agents_remove(E(L)->agents, (prts_entity)luaL_checkinteger(L, 1)));
    return 1;
}

static prts_entity check_agent(lua_State *L, int idx)
{
    prts_entity ent = prts_engine_check_entity(L, idx);
    prts_agent_info info;
    if (!prts_agents_info(E(L)->agents, ent, &info)) luaL_error(L, "entity %d is not an agent", (int)(ent & 0xffffffffu));
    return ent;
}

/* prts.agent.move(e, x, z) -> true if a path was found */
static int l_agent_move(lua_State *L)
{
    prts_engine *e = E(L);
    prts_entity ent = check_agent(L, 1);
    float x = (float)luaL_checknumber(L, 2), z = (float)luaL_checknumber(L, 3);
    prts_dvec3 p = prts_world_pos(e->world, ent);
    float pts[MAX_POINTS * 2];
    int count;
    if (e->nav) {
        count = prts_nav_path(e->nav, (float)p.x, (float)p.z, x, z, pts, MAX_POINTS);
    } else { /* no grid: straight there */
        pts[0] = x;
        pts[1] = z;
        count = 1;
    }
    if (count < 0) {
        lua_pushboolean(L, false);
        return 1;
    }
    prts_agents_follow(e->agents, ent, pts, count);
    lua_pushboolean(L, true);
    return 1;
}

/* prts.agent.follow(e, { {x, z}, ... }) */
static int l_agent_follow(lua_State *L)
{
    prts_entity ent = check_agent(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    int count = (int)lua_rawlen(L, 2);
    if (count > MAX_POINTS) return luaL_error(L, "at most %d points", MAX_POINTS);
    float pts[MAX_POINTS * 2];
    for (int i = 0; i < count; i++) {
        lua_rawgeti(L, 2, i + 1);
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_rawgeti(L, -1, 1);
        lua_rawgeti(L, -2, 2);
        pts[2 * i] = (float)luaL_checknumber(L, -2);
        pts[2 * i + 1] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 3);
    }
    prts_agents_follow(E(L)->agents, ent, pts, count);
    return 0;
}

static int l_agent_stop(lua_State *L)
{
    prts_agents_follow(E(L)->agents, check_agent(L, 1), NULL, 0);
    return 0;
}

static int l_agent_info(lua_State *L)
{
    prts_agent_info info;
    if (!prts_agents_info(E(L)->agents, (prts_entity)luaL_checkinteger(L, 1), &info)) return 0;
    lua_createtable(L, 0, 6);
    lua_pushboolean(L, info.moving);
    lua_setfield(L, -2, "moving");
    lua_pushnumber(L, info.speed);
    lua_setfield(L, -2, "speed");
    lua_pushnumber(L, info.radius);
    lua_setfield(L, -2, "radius");
    lua_pushnumber(L, info.tx);
    lua_setfield(L, -2, "tx");
    lua_pushnumber(L, info.tz);
    lua_setfield(L, -2, "tz");
    lua_pushinteger(L, info.left);
    lua_setfield(L, -2, "left");
    return 1;
}

static int l_agent_set_speed(lua_State *L)
{
    prts_entity ent = check_agent(L, 1);
    float s = (float)luaL_checknumber(L, 2);
    if (!(s >= 0.0f && s <= 100.0f)) return luaL_error(L, "speed must be 0..100");
    prts_agents_set_speed(E(L)->agents, ent, s);
    return 0;
}

/* prts.agent.near(x, z, r [, max = 64]) -> { e, ... } nearest first */
static int l_agent_near(lua_State *L)
{
    prts_engine *e = E(L);
    float x = (float)luaL_checknumber(L, 1), z = (float)luaL_checknumber(L, 2), r = (float)luaL_checknumber(L, 3);
    int max = (int)luaL_optinteger(L, 4, 64);
    if (max < 1 || max > 4096) return luaL_error(L, "max must be 1..4096");
    prts_entity *out = prts_malloc((size_t)max * sizeof(prts_entity));
    int n = prts_agents_near(e->agents, e->world, x, z, r, out, max);
    lua_createtable(L, n, 0);
    for (int i = 0; i < n; i++) {
        lua_pushinteger(L, (lua_Integer)out[i]);
        lua_rawseti(L, -2, i + 1);
    }
    prts_free(out);
    return 1;
}

static int l_agent_count(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)prts_agents_count(E(L)->agents));
    return 1;
}

void prts_engine_open_nav_api(prts_engine *e)
{
    static const prts_script_fn nav[] = {{"build", l_nav_build},     {"path", l_nav_path},       {"walkable", l_nav_walkable},
                                         {"height", l_nav_height},   {"block", l_nav_block},     {"unblock", l_nav_unblock},
                                         {"clear_line", l_nav_clear_line}, {"info", l_nav_info}, {NULL, NULL}};
    static const prts_script_fn agent[] = {{"add", l_agent_add},           {"remove", l_agent_remove},
                                           {"move", l_agent_move},         {"follow", l_agent_follow},
                                           {"stop", l_agent_stop},         {"info", l_agent_info},
                                           {"set_speed", l_agent_set_speed}, {"near", l_agent_near},
                                           {"count", l_agent_count},       {NULL, NULL}};
    prts_script_register(e->script, "nav", nav);
    prts_script_register(e->script, "agent", agent);
}

/* After each tick's movement: the agents that reached the end of their path. */
int prts_engine_tick_agents(prts_engine *e, float dt, prts_entity *arrived, int max)
{
    return prts_agents_tick(e->agents, e->world, e->nav, dt, arrived, max);
}
