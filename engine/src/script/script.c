/* 1stPRTS engine: sandboxed Lua scripting runtime. MIT License. */
#include "prts/prts_script.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

#include "prts/prts_log.h"
#include "script/script_internal.h"

#define PRTS_HOOK_INTERVAL 1000
#define PRTS_DEFAULT_BUDGET 50000000u
#define PRTS_DATA_BUDGET 100000u

static const char *REG_SCRIPT = "prts.script";
static const char *REG_LOADED = "prts.loaded";
static const char *REG_ENVS = "prts.envs";
static const char *REG_EVENTS = "prts.events";

prts_script *prts_script_from_lua(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, REG_SCRIPT);
    prts_script *s = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return s;
}

lua_State *prts_script_lua(prts_script *s) { return s->L; }

static void hook(lua_State *L, lua_Debug *ar)
{
    prts_script *s = prts_script_from_lua(L);
    if (ar->event == LUA_HOOKLINE) {
        prts_coverage_hit(s, L, ar);
        return;
    }
    s->instructions += PRTS_HOOK_INTERVAL;
    if (s->instructions > s->budget) {
        luaL_error(L, "script budget exceeded (%I instructions)", (lua_Integer)s->budget);
    }
}

static void install_hook(prts_script *s)
{
    int mask = LUA_MASKCOUNT | (s->coverage ? LUA_MASKLINE : 0);
    lua_sethook(s->L, hook, mask, PRTS_HOOK_INTERVAL);
}

static int traceback_handler(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);
    if (!msg) msg = luaL_tolstring(L, 1, NULL);
    luaL_traceback(L, L, msg, 1);
    return 1;
}

bool prts_script_call(prts_script *s, int nargs, int nresults)
{
    lua_State *L = s->L;
    int base = lua_gettop(L) - nargs;
    lua_pushcfunction(L, traceback_handler);
    lua_insert(L, base);
    if (s->depth == 0) s->instructions = 0;
    s->depth++;
    int rc = lua_pcall(L, nargs, nresults, base);
    s->depth--;
    if (rc != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        prts_free(s->last_error);
        s->last_error = prts_strdup(err ? err : "(error object is not a string)");
        s->errors++;
        prts_log_error("script error: %s", s->last_error);
        lua_pop(L, 1);
        lua_remove(L, base);
        return false;
    }
    lua_remove(L, base);
    return true;
}

/* Pushes the environment table for a mount (created on first use). */
static void push_env(prts_script *s, const char *mount_id)
{
    lua_State *L = s->L;
    lua_getfield(L, LUA_REGISTRYINDEX, REG_ENVS);
    if (lua_getfield(L, -1, mount_id) != LUA_TTABLE) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_newtable(L);
        lua_pushglobaltable(L);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "_G");
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, mount_id);
    }
    lua_remove(L, -2);
}

/* Loads a VFS file as a function with its mod's environment; pushes it. */
static bool load_vfs_chunk(prts_script *s, const char *vpath)
{
    lua_State *L = s->L;
    char *src = NULL;
    size_t size = 0;
    prts_vfs_result r = prts_vfs_read(s->vfs, vpath, &src, &size);
    if (r != PRTS_VFS_OK) {
        lua_pushfstring(L, "cannot read '%s': %s", vpath, prts_vfs_result_name(r));
        return false;
    }
    int mount = prts_vfs_find(s->vfs, vpath);
    char *chunk = prts_malloc(strlen(vpath) + 2);
    chunk[0] = '@';
    strcpy(chunk + 1, vpath);
    int rc = luaL_loadbufferx(L, src, size, chunk, "t");
    prts_free(src);
    if (rc == LUA_OK) {
        const char *dir = prts_vfs_mount_dir(s->vfs, (size_t)mount);
        prts_coverage_register_chunk(s, L, chunk + 1, dir);
        push_env(s, prts_vfs_mount_id(s->vfs, (size_t)mount));
        lua_setupvalue(L, -2, 1);
    }
    prts_free(chunk);
    return rc == LUA_OK;
}

bool prts_script_run_file(prts_script *s, const char *vpath)
{
    if (!load_vfs_chunk(s, vpath)) {
        prts_free(s->last_error);
        s->last_error = prts_strdup(lua_tostring(s->L, -1));
        s->errors++;
        prts_log_error("script error: %s", s->last_error);
        lua_pop(s->L, 1);
        return false;
    }
    return prts_script_call(s, 0, 0);
}

bool prts_script_run_string(prts_script *s, const char *code, const char *chunk_name)
{
    if (luaL_loadbufferx(s->L, code, strlen(code), chunk_name, "t") != LUA_OK) {
        prts_free(s->last_error);
        s->last_error = prts_strdup(lua_tostring(s->L, -1));
        s->errors++;
        prts_log_error("script error: %s", s->last_error);
        lua_pop(s->L, 1);
        return false;
    }
    return prts_script_call(s, 0, 0);
}

bool prts_script_eval_data(prts_script *s, const char *code, const char *chunk_name)
{
    lua_State *L = s->L;
    if (luaL_loadbufferx(L, code, strlen(code), chunk_name, "t") != LUA_OK) {
        prts_log_error("data error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    lua_newtable(L); /* empty environment: data only */
    lua_setupvalue(L, -2, 1);
    uint64_t saved = s->budget;
    s->budget = PRTS_DATA_BUDGET;
    bool ok = prts_script_call(s, 0, 1);
    s->budget = saved;
    return ok;
}

static bool valid_module_name(const char *name)
{
    if (!*name || name[0] == '.') return false;
    for (const char *p = name; *p; p++) {
        char c = *p;
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' ||
                  (c == '.' && p[1] != '.' && p[1] != '\0');
        if (!ok) return false;
    }
    return true;
}

static int l_require(lua_State *L)
{
    prts_script *s = prts_script_from_lua(L);
    const char *name = luaL_checkstring(L, 1);
    if (!valid_module_name(name)) return luaL_error(L, "invalid module name '%s'", name);
    lua_getfield(L, LUA_REGISTRYINDEX, REG_LOADED);
    if (lua_getfield(L, -1, name) != LUA_TNIL) return 1;
    lua_pop(L, 1);
    prts_str path = {0};
    prts_str_append(&path, "scripts/");
    for (const char *p = name; *p; p++) prts_str_append_n(&path, *p == '.' ? "/" : p, 1);
    prts_str_append(&path, ".lua");
    bool ok = load_vfs_chunk(s, path.data);
    prts_str_free(&path);
    if (!ok) return lua_error(L);
    lua_pushstring(L, name);
    lua_call(L, 1, 1);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushboolean(L, 1);
    }
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, name);
    return 1;
}

static int l_load(lua_State *L)
{
    size_t len = 0;
    const char *code = luaL_checklstring(L, 1, &len);
    const char *name = luaL_optstring(L, 2, "=(load)");
    if (luaL_loadbufferx(L, code, len, name, "t") != LUA_OK) {
        lua_pushnil(L);
        lua_insert(L, -2);
        return 2;
    }
    return 1;
}

static void remove_field(lua_State *L, const char *table, const char *field)
{
    if (table) lua_getglobal(L, table);
    else lua_pushglobaltable(L);
    lua_pushnil(L);
    lua_setfield(L, -2, field);
    lua_pop(L, 1);
}

static void open_sandbox_libs(lua_State *L)
{
    static const luaL_Reg libs[] = {
        {LUA_GNAME, luaopen_base},       {LUA_TABLIBNAME, luaopen_table},
        {LUA_STRLIBNAME, luaopen_string}, {LUA_MATHLIBNAME, luaopen_math},
        {LUA_UTF8LIBNAME, luaopen_utf8}, {LUA_COLIBNAME, luaopen_coroutine},
    };
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(libs); i++) {
        luaL_requiref(L, libs[i].name, libs[i].func, 1);
        lua_pop(L, 1);
    }
    remove_field(L, NULL, "dofile");
    remove_field(L, NULL, "loadfile");
    remove_field(L, NULL, "collectgarbage");
    remove_field(L, "string", "dump");
    lua_pushcfunction(L, l_load);
    lua_setglobal(L, "load");
    lua_pushcfunction(L, l_require);
    lua_setglobal(L, "require");
}

void prts_script_register(prts_script *s, const char *ns, const prts_script_fn *fns)
{
    lua_State *L = s->L;
    lua_getglobal(L, "prts");
    if (ns) {
        if (lua_getfield(L, -1, ns) != LUA_TTABLE) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, ns);
        }
    }
    for (const prts_script_fn *f = fns; f->name; f++) {
        lua_pushcfunction(L, f->fn);
        lua_setfield(L, -2, f->name);
    }
    lua_pop(L, ns ? 2 : 1);
}

void prts_script_set_context(prts_script *s, const char *key, void *ptr)
{
    lua_pushlightuserdata(s->L, ptr);
    lua_setfield(s->L, LUA_REGISTRYINDEX, key);
}

void *prts_script_get_context(prts_script *s, const char *key)
{
    lua_getfield(s->L, LUA_REGISTRYINDEX, key);
    void *p = lua_touserdata(s->L, -1);
    lua_pop(s->L, 1);
    return p;
}

/* --- events ------------------------------------------------------------- */

static int l_events_on(lua_State *L)
{
    const char *event = luaL_checkstring(L, 1);
    const char *key = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    lua_getfield(L, LUA_REGISTRYINDEX, REG_EVENTS);
    if (lua_getfield(L, -1, event) != LUA_TTABLE) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, event);
    }
    lua_pushvalue(L, 3);
    lua_setfield(L, -2, key);
    return 0;
}

static int l_events_off(lua_State *L)
{
    const char *event = luaL_checkstring(L, 1);
    const char *key = luaL_checkstring(L, 2);
    lua_getfield(L, LUA_REGISTRYINDEX, REG_EVENTS);
    if (lua_getfield(L, -1, event) == LUA_TTABLE) {
        lua_pushnil(L);
        lua_setfield(L, -2, key);
    }
    return 0;
}

static int compare_cstr(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

int prts_script_emit(prts_script *s, const char *event, int nargs)
{
    lua_State *L = s->L;
    int args = lua_gettop(L) - nargs + 1;
    int failed = 0;
    lua_getfield(L, LUA_REGISTRYINDEX, REG_EVENTS);
    if (lua_getfield(L, -1, event) == LUA_TTABLE) {
        prts_array keys;
        prts_array_init(&keys, sizeof(char *));
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            lua_pop(L, 1);
            if (lua_type(L, -1) == LUA_TSTRING) {
                char *k = prts_strdup(lua_tostring(L, -1));
                prts_array_push(&keys, &k);
            }
        }
        if (keys.len > 1) qsort(keys.data, keys.len, keys.elem, compare_cstr);
        for (size_t i = 0; i < keys.len; i++) {
            char *k = *(char **)prts_array_at(&keys, i);
            if (lua_getfield(L, -1, k) == LUA_TFUNCTION) {
                for (int a = 0; a < nargs; a++) lua_pushvalue(L, args + a);
                if (!prts_script_call(s, nargs, 0)) failed++;
            } else {
                lua_pop(L, 1);
            }
            prts_free(k);
        }
        prts_array_free(&keys);
    }
    lua_settop(L, args - 1);
    return failed;
}

static int l_events_emit(lua_State *L)
{
    prts_script *s = prts_script_from_lua(L);
    const char *event = luaL_checkstring(L, 1);
    char *name = prts_strdup(event);
    lua_remove(L, 1);
    int failed = prts_script_emit(s, name, lua_gettop(L));
    prts_free(name);
    lua_pushinteger(L, failed);
    return 1;
}

/* --- lifecycle ---------------------------------------------------------- */

prts_script *prts_script_create(prts_vfs *vfs, uint64_t seed)
{
    prts_script *s = prts_calloc(1, sizeof(*s));
    s->vfs = vfs;
    s->budget = PRTS_DEFAULT_BUDGET;
    prts_rng_seed(&s->rng, seed, 0x5c121u);
    s->L = luaL_newstate();
    lua_State *L = s->L;
    lua_pushlightuserdata(L, s);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_SCRIPT);
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_LOADED);
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_ENVS);
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, REG_EVENTS);
    open_sandbox_libs(L);
    lua_newtable(L);
    lua_setglobal(L, "prts");
    static const prts_script_fn events[] = {
        {"on", l_events_on}, {"off", l_events_off}, {"emit", l_events_emit}, {NULL, NULL}};
    prts_script_register(s, "events", events);
    prts_script_open_core_api(s);
    install_hook(s);
    return s;
}

void prts_script_destroy(prts_script *s)
{
    if (!s) return;
    lua_close(s->L);
    prts_coverage_free(s);
    prts_free(s->last_error);
    prts_free(s);
}

void prts_script_set_budget(prts_script *s, uint64_t instructions) { s->budget = instructions; }
void prts_script_reset_budget(prts_script *s) { s->instructions = 0; }
size_t prts_script_error_count(const prts_script *s) { return s->errors; }
const char *prts_script_last_error(const prts_script *s) { return s->last_error ? s->last_error : ""; }

void prts_script_set_time(prts_script *s, uint64_t tick, double tick_seconds)
{
    s->tick = tick;
    s->tick_seconds = tick_seconds;
}

prts_rng *prts_script_rng(prts_script *s) { return &s->rng; }

void prts_script_enable_coverage(prts_script *s)
{
    s->coverage = true;
    install_hook(s);
}
