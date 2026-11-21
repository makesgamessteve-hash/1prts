/* 1stPRTS engine: hot reload of Lua modules in developer builds. MIT License.
 *
 * Every module loaded with require is remembered with the file it came from and that file's
 * modification time and size. prts_script_check_reload (called by the engine about once per
 * second of real time, outside the simulation tick) re-runs the modules whose file changed, in
 * their mod's environment. A module that returns a table updates the table already in the
 * require cache in place, so other modules holding it see the new functions; handlers
 * registered by name (prts.events.on) are replaced when the module registers them again.
 * Entity data (prts.entity.data) is not touched. Afterwards main.lua's on_reload(names) runs
 * if it defines one. */
#include <lauxlib.h>
#include <lua.h>

#include "prts/prts_arena.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"
#include "script/script_internal.h"

typedef struct reload_entry {
    const char *module;
    const char *vpath;
    char *real_path;
    int64_t mtime;
    uint64_t size;
} reload_entry;

struct prts_reload {
    prts_map modules; /* module name -> reload_entry (in load order) */
    prts_arena arena; /* entries and their strings */
};

void prts_script_enable_hot_reload(prts_script *s)
{
    if (s->reload) return;
    s->reload = prts_calloc(1, sizeof(*s->reload));
    prts_map_init(&s->reload->modules);
    prts_arena_init(&s->reload->arena, 4096);
}

void prts_reload_free(prts_script *s)
{
    if (!s->reload) return;
    size_t it = 0;
    void *v;
    while (prts_map_next(&s->reload->modules, &it, NULL, &v)) prts_free(((reload_entry *)v)->real_path);
    prts_map_free(&s->reload->modules);
    prts_arena_free(&s->reload->arena);
    prts_free(s->reload);
    s->reload = NULL;
}

/* The file vpath resolves to now; NULL if no mount has it. */
static char *resolve(prts_script *s, const char *vpath)
{
    int mount = prts_vfs_find(s->vfs, vpath);
    return mount < 0 ? NULL : prts_path_join(prts_vfs_mount_dir(s->vfs, (size_t)mount), vpath);
}

void prts_reload_track(prts_script *s, const char *module, const char *vpath)
{
    reload_entry *r = prts_map_get(&s->reload->modules, module);
    if (!r) {
        r = prts_arena_alloc(&s->reload->arena, sizeof(*r));
        r->module = prts_arena_strdup(&s->reload->arena, module);
        r->vpath = prts_arena_strdup(&s->reload->arena, vpath);
        prts_map_set(&s->reload->modules, module, r);
    }
    prts_free(r->real_path);
    r->real_path = resolve(s, vpath);
    if (!r->real_path || !prts_fs_stat(r->real_path, &r->mtime, &r->size)) r->mtime = -1;
}

/* Copies the fields of the table on top of the stack into the table at dst (in place). */
static void update_in_place(lua_State *L, int dst)
{
    int src = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, src)) {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_rawset(L, dst);
    }
}

/* Re-runs one module; on success replaces its require cache entry. */
static bool reload_module(prts_script *s, reload_entry *r)
{
    lua_State *L = s->L;
    prts_log_info("hot reload: %s (%s)", r->module, r->vpath);
    if (!prts_script_load_chunk(s, r->vpath)) {
        prts_log_error("hot reload of %s failed: %s; keeping the old version", r->module, lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    lua_pushstring(L, r->module);
    if (!prts_script_call(s, 1, 1)) {
        prts_log_error("hot reload of %s failed; keeping the old version", r->module);
        return false;
    }
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushboolean(L, 1);
    }
    lua_getfield(L, LUA_REGISTRYINDEX, PRTS_REG_LOADED);
    lua_getfield(L, -1, r->module);
    if (lua_istable(L, -1) && lua_istable(L, -3) && !lua_rawequal(L, -1, -3)) {
        int old = lua_gettop(L);
        lua_pushvalue(L, -3);
        update_in_place(L, old);
        lua_pop(L, 2);
    } else {
        lua_pop(L, 1);
        lua_pushvalue(L, -2);
        lua_setfield(L, -2, r->module);
    }
    lua_pop(L, 2);
    return true;
}

static void call_on_reload(prts_script *s, prts_array *names)
{
    lua_State *L = s->L;
    int mount = prts_vfs_find(s->vfs, "scripts/main.lua");
    if (mount < 0) return;
    int top = lua_gettop(L);
    lua_getfield(L, LUA_REGISTRYINDEX, PRTS_REG_ENVS);
    if (lua_getfield(L, -1, prts_vfs_mount_id(s->vfs, (size_t)mount)) != LUA_TTABLE ||
        lua_getfield(L, -1, "on_reload") != LUA_TFUNCTION) {
        lua_settop(L, top);
        return;
    }
    lua_createtable(L, (int)names->len, 0);
    for (size_t i = 0; i < names->len; i++) {
        lua_pushstring(L, ((reload_entry *)*(void **)prts_array_at(names, i))->module);
        lua_rawseti(L, -2, (lua_Integer)i + 1);
    }
    prts_script_call(s, 1, 0);
    lua_pop(L, 2);
}

int prts_script_check_reload(prts_script *s)
{
    if (!s->reload) return 0;
    prts_array changed;
    prts_array_init(&changed, sizeof(void *));
    size_t it = 0;
    void *v;
    while (prts_map_next(&s->reload->modules, &it, NULL, &v)) {
        reload_entry *r = v;
        int64_t mtime = 0;
        uint64_t size = 0;
        if (!r->real_path || !prts_fs_stat(r->real_path, &mtime, &size)) continue; /* removed: keep the old one */
        if (mtime == r->mtime && size == r->size) continue;
        r->mtime = mtime;
        r->size = size;
        prts_array_push(&changed, &v);
    }
    int reloaded = 0;
    prts_array names;
    prts_array_init(&names, sizeof(void *));
    for (size_t i = 0; i < changed.len; i++) {
        void *entry = *(void **)prts_array_at(&changed, i);
        if (reload_module(s, entry)) {
            prts_array_push(&names, &entry);
            reloaded++;
        }
    }
    if (reloaded) call_on_reload(s, &names);
    prts_array_free(&names);
    prts_array_free(&changed);
    return reloaded;
}
