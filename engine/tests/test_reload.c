/* Tests: hot reload of Lua modules (developer builds, desktop). MIT License. */
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"
#include "utest.h"

#define TMP PRTS_TEST_TMP

static const char *eval(prts_script *s, const char *expr)
{
    static char buf[512];
    char code[1024];
    snprintf(code, sizeof(code), "__r = tostring(%s)", expr);
    if (!prts_script_run_string(s, code, "=test")) return "<error>";
    lua_State *L = prts_script_lua(s);
    lua_getglobal(L, "__r");
    snprintf(buf, sizeof(buf), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return buf;
}

static void put(const char *path, const char *text) { prts_fs_write_file(path, text, strlen(text)); }

UTEST(reload, modules_are_rerun_in_place_and_on_reload_runs)
{
    const char *root = TMP "/reload_mod";
    prts_fs_remove_tree(root);
    put(TMP "/reload_mod/scripts/main.lua",
        "function on_reload(names) prts.events.emit('reloaded', table.concat(names, ',')) end\n"
        "require('game.rules')\n"
        "require('game.value')\n");
    put(TMP "/reload_mod/scripts/game/rules.lua", "local M = { speed = 1 } function M.get() return M.speed end return M\n");
    put(TMP "/reload_mod/scripts/game/value.lua", "return 5\n");
    prts_vfs *v = prts_vfs_create();
    prts_vfs_mount(v, root, "base");
    prts_script *s = prts_script_create(v, 1);
    ASSERT_EQ(0, prts_script_check_reload(s)); /* off */
    prts_script_enable_hot_reload(s);
    prts_script_enable_hot_reload(s);
    ASSERT_TRUE(prts_script_run_file(s, "scripts/main.lua"));
    ASSERT_TRUE(prts_script_run_string(s,
        "rules = require('game.rules') prts.events.on('reloaded', 'test', function(n) reloaded = n end)", "=t"));
    ASSERT_EQ(0, prts_script_check_reload(s)); /* nothing changed */
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);

    /* the module table is updated in place: code holding it sees the new functions */
    put(TMP "/reload_mod/scripts/game/rules.lua",
        "local M = { speed = 2 } function M.get() return M.speed * 10 end return M\n");
    ASSERT_EQ(1, prts_script_check_reload(s));
    ASSERT_STREQ("20", eval(s, "rules.get()"));
    ASSERT_STREQ("true", eval(s, "require('game.rules') == rules"));
    ASSERT_STREQ("game.rules", eval(s, "reloaded"));

    /* a module that returns a plain value replaces its cache entry */
    put(TMP "/reload_mod/scripts/game/value.lua", "return 'seven'\n");
    ASSERT_EQ(1, prts_script_check_reload(s));
    ASSERT_STREQ("seven", eval(s, "require('game.value')"));

    /* broken edits keep the old version, and are retried once fixed */
    put(TMP "/reload_mod/scripts/game/rules.lua", "local M = = 3\n");
    ASSERT_EQ(0, prts_script_check_reload(s));
    put(TMP "/reload_mod/scripts/game/rules.lua", "error('rules broke')\n");
    ASSERT_EQ(0, prts_script_check_reload(s));
    ASSERT_STREQ("20", eval(s, "rules.get()"));
    put(TMP "/reload_mod/scripts/game/value.lua", "-- returns nothing\n");
    ASSERT_EQ(1, prts_script_check_reload(s));
    ASSERT_STREQ("true", eval(s, "require('game.value')"));

    /* a removed file is left alone */
    prts_fs_remove_tree(TMP "/reload_mod/scripts/game/value.lua");
    ASSERT_EQ(0, prts_script_check_reload(s));

    /* main.lua never ran (no environment, so no on_reload): reloading still works */
    put(TMP "/reload_mod/scripts/game/rules.lua", "return { speed = 3 }\n");
    prts_script *direct = prts_script_create(v, 1);
    prts_script_enable_hot_reload(direct);
    ASSERT_STREQ("3", eval(direct, "require('game.rules').speed"));
    put(TMP "/reload_mod/scripts/game/rules.lua", "return { speed = 4, more = true }\n");
    ASSERT_EQ(1, prts_script_check_reload(direct));
    ASSERT_STREQ("4", eval(direct, "require('game.rules').speed"));
    /* a main.lua without on_reload */
    put(TMP "/reload_mod/scripts/main.lua", "x = 1\n");
    ASSERT_TRUE(prts_script_run_file(direct, "scripts/main.lua"));
    put(TMP "/reload_mod/scripts/game/rules.lua", "return { speed = 5 }\n");
    ASSERT_EQ(1, prts_script_check_reload(direct));
    /* no main.lua at all */
    prts_fs_remove_tree(TMP "/reload_mod/scripts/main.lua");
    put(TMP "/reload_mod/scripts/game/rules.lua", "return { speed = 66 }\n");
    ASSERT_EQ(1, prts_script_check_reload(direct));
    ASSERT_STREQ("66", eval(direct, "require('game.rules').speed"));
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_script_destroy(direct);
    prts_script_destroy(s);
    prts_vfs_destroy(v);
    prts_fs_remove_tree(root);
}

static prts_str g_log;

static void capture(prts_log_level level, const char *message, void *user)
{
    PRTS_UNUSED(level);
    PRTS_UNUSED(user);
    prts_str_appendf(&g_log, "%s\n", message);
}

/* Uses the door and returns how fast it swings during the next tick (radians per second). */
static double use_door_and_measure(prts_engine *e)
{
    prts_entity door = prts_world_find(e->world, "keep.door");
    prts_script_run_string(e->script, "prts.events.emit('use', prts.entity.find('keep.door'), 'keep.door')", "=use");
    float before = prts_world_local_yaw(e->world, door);
    prts_engine_frame(e, e->clock.tick_seconds);
    float after = prts_world_local_yaw(e->world, door);
    for (int i = 0; i < 60; i++) prts_engine_frame(e, e->clock.tick_seconds); /* let it finish */
    double d = (double)after - (double)before;
    return (d < 0 ? -d : d) / e->clock.tick_seconds;
}

/* Writes the base game's door script into the test mod, with its speed replaced. */
static bool write_door(const char *path, const char *speed)
{
    char *src = NULL;
    if (!prts_fs_read_file(PRTS_TEST_DATA "/base/scripts/props/door.lua", &src, NULL)) return false;
    const char *key = "local SPEED = 2.5";
    char *at = strstr(src, key);
    if (!at) {
        prts_free(src);
        return false;
    }
    prts_str out = {0};
    prts_str_append_n(&out, src, (size_t)(at - src));
    prts_str_appendf(&out, "local SPEED = %s", speed);
    prts_str_append(&out, at + strlen(key));
    bool ok = prts_fs_write_file(path, out.data, out.len);
    prts_str_free(&out);
    prts_free(src);
    return ok;
}

UTEST(reload, changing_a_door_script_while_playing)
{
    /* Spec: Changing a door script while playing */
#if !defined(PRTS_DEV)
    UTEST_SKIP("hot reload is a developer-build feature");
#endif
    const char *user = TMP "/reload_user";
    const char *door_file = TMP "/reload_user/mods/hotdoor/scripts/props/door.lua";
    prts_fs_remove_tree(user);
    put(TMP "/reload_user/mods/hotdoor/mod.lua", "return { id = \"hotdoor\", api = \"1\", depends = { \"base\" } }\n");
    ASSERT_TRUE(write_door(door_file, "2.5"));

    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = PRTS_TEST_DATA;
    cfg.user_dir = user;
    cfg.headless = true;
    cfg.hot_reload = true;
    cfg.mods[cfg.mod_count++] = "hotdoor";
    char err[1024];
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(e->hot_reload);

    /* the player stands at the door and uses it */
    prts_script_run_string(e->script,
        "local x, y, z = prts.scene.marker_pos('keep_door')\n"
        "prts.entity.data(prts.entity.find('keep.door')).visits = 3\n", "=setup");
    ASSERT_NEAR(2.5, use_door_and_measure(e), 1e-3);
    ASSERT_STREQ("true", eval(e->script, "prts.entity.data(prts.entity.find('keep.door')).open"));
    size_t entities = prts_world_count(e->world);
    prts_dvec3 cmd = prts_world_pos(e->world, e->commander.entity);

    /* the script file is edited; within a second of real time the module is reloaded */
    ASSERT_TRUE(write_door(door_file, "7.25"));
    prts_str_free(&g_log);
    prts_log_set_sink(capture, NULL);
    prts_log_set_min_level(PRTS_LOG_INFO);
    for (int i = 0; i < 31; i++) prts_engine_frame(e, e->clock.tick_seconds);
    prts_log_set_sink(NULL, NULL);
    ASSERT_TRUE(strstr(prts_str_cstr(&g_log), "hot reload: props.door (scripts/props/door.lua)") != NULL);

    /* entity data is kept, and the next use of the door uses the new speed */
    ASSERT_STREQ("3", eval(e->script, "prts.entity.data(prts.entity.find('keep.door')).visits"));
    ASSERT_STREQ("true", eval(e->script, "prts.entity.data(prts.entity.find('keep.door')).open"));
    ASSERT_NEAR(7.25, use_door_and_measure(e), 1e-3);
    ASSERT_STREQ("false", eval(e->script, "prts.entity.data(prts.entity.find('keep.door')).open"));
    /* the world is otherwise unchanged */
    ASSERT_EQ(entities, prts_world_count(e->world));
    prts_dvec3 now = prts_world_pos(e->world, e->commander.entity);
    ASSERT_EQ(cmd.x, now.x);
    ASSERT_EQ(cmd.z, now.z);
    prts_engine_destroy(e);
    prts_str_free(&g_log);
    prts_fs_remove_tree(user);
}
