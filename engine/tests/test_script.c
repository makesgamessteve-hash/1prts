/* Unit tests: Lua scripting runtime. MIT License. */
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

#include "prts/prts_fs.h"
#include "prts/prts_log.h"
#include "prts/prts_script.h"
#include "utest.h"

#define FIX PRTS_TEST_FIXTURES "/scripts"

static prts_script *make(prts_vfs **out_vfs, bool with_mod)
{
    prts_vfs *v = prts_vfs_create();
    prts_vfs_mount(v, FIX "/base", "base");
    if (with_mod) prts_vfs_mount(v, FIX "/mod", "mod");
    *out_vfs = v;
    return prts_script_create(v, 1234);
}

static const char *eval_str(prts_script *s, const char *expr)
{
    static char buf[512];
    char code[512];
    snprintf(code, sizeof(code), "__result = tostring(%s)", expr);
    buf[0] = '\0';
    if (!prts_script_run_string(s, code, "=test")) return "<error>";
    lua_State *L = prts_script_lua(s);
    lua_getglobal(L, "__result");
    strncpy(buf, lua_tostring(L, -1), sizeof(buf) - 1);
    lua_pop(L, 1);
    return buf;
}

UTEST(script, api_version_and_platform)
{
    prts_vfs *v;
    prts_script *s = make(&v, false);
    ASSERT_STREQ("1", eval_str(s, "prts.api_version"));
    ASSERT_STREQ("desktop", eval_str(s, "prts.platform"));
    ASSERT_STREQ(PRTS_ENGINE_VERSION, eval_str(s, "prts.engine_version"));
    ASSERT_TRUE(prts_script_from_lua(prts_script_lua(s)) == s);
    prts_script_destroy(s);
    prts_script_destroy(NULL);
    prts_vfs_destroy(v);
}

UTEST(script, sandbox_blocks_os_access)
{
    /* Spec: Blocked operating system access */
    prts_vfs *v;
    prts_script *s = make(&v, false);
    ASSERT_STREQ("nil", eval_str(s, "os"));
    ASSERT_STREQ("nil", eval_str(s, "io"));
    ASSERT_STREQ("nil", eval_str(s, "package"));
    ASSERT_STREQ("nil", eval_str(s, "debug"));
    ASSERT_STREQ("nil", eval_str(s, "dofile"));
    ASSERT_STREQ("nil", eval_str(s, "loadfile"));
    ASSERT_STREQ("nil", eval_str(s, "string.dump"));
    ASSERT_STREQ("nil", eval_str(s, "collectgarbage"));
    ASSERT_FALSE(prts_script_run_string(s, "os.execute('echo hi')", "=t"));
    ASSERT_TRUE(strstr(prts_script_last_error(s), "os") != NULL);
    /* load() accepts text but never bytecode */
    ASSERT_STREQ("3", eval_str(s, "load('return 1 + 2')()"));
    ASSERT_STREQ("nil", eval_str(s, "(load('\\27Lua'))"));
    ASSERT_STREQ("nil", eval_str(s, "(load('return = ='))"));
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(script, require_through_vfs_with_overrides)
{
    /* Spec: Module loading through the file system */
    prts_vfs *v;
    prts_script *s = make(&v, false);
    ASSERT_STREQ("base rules", eval_str(s, "require('game.rules').name"));
    ASSERT_STREQ("8", eval_str(s, "require('game.rules').double(4)"));
    ASSERT_STREQ("true", eval_str(s, "require('game.rules') == require('game.rules')"));
    ASSERT_STREQ("true", eval_str(s, "require('game.noreturn')"));
    ASSERT_STREQ("<error>", eval_str(s, "require('game.missing')"));
    ASSERT_STREQ("<error>", eval_str(s, "require('../x')"));
    ASSERT_STREQ("<error>", eval_str(s, "require('a..b')"));
    ASSERT_STREQ("<error>", eval_str(s, "require('a.')"));
    ASSERT_STREQ("<error>", eval_str(s, "require('')"));
    ASSERT_STREQ("<error>", eval_str(s, "require('game.broken')"));
    ASSERT_STREQ("<error>", eval_str(s, "require('game.fails')"));
    prts_script_destroy(s);
    prts_vfs_destroy(v);

    s = make(&v, true);
    ASSERT_STREQ("mod rules", eval_str(s, "require('game.rules').name"));
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(script, run_file_and_mod_environments)
{
    prts_vfs *v;
    prts_script *s = make(&v, false);
    ASSERT_TRUE(prts_script_run_file(s, "scripts/main.lua"));
    /* each mod's files run in their own environment */
    ASSERT_STREQ("nil", eval_str(s, "booted"));
    ASSERT_FALSE(prts_script_run_file(s, "scripts/none.lua"));
    ASSERT_FALSE(prts_script_run_file(s, "scripts/game/broken.lua"));
    ASSERT_FALSE(prts_script_run_file(s, "scripts/game/fails.lua"));
    ASSERT_TRUE(strstr(prts_script_last_error(s), "fails on purpose") != NULL);
    ASSERT_TRUE(strstr(prts_script_last_error(s), "stack traceback") != NULL);
    ASSERT_FALSE(prts_script_run_string(s, "x = = 1", "=bad"));
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(script, error_containment_in_event_handlers)
{
    /* Spec: Error in an event handler */
    prts_vfs *v;
    prts_script *s = make(&v, false);
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    ASSERT_TRUE(prts_script_run_string(s,
        "hits = {}\n"
        "prts.events.on('use', 'a_door', function(e) error('door broke') end)\n"
        "prts.events.on('use', 'b_lift', function(e) hits[#hits+1] = 'lift' .. e end)\n"
        "prts.events.on('use', 'c_bell', function(e) hits[#hits+1] = 'bell' .. e end)\n", "=t"));
    lua_State *L = prts_script_lua(s);
    size_t before = prts_script_error_count(s);
    lua_pushinteger(L, 7);
    ASSERT_EQ(1, prts_script_emit(s, "use", 1));
    ASSERT_EQ(before + 1, prts_script_error_count(s));
    ASSERT_STREQ("lift7,bell7", eval_str(s, "table.concat(hits, ',')"));
    ASSERT_STREQ("1", eval_str(s, "prts.events.emit('use', 1)"));
    ASSERT_TRUE(prts_script_run_string(s, "prts.events.off('use', 'a_door'); prts.events.off('none', 'x')", "=t"));
    ASSERT_STREQ("0", eval_str(s, "prts.events.emit('use', 2)"));
    ASSERT_STREQ("0", eval_str(s, "prts.events.emit('nobody')"));
    ASSERT_TRUE(prts_script_run_string(s, "prts.events.on('use', 'z_bad', 1) ", "=t") == false);
    ASSERT_TRUE(prts_script_run_string(s, "prts.events.on('w', 'k', function() end) prts.events.on('w', 'k2', function() end)", "=t"));
    ASSERT_EQ(0, lua_gettop(L));
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(script, budget_interrupts_endless_loop)
{
    /* Spec: Endless loop */
    prts_vfs *v;
    prts_script *s = make(&v, false);
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_script_set_budget(s, 100000);
    ASSERT_FALSE(prts_script_run_string(s, "while true do end", "=loop"));
    ASSERT_TRUE(strstr(prts_script_last_error(s), "budget exceeded") != NULL);
    ASSERT_TRUE(prts_script_run_string(s, "local x = 0 for i = 1, 1000 do x = x + i end", "=ok"));
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(script, deterministic_random_and_time)
{
    /* Spec: Same seed, same random numbers */
    prts_vfs *v;
    prts_script *a = make(&v, false);
    prts_script *b = prts_script_create(v, 1234);
    char seq_a[256], seq_b[256];
    strcpy(seq_a, eval_str(a, "math.random(1, 1000) .. ',' .. math.random(6) .. ',' .. math.random()"));
    strcpy(seq_b, eval_str(b, "math.random(1, 1000) .. ',' .. math.random(6) .. ',' .. math.random()"));
    ASSERT_STREQ(seq_a, seq_b);
    ASSERT_STREQ("true", eval_str(a, "prts.rand.random == math.random"));
    ASSERT_STREQ("<error>", eval_str(a, "math.random(0)"));
    ASSERT_STREQ("<error>", eval_str(a, "math.random(5, 1)"));
    ASSERT_TRUE(prts_script_run_string(a, "math.randomseed(9) x1 = math.random(100) math.randomseed(9) x2 = math.random(100)", "=t"));
    ASSERT_STREQ("true", eval_str(a, "x1 == x2"));
    prts_script_set_time(a, 90, 1.0 / 30.0);
    ASSERT_STREQ("90", eval_str(a, "prts.time.tick()"));
    ASSERT_STREQ("3.0", eval_str(a, "prts.time.seconds()"));
    ASSERT_STREQ("true", eval_str(a, "math.abs(prts.time.dt() - 1/30) < 1e-12"));
    ASSERT_TRUE(prts_script_rng(a) != NULL);
    prts_script_destroy(a);
    prts_script_destroy(b);
    prts_vfs_destroy(v);
}

UTEST(script, sorted_pairs_and_logging)
{
    prts_vfs *v;
    prts_script *s = make(&v, false);
    ASSERT_STREQ("1,2,10,a,b,z,",
                 eval_str(s, "(function() local o = '' for k in prts.sorted_pairs({z=1, a=2, [10]=3, [1]=4, b=5, [2]=6, [true]=7}) do o = o .. tostring(k) .. ',' end return o end)()"));
    ASSERT_STREQ("4", eval_str(s, "(function() for k, v in prts.sorted_pairs({[1]=4}) do return v end end)()"));
    ASSERT_TRUE(prts_script_run_string(s, "prts.log.debug('d', 1) prts.log.info('i') prts.log.warn('w') print('p', nil, true)", "=t"));
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    ASSERT_TRUE(prts_script_run_string(s, "prts.log.error('e')", "=t"));
    ASSERT_FALSE(prts_script_run_string(s, "error({})", "=t"));
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(script, vfs_api)
{
    prts_vfs *v;
    prts_script *s = make(&v, true);
    ASSERT_STREQ("true", eval_str(s, "prts.vfs.exists('scripts/main.lua')"));
    ASSERT_STREQ("false", eval_str(s, "prts.vfs.exists('../main.lua')"));
    ASSERT_STREQ("booted = \"base\"\n", eval_str(s, "prts.vfs.read('scripts/main.lua')"));
    ASSERT_STREQ("not found", eval_str(s, "select(2, prts.vfs.read('scripts/none.lua'))"));
    ASSERT_STREQ("cov.lua,game,main.lua,", eval_str(s, "(function() local o = '' for _, e in ipairs(prts.vfs.list('scripts')) do o = o .. e.name .. ',' end return o end)()"));
    ASSERT_STREQ("true", eval_str(s, "prts.vfs.list('scripts')[2].dir"));
    ASSERT_STREQ("0", eval_str(s, "#prts.vfs.list('nothing')"));
    ASSERT_STREQ("<error>", eval_str(s, "prts.vfs.list('../')"));
    ASSERT_STREQ("true", eval_str(s, "#prts.vfs.list() >= 1"));
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(script, data_only_eval_and_context)
{
    prts_vfs *v;
    prts_script *s = make(&v, false);
    lua_State *L = prts_script_lua(s);
    ASSERT_TRUE(prts_script_eval_data(s, "return { id = 'base', api = '1' }", "=mod.lua"));
    lua_getfield(L, -1, "id");
    ASSERT_STREQ("base", lua_tostring(L, -1));
    lua_pop(L, 2);
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    ASSERT_FALSE(prts_script_eval_data(s, "return { x = prts.api_version }", "=mod.lua"));
    ASSERT_FALSE(prts_script_eval_data(s, "return {", "=mod.lua"));
    ASSERT_FALSE(prts_script_eval_data(s, "while true do end", "=mod.lua"));
    prts_log_set_min_level(PRTS_LOG_INFO);
    int marker = 5;
    prts_script_set_context(s, "test.ctx", &marker);
    ASSERT_TRUE(prts_script_get_context(s, "test.ctx") == &marker);
    ASSERT_TRUE(prts_script_get_context(s, "test.none") == NULL);
    ASSERT_EQ(0, lua_gettop(L));
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(script, lua_line_coverage)
{
    prts_vfs *v;
    prts_script *s = make(&v, true);
    prts_script_enable_coverage(s);
    ASSERT_TRUE(prts_script_run_file(s, "scripts/cov.lua"));
    ASSERT_TRUE(prts_script_run_file(s, "scripts/cov.lua"));
    ASSERT_TRUE(prts_script_run_string(s, "local a = 1", "=inline"));
    const char *out = PRTS_TEST_TMP "/cov/lua.info";
    ASSERT_TRUE(prts_script_write_coverage(s, out));
    char *data = NULL;
    ASSERT_TRUE(prts_fs_read_file(out, &data, NULL));
    ASSERT_TRUE(strstr(data, "SF:") != NULL);
    ASSERT_TRUE(strstr(data, "cov.lua") != NULL);
    ASSERT_TRUE(strstr(data, "DA:3,2") != NULL); /* 'return 1' ran twice */
    ASSERT_TRUE(strstr(data, "DA:5,0") != NULL); /* 'return 2' never ran */
    ASSERT_TRUE(strstr(data, "LH:") != NULL);
    prts_free(data);
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}
