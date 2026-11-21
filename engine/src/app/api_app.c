/* 1stPRTS engine: application scripting API: boot flags (prts.boot), quitting (prts.app),
 * mounted packs and their credits (prts.vfs.mounts, prts.vfs.read_from), third-party code
 * licenses (prts.licenses), map post-load (prts.scene.finish), the commander's interaction
 * target (prts.commander.target) and developer tools (prts.dev). MIT License. */
#include <lauxlib.h>
#include <string.h>

#include "app/engine_internal.h"

/* The libraries linked into this executable. Their full license texts ship with every
 * package (third_party/<lib>/LICENSE); the credits screen lists them from here because
 * they belong to the executable, not to a data pack. */
static const struct {
    const char *name, *version, *license, *copyright, *url;
} LICENSES[] = {
    {"Lua", "5.4.7", "MIT", "Copyright (c) 1994-2024 Lua.org, PUC-Rio", "https://www.lua.org"},
    {"SDL", "3", "zlib", "Copyright (c) 1997-2026 Sam Lantinga", "https://libsdl.org"},
    {"stb_image, stb_image_write", "", "MIT or public domain", "Copyright (c) 2017 Sean Barrett",
     "https://github.com/nothings/stb"},
    {"microui", "2.02", "MIT", "Copyright (c) 2024 rxi", "https://github.com/rxi/microui"},
};

static int l_licenses(lua_State *L)
{
    lua_createtable(L, (int)PRTS_ARRAY_COUNT(LICENSES), 0);
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(LICENSES); i++) {
        lua_createtable(L, 0, 5);
        lua_pushstring(L, LICENSES[i].name);
        lua_setfield(L, -2, "name");
        lua_pushstring(L, LICENSES[i].version);
        lua_setfield(L, -2, "version");
        lua_pushstring(L, LICENSES[i].license);
        lua_setfield(L, -2, "license");
        lua_pushstring(L, LICENSES[i].copyright);
        lua_setfield(L, -2, "copyright");
        lua_pushstring(L, LICENSES[i].url);
        lua_setfield(L, -2, "url");
        lua_rawseti(L, -2, (lua_Integer)i + 1);
    }
    return 1;
}

static int l_app_quit(lua_State *L)
{
    prts_engine_from_lua(L)->quit = true;
    return 0;
}

/* prts.vfs.mounts() -> { { id = "base" }, ... } in load order (later ones override) */
static int l_vfs_mounts(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    size_t n = prts_vfs_mount_count(e->vfs);
    lua_createtable(L, (int)n, 0);
    for (size_t i = 0; i < n; i++) {
        lua_createtable(L, 0, 1);
        lua_pushstring(L, prts_vfs_mount_id(e->vfs, i));
        lua_setfield(L, -2, "id");
        lua_rawseti(L, -2, (lua_Integer)i + 1);
    }
    return 1;
}

/* prts.vfs.read_from(mount_id, path) -> text or nil, error: a file of one pack only */
static int l_vfs_read_from(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *id = luaL_checkstring(L, 1), *path = luaL_checkstring(L, 2);
    prts_vfs_result r = PRTS_VFS_NOT_FOUND;
    char *data = NULL;
    size_t size = 0;
    for (size_t i = 0; i < prts_vfs_mount_count(e->vfs) && r == PRTS_VFS_NOT_FOUND; i++) {
        if (strcmp(prts_vfs_mount_id(e->vfs, i), id) == 0) r = prts_vfs_read_mount(e->vfs, i, path, &data, &size);
    }
    if (r != PRTS_VFS_OK) {
        lua_pushnil(L);
        lua_pushstring(L, prts_vfs_result_name(r));
        return 2;
    }
    lua_pushlstring(L, data, size);
    prts_free(data);
    return 1;
}

static int l_scene_finish(lua_State *L)
{
    prts_engine_finish_scene(prts_engine_from_lua(L));
    return 0;
}

/* prts.commander.target() -> entity, name of the usable object in view and reach, or nil */
static int l_commander_target(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    if (!e->has_commander) return 0;
    prts_entity t = prts_commander_target(&e->commander, e->world, e->cworld);
    if (t == PRTS_ENTITY_NONE) return 0;
    lua_pushinteger(L, (lua_Integer)t);
    lua_pushstring(L, prts_world_name(e->world, t));
    return 2;
}

/* --- developer tools (developer builds only) ----------------------------------------- */

/* prts.dev.observer([on]) -> active: the free observer camera, labeled as a developer view */
static int l_dev_observer(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    if (!lua_isnone(L, 1)) prts_engine_set_observer(e, lua_toboolean(L, 1));
    lua_pushboolean(L, e->observer.active);
    return 1;
}

/* prts.dev.console([open]) -> open */
static int l_dev_console(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    if (!lua_isnone(L, 1)) prts_console_set_open(e->console, lua_toboolean(L, 1));
    lua_pushboolean(L, prts_console_is_open(e->console));
    return 1;
}

/* prts.dev.run(line): runs a console command as if it was typed */
static int l_dev_run(lua_State *L)
{
    prts_console_submit(prts_engine_from_lua(L)->console, luaL_checkstring(L, 1));
    return 0;
}

static void set_bool(lua_State *L, const char *key, bool v)
{
    lua_pushboolean(L, v);
    lua_setfield(L, -2, key);
}

void prts_engine_open_app_api(prts_engine *e)
{
    lua_State *L = prts_script_lua(e->script);
    const prts_engine_config *c = &e->cfg;
    /* prts.boot: how this run was started. Batch, test and scenario runs (and --start) begin
     * a map at once; a normal start shows the front-end menu. */
    lua_getglobal(L, "prts");
    lua_createtable(L, 0, 6);
    set_bool(L, "auto_start", !c->front_end && (c->headless || c->test_lua || c->scenario || c->start_map));
    set_bool(L, "front_end", c->front_end);
    set_bool(L, "developer", e->developer);
    set_bool(L, "headless", c->headless);
    set_bool(L, "test", c->test_lua || c->scenario);
    if (c->start_map) {
        lua_pushstring(L, c->start_map);
        lua_setfield(L, -2, "map");
    }
    lua_setfield(L, -2, "boot");
    lua_pop(L, 1);

    static const prts_script_fn root[] = {{"licenses", l_licenses}, {NULL, NULL}};
    static const prts_script_fn app[] = {{"quit", l_app_quit}, {NULL, NULL}};
    static const prts_script_fn vfs[] = {{"mounts", l_vfs_mounts}, {"read_from", l_vfs_read_from}, {NULL, NULL}};
    static const prts_script_fn scene[] = {{"finish", l_scene_finish}, {NULL, NULL}};
    static const prts_script_fn commander[] = {{"target", l_commander_target}, {NULL, NULL}};
    static const prts_script_fn dev[] = {
        {"observer", l_dev_observer}, {"console", l_dev_console}, {"run", l_dev_run}, {NULL, NULL}};
    prts_script_register(e->script, NULL, root);
    prts_script_register(e->script, "app", app);
    prts_script_register(e->script, "vfs", vfs);
    prts_script_register(e->script, "scene", scene);
    prts_script_register(e->script, "commander", commander);
    /* First-Person Contract: release builds have no developer namespace at all */
    if (e->developer) prts_script_register(e->script, "dev", dev);
}
