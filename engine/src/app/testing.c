/* 1stPRTS engine: automated game tests (scenarios) and the Lua unit test runner. MIT License. */
#include <lauxlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"

/* Helpers written in Lua on top of the C functions below. */
static const char *SCENARIO_PRELUDE =
    "local t = ...\n"
    "t.input = {\n"
    "  hold = function(action, seconds) t._hold(action, math.max(1, math.floor(seconds * 30 + 0.5))) end,\n"
    "  press = function(action) t._hold(action, 1) end,\n"
    "}\n"
    "function t.wait_until(fn, seconds)\n"
    "  for _ = 1, math.floor(seconds * 30 + 0.5) do\n"
    "    if fn() then return true end\n"
    "    t.ticks(1)\n"
    "  end\n"
    "  error('timed out after ' .. seconds .. ' s', 2)\n"
    "end\n"
    "function t.expect(v, what)\n"
    "  local label = what and (what .. ': ') or ''\n"
    "  return {\n"
    "    is_true = function() if v ~= true then error(label .. 'expected true, got ' .. tostring(v), 2) end end,\n"
    "    equals = function(x) if v ~= x then error(label .. 'expected ' .. tostring(x) .. ', got ' .. tostring(v), 2) end end,\n"
    "    near = function(x, tol) if math.abs(v - x) > tol then\n"
    "      error(label .. 'expected ' .. x .. ' +- ' .. tol .. ', got ' .. v, 2) end end,\n"
    "  }\n"
    "end\n";

static const char *TEST_PRELUDE =
    "local T = { cases = {} }\n"
    "function T.case(name, fn) T.cases[#T.cases + 1] = { name = name, fn = fn } end\n"
    "function T.eq(a, b, what)\n"
    "  if a ~= b then error((what and what .. ': ' or '') .. 'expected ' .. tostring(b) .. ', got ' .. tostring(a), 2) end\n"
    "end\n"
    "function T.is_true(v, what) if v ~= true then error((what or 'value') .. ' is not true', 2) end end\n"
    "function T.near(a, b, tol, what)\n"
    "  if math.abs(a - b) > tol then error((what or 'value') .. ': ' .. a .. ' is not within ' .. tol .. ' of ' .. b, 2) end\n"
    "end\n"
    "function T.fails(fn, what) if pcall(fn) then error((what or 'call') .. ' did not fail', 2) end end\n"
    "return T\n";

static prts_engine *E(lua_State *L) { return prts_engine_from_lua(L); }

static int t_commander(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)E(L)->commander.entity);
    return 1;
}

static int t_ticks(lua_State *L)
{
    lua_Integer n = luaL_optinteger(L, 1, 1);
    lua_pushinteger(L, n < 1 ? 1 : n);
    return lua_yield(L, 1);
}

static int t_screenshot(lua_State *L)
{
    prts_engine *e = E(L);
    const char *name = luaL_checkstring(L, 1);
    /* names become file names in the output folder: no paths */
    for (const char *p = name; *p; p++) {
        bool ok = (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_' || *p == '-';
        if (!ok || strlen(name) >= sizeof(e->shot_name)) return luaL_error(L, "invalid screenshot name '%s'", name);
    }
    if (!*name) return luaL_error(L, "invalid screenshot name ''");
    snprintf(e->shot_name, sizeof(e->shot_name), "%s", name);
    e->want_shot = e->platform != NULL;
    lua_pushinteger(L, 1);
    return lua_yield(L, 1);
}

static int t_hold(lua_State *L)
{
    prts_engine *e = E(L);
    const char *action = luaL_checkstring(L, 1);
    if (prts_input_find(e->input, action) < 0) return luaL_error(L, "unknown action '%s'", action);
    prts_input_override(e->input, action, 1.0f, (int)luaL_checkinteger(L, 2));
    return 0;
}

static int t_place(lua_State *L)
{
    prts_engine *e = E(L);
    prts_commander *c = &e->commander;
    prts_dvec3 pos;
    if (lua_type(L, 1) == LUA_TSTRING) {
        const prts_marker *m = prts_scene_find_marker(e->scene, lua_tostring(L, 1));
        if (!m) return luaL_error(L, "unknown marker '%s'", lua_tostring(L, 1));
        pos = m->pos;
        c->yaw = m->yaw;
        c->pitch = (float)luaL_optnumber(L, 2, 0.0);
    } else {
        pos = prts_dv3(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3));
        c->yaw = (float)luaL_optnumber(L, 4, c->yaw);
        c->pitch = (float)luaL_optnumber(L, 5, 0.0);
    }
    prts_world_set_parent(e->world, c->entity, PRTS_ENTITY_NONE);
    prts_world_set_local(e->world, c->entity, pos, 0.0f);
    prts_world_begin_tick(e->world);
    return 0;
}

static int t_look(lua_State *L)
{
    prts_engine *e = E(L);
    e->commander.yaw = (float)luaL_checknumber(L, 1);
    e->commander.pitch = (float)luaL_optnumber(L, 2, 0.0);
    return 0;
}

/* Turns the commander's view towards an entity (by name, with an optional offset in the
 * entity's frame) or a point x, y, z. */
static int t_face(lua_State *L)
{
    prts_engine *e = E(L);
    prts_dvec3 target;
    if (lua_type(L, 1) == LUA_TSTRING) {
        prts_entity ent = prts_world_find(e->world, lua_tostring(L, 1));
        if (ent == PRTS_ENTITY_NONE) return luaL_error(L, "unknown entity '%s'", lua_tostring(L, 1));
        prts_vec3 off = prts_v3((float)luaL_optnumber(L, 2, 0), (float)luaL_optnumber(L, 3, 0), (float)luaL_optnumber(L, 4, 0));
        off = prts_v3_rotate_y(off, prts_world_yaw(e->world, ent));
        target = prts_dv3_add(prts_world_pos(e->world, ent), prts_dv3_from_v3(off));
    } else {
        target = prts_dv3(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3));
    }
    prts_dvec3 eye = prts_commander_eye(&e->commander, e->world, 1.0);
    double dx = target.x - eye.x, dy = target.y - eye.y, dz = target.z - eye.z;
    e->commander.yaw = (float)atan2(-dx, -dz);
    e->commander.pitch = (float)atan2(dy, sqrt(dx * dx + dz * dz));
    return 0;
}

static int t_pos(lua_State *L)
{
    prts_dvec3 p = prts_world_pos(E(L)->world, prts_engine_check_entity(L, 1));
    lua_pushnumber(L, p.x);
    lua_pushnumber(L, p.y);
    lua_pushnumber(L, p.z);
    return 3;
}

static int t_parent(lua_State *L)
{
    prts_engine *e = E(L);
    prts_entity p = prts_world_parent(e->world, prts_engine_check_entity(L, 1));
    lua_pushstring(L, p == PRTS_ENTITY_NONE ? "world" : prts_world_name(e->world, p));
    return 1;
}

static int t_in_zone(lua_State *L)
{
    prts_engine *e = E(L);
    prts_dvec3 p = prts_world_pos(e->world, prts_engine_check_entity(L, 1));
    const prts_zone *z = prts_scene_find_zone(e->scene, luaL_checkstring(L, 2));
    if (!z) return luaL_error(L, "unknown zone '%s'", lua_tostring(L, 2));
    bool in = p.x >= z->box.min.x && p.x <= z->box.max.x && p.y >= z->box.min.y && p.y <= z->box.max.y &&
              p.z >= z->box.min.z && p.z <= z->box.max.z;
    lua_pushboolean(L, in);
    return 1;
}

static int t_in_cell(lua_State *L)
{
    prts_engine *e = E(L);
    prts_dvec3 p = prts_world_pos(e->world, prts_engine_check_entity(L, 1));
    const char *want = luaL_checkstring(L, 2);
    if (prts_scene_find_cell(e->scene, want) < 0) return luaL_error(L, "unknown cell '%s'", want);
    const char *at = prts_scene_cell_name(e->scene, prts_scene_cell_at(e->scene, prts_v3((float)p.x, (float)p.y + 1.0f, (float)p.z)));
    lua_pushboolean(L, strcmp(at, want) == 0);
    return 1;
}

/* t.drawn_cells(): set of the cell names drawn in the last rendered frame (empty headless). */
static int t_drawn_cells(lua_State *L)
{
    prts_engine *e = E(L);
    prts_cell_mask mask = prts_gfx_last_visible_cells(e->gfx);
    lua_newtable(L);
    for (size_t i = 0; i < e->scene->cells.len; i++) {
        if (!(mask & PRTS_CELL_BIT(i))) continue;
        lua_pushboolean(L, 1);
        lua_setfield(L, -2, prts_scene_cell_name(e->scene, (int)i));
    }
    return 1;
}

static int t_target(lua_State *L)
{
    prts_engine *e = E(L);
    prts_entity t = prts_commander_target(&e->commander, e->world, e->cworld);
    if (t == PRTS_ENTITY_NONE) return 0;
    lua_pushstring(L, prts_world_name(e->world, t));
    return 1;
}

static int t_hash(lua_State *L)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)prts_engine_hash(E(L)));
    lua_pushstring(L, buf);
    return 1;
}

static int t_tick(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)E(L)->clock.tick);
    return 1;
}

static int t_headless(lua_State *L)
{
    lua_pushboolean(L, E(L)->platform == NULL);
    return 1;
}

static const luaL_Reg T_FUNCS[] = {
    {"commander", t_commander}, {"ticks", t_ticks},   {"screenshot", t_screenshot}, {"_hold", t_hold},
    {"place", t_place},         {"look", t_look},     {"pos", t_pos},               {"parent", t_parent},
    {"face", t_face},           {"in_cell", t_in_cell}, {"drawn_cells", t_drawn_cells},
    {"in_zone", t_in_zone},     {"target", t_target}, {"hash", t_hash},             {"tick", t_tick},
    {"headless", t_headless},   {NULL, NULL}};

static void finish(prts_engine *e, bool passed, const char *message)
{
    prts_script *s = e->script;
    lua_State *L = prts_script_lua(s);
    if (passed) prts_log_info("scenario PASSED (%llu ticks)", (unsigned long long)e->scenario_ticks);
    else prts_log_error("scenario FAILED: %s", message);
    e->exit_code = passed ? 0 : 1;
    e->scenario_active = false;
    e->quit = true;
    luaL_unref(L, LUA_REGISTRYINDEX, e->co_ref);
    e->co = NULL;
}

static void resume(prts_engine *e)
{
    lua_State *co = e->co;
    int nres = 0;
    prts_script_reset_budget(e->script);
    int nargs = lua_gettop(co) > 0 && lua_status(co) == LUA_OK ? 1 : 0;
    int rc = lua_resume(co, prts_script_lua(e->script), nargs, &nres);
    if (rc == LUA_YIELD) {
        e->wait_ticks = nres > 0 ? (int)lua_tointeger(co, -1) : 1;
        lua_pop(co, nres);
    } else if (rc == LUA_OK) {
        finish(e, true, NULL);
    } else {
        const char *msg = lua_tostring(co, -1);
        luaL_traceback(co, co, msg ? msg : "(error)", 0);
        finish(e, false, lua_tostring(co, -1));
    }
}

bool prts_engine_start_scenario(prts_engine *e, const char *real_path)
{
    lua_State *L = prts_script_lua(e->script);
    char *code = NULL;
    size_t size = 0;
    if (!prts_fs_read_file(real_path, &code, &size)) {
        prts_log_error("cannot read scenario %s", real_path);
        return false;
    }
    const char *slash = strrchr(real_path, '/');
    prts_str chunk = {0};
    prts_str_appendf(&chunk, "@%s", slash ? slash + 1 : real_path);
    int rc = luaL_loadbufferx(L, code, size, chunk.data, "t");
    prts_free(code);
    prts_str_free(&chunk);
    if (rc != LUA_OK) {
        prts_log_error("scenario error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    if (!prts_script_call(e->script, 0, 1)) return false;
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        prts_log_error("scenario %s must return a table with a run function", real_path);
        return false;
    }
    lua_getfield(L, -1, "name");
    prts_log_info("scenario: %s", lua_isstring(L, -1) ? lua_tostring(L, -1) : real_path);
    lua_pop(L, 1);
    lua_getfield(L, -1, "max_seconds");
    e->scenario_max_ticks = (uint64_t)((lua_isnumber(L, -1) ? lua_tonumber(L, -1) : 120.0) * PRTS_TICK_HZ);
    lua_pop(L, 1);
    lua_getfield(L, -1, "run");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        prts_log_error("scenario %s has no run function", real_path);
        return false;
    }
    /* the test API table, extended by the Lua prelude */
    lua_newtable(L);
    luaL_setfuncs(L, T_FUNCS, 0);
    int t = lua_gettop(L);
    luaL_loadbufferx(L, SCENARIO_PRELUDE, strlen(SCENARIO_PRELUDE), "=scenario_prelude", "t");
    lua_pushvalue(L, t);
    prts_script_call(e->script, 1, 0);
    /* coroutine running run(t) */
    e->co = lua_newthread(L);
    e->co_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushvalue(L, -2); /* run */
    lua_pushvalue(L, -2); /* t */
    lua_xmove(L, e->co, 2);
    lua_pop(L, 3);
    e->scenario_active = true;
    e->wait_ticks = 0;
    resume(e);
    return true;
}

void prts_engine_scenario_step(prts_engine *e)
{
    e->scenario_ticks++;
    if (e->scenario_ticks > e->scenario_max_ticks) {
        finish(e, false, "scenario ran longer than its max_seconds");
        return;
    }
    if (--e->wait_ticks <= 0) resume(e);
}

typedef struct test_files {
    prts_array names; /* char* */
} test_files;

static void collect_tests(const char *name, bool is_dir, void *user)
{
    test_files *f = user;
    if (is_dir || !prts_str_ends_with(name, ".lua")) return;
    char *path = prts_path_join("scripts/tests", name);
    prts_array_push(&f->names, &path);
}

int prts_engine_run_lua_tests(prts_engine *e)
{
    lua_State *L = prts_script_lua(e->script);
    luaL_loadbufferx(L, TEST_PRELUDE, strlen(TEST_PRELUDE), "=test_prelude", "t");
    prts_script_call(e->script, 0, 1);
    lua_setglobal(L, "T");
    test_files files;
    prts_array_init(&files.names, sizeof(char *));
    prts_vfs_list(e->vfs, "scripts/tests", collect_tests, &files);
    int passed = 0, failed = 0;
    for (size_t i = 0; i < files.names.len; i++) {
        char *path = *(char **)prts_array_at(&files.names, i);
        lua_getglobal(L, "T");
        lua_newtable(L);
        lua_setfield(L, -2, "cases");
        if (!prts_script_run_file(e->script, path)) {
            prts_log_error("FAIL %s (could not load)", path);
            failed++;
        }
        lua_getfield(L, -1, "cases");
        int cases = lua_gettop(L);
        for (int c = 1; lua_rawgeti(L, cases, c) == LUA_TTABLE; c++) {
            lua_getfield(L, -1, "name");
            char *name = prts_strdup(lua_isstring(L, -1) ? lua_tostring(L, -1) : "?");
            lua_pop(L, 1);
            lua_getfield(L, -1, "fn");
            if (prts_script_call(e->script, 0, 0)) {
                passed++;
                prts_log_info("ok   %s: %s", path, name);
            } else {
                failed++;
                prts_log_error("FAIL %s: %s", path, name);
            }
            prts_free(name);
            lua_pop(L, 1);
        }
        lua_pop(L, 3);
        prts_free(path);
    }
    prts_array_free(&files.names);
    prts_log_info("Lua tests: %d passed, %d failed", passed, failed);
    e->exit_code = failed == 0 && passed > 0 ? 0 : 1;
    return e->exit_code;
}
