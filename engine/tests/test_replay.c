/* Unit tests: replays and determinism checks, JUnit reports, new command-line options. MIT License. */
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

#include "app/engine_internal.h"
#include "app/report.h"
#include "app/snapshot.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"
#include "utest.h"

#define FIX PRTS_TEST_FIXTURES
#define TMP PRTS_TEST_TMP

static prts_str g_log;

static void capture(prts_log_level level, const char *message, void *user)
{
    PRTS_UNUSED(level);
    PRTS_UNUSED(user);
    prts_str_appendf(&g_log, "%s\n", message);
}

static void capture_start(void)
{
    prts_str_free(&g_log);
    prts_log_set_sink(capture, NULL);
    prts_log_set_min_level(PRTS_LOG_INFO);
}

static void capture_stop(void) { prts_log_set_sink(NULL, NULL); }

static bool logged(const char *text) { return strstr(prts_str_cstr(&g_log), text) != NULL; }

/* "An unseeded random source": a test-only hook registered into the script API, whose value the
 * test changes between the recording and the playback. */
static lua_Integer g_noise;
static float g_extra; /* value of an action the recording does not know, after playback */

static int l_unseeded(lua_State *L)
{
    lua_pushinteger(L, g_noise);
    return 1;
}

static void base_config(prts_engine_config *cfg)
{
    prts_engine_config_default(cfg);
    cfg->data_dir = PRTS_TEST_DATA;
    cfg->user_dir = TMP "/replay_user";
    cfg->headless = true;
    cfg->seed = 11;
}

/* Records (record != NULL) or replays a short walk with a door use. A tick handler stores the
 * hook's value in the commander's script data at tick 40. Returns the exit code. */
static int walk(const char *record, const char *replay, const char *junit, lua_Integer noise)
{
    prts_engine_config cfg;
    base_config(&cfg);
    cfg.record = record;
    cfg.replay = replay;
    cfg.junit = junit;
    cfg.ticks = record ? 90 : 0;
    char err[1024];
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    if (!e) {
        prts_logf(PRTS_LOG_ERROR, "boot failed: %s", err);
        return -1;
    }
    static const prts_script_fn hook[] = {{"unseeded", l_unseeded}, {NULL, NULL}};
    prts_script_register(e->script, "testhook", hook);
    g_noise = noise;
    prts_script_run_string(e->script,
        "prts.events.on('tick', 'test.noise', function(tick)\n"
        "  if tick == 40 then prts.entity.data(prts.commander.entity()).noise = prts.testhook.unseeded() end\n"
        "  if tick == 20 then prts.events.emit('use', prts.entity.find('keep.door'), 'keep.door') end\n"
        "end)", "=hook");
    if (record) {
        prts_input_override(e->input, "move_forward", 1.0f, 50);
        prts_input_override(e->input, "jump", 1.0f, 1);
    } else {
        /* live input on an action the recording lacks must not reach the simulation */
        prts_input_define(e->input, "test_extra");
        prts_input_override(e->input, "test_extra", 1.0f, 1000);
        prts_input_override(e->input, "move_back", 1.0f, 1000);
    }
    int code = prts_engine_run(e);
    g_extra = prts_input_value(e->input, "test_extra");
    prts_engine_destroy(e);
    return code;
}

UTEST(replay, record_and_replay_match_every_tick)
{
    /* Spec: Record and replay, Two identical runs */
    capture_start();
    ASSERT_EQ(0, walk(TMP "/replays/walk.rpl", NULL, NULL, 1));
    ASSERT_TRUE(logged("replay recorded: 90 ticks"));
    ASSERT_EQ(0, walk(NULL, TMP "/replays/walk.rpl", TMP "/junit/replay_ok.xml", 1));
    ASSERT_TRUE(logged("replay verified: all 90 ticks match the recording"));
    ASSERT_EQ(0.0f, g_extra);
    capture_stop();
    char *xml = NULL;
    ASSERT_TRUE(prts_fs_read_file(TMP "/junit/replay_ok.xml", &xml, NULL));
    ASSERT_TRUE(strstr(xml, "failures=\"0\"") != NULL);
    prts_free(xml);
}

UTEST(replay, nondeterministic_script_is_caught_at_the_first_differing_tick)
{
    /* Spec: Non-deterministic code is caught */
    capture_start();
    ASSERT_EQ(0, walk(TMP "/replays/noise.rpl", NULL, NULL, 1));
    ASSERT_EQ(1, walk(NULL, TMP "/replays/noise.rpl", TMP "/junit/replay_bad.xml", 2));
    ASSERT_TRUE(logged("replay diverged at tick 40 (record 40 of 90)"));
    capture_stop();
    char *xml = NULL;
    ASSERT_TRUE(prts_fs_read_file(TMP "/junit/replay_bad.xml", &xml, NULL));
    ASSERT_TRUE(strstr(xml, "<failure message=\"replay diverged at tick 40") != NULL);
    prts_free(xml);
}

static bool boot_fails(const char *replay, const char *message)
{
    prts_engine_config cfg;
    base_config(&cfg);
    cfg.replay = replay;
    char err[1024];
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    prts_log_set_min_level(PRTS_LOG_INFO);
    if (e) {
        prts_engine_destroy(e);
        return false;
    }
    return strstr(err, message) != NULL;
}

/* Writes a copy of a replay with bytes changed, re-sealing the checksum unless told not to. */
static void write_variant(const char *path, const char *src, size_t at, unsigned char value, size_t cut, bool seal)
{
    char *data = NULL;
    size_t size = 0;
    prts_fs_read_file(src, &data, &size);
    if (cut) size = cut;
    if (at < size) data[at] = (char)value;
    if (seal) {
        uint64_t h = prts_hash_bytes(data, size - 8u);
        for (int i = 0; i < 8; i++) data[size - 8u + (size_t)i] = (char)(unsigned char)(h >> (8 * i));
    }
    prts_fs_write_file(path, data, size);
    prts_free(data);
}

UTEST(replay, damaged_replays_are_refused)
{
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    ASSERT_EQ(0, walk(TMP "/replays/base.rpl", NULL, NULL, 1));
    prts_log_set_min_level(PRTS_LOG_INFO);
    const char *src = TMP "/replays/base.rpl";
    const char *bad = TMP "/replays/bad.rpl";
    ASSERT_TRUE(boot_fails(TMP "/replays/none.rpl", "cannot read the replay"));
    prts_fs_write_file(bad, "1PRTSXXX0000000000000000", 24);
    ASSERT_TRUE(boot_fails(bad, "is not a 1stPRTS replay"));
    write_variant(bad, src, 8, 9, 0, true); /* format */
    ASSERT_TRUE(boot_fails(bad, "unsupported format"));
    write_variant(bad, src, 40, 0x55, 0, false); /* checksum */
    ASSERT_TRUE(boot_fails(bad, "damaged"));
    write_variant(bad, src, 15, 0x7f, 0, true); /* snapshot size beyond the file */
    ASSERT_TRUE(boot_fails(bad, "is damaged"));
    write_variant(bad, src, 16, 'X', 0, true); /* the start snapshot */
    ASSERT_TRUE(boot_fails(bad, "start state cannot be loaded: not a 1stPRTS save"));
    /* find the action list after the snapshot */
    char *data = NULL;
    size_t size = 0;
    ASSERT_TRUE(prts_fs_read_file(src, &data, &size));
    size_t snap = (size_t)(unsigned char)data[12] | (size_t)(unsigned char)data[13] << 8 |
                  (size_t)(unsigned char)data[14] << 16;
    size_t actions_at = 16 + snap;
    size_t first_name = actions_at + 4 + 4; /* count, then the first name's length */
    prts_free(data);
    write_variant(bad, src, first_name, 'Z', 0, true);
    ASSERT_TRUE(boot_fails(bad, "which this game does not define"));
    write_variant(bad, src, actions_at + 3, 0x10, 0, true); /* absurd action count */
    ASSERT_TRUE(boot_fails(bad, "is damaged"));
    write_variant(bad, src, 0, '1', size - 9, true); /* tick records cut short */
    ASSERT_TRUE(boot_fails(bad, "is damaged"));
}

UTEST(replay, recording_needs_plain_script_data)
{
    prts_engine_config cfg;
    base_config(&cfg);
    cfg.record = TMP "/replays/never.rpl";
    cfg.ticks = 3;
    char err[1024];
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    ASSERT_TRUE(e != NULL);
    /* loading saves is refused while recording */
    lua_State *L = prts_script_lua(e->script);
    ASSERT_TRUE(prts_script_run_string(e->script, "_, why = prts.save.load('x')", "=t"));
    lua_getglobal(L, "why");
    ASSERT_STREQ("saves cannot be loaded while a replay is recorded or played", lua_tostring(L, -1));
    lua_pop(L, 1);
    ASSERT_TRUE(prts_script_run_string(e->script, "prts.entity.data(prts.commander.entity()).f = print", "=t"));
    capture_start();
    ASSERT_EQ(1, prts_engine_run(e));
    ASSERT_TRUE(logged("cannot record a replay: entity 'commander' data field 'f' is a function"));
    prts_engine_destroy(e);
    capture_stop();
    /* a recording that cannot be written */
    base_config(&cfg);
    cfg.record = "/proc/1stprts-not-writable/x.rpl";
    e = prts_engine_create(&cfg, err, sizeof(err));
    ASSERT_TRUE(e != NULL);
    capture_start();
    prts_engine_destroy(e); /* nothing ticked: the start snapshot is taken now */
    capture_stop();
    ASSERT_TRUE(logged("cannot write the replay"));
}

/* --- JUnit reports -------------------------------------------------------------------- */

UTEST(report, junit_xml_escaping_and_cases)
{
    prts_report *r = prts_report_create("suite & <co>");
    prts_report_end(r, 1.0, NULL); /* nothing open: ignored */
    ASSERT_FALSE(prts_report_is_open(r));
    prts_report_case(r, "file.lua", "passes \"quoted\"", 0.5, NULL);
    prts_report_case(r, "file.lua", "fails", 0.0, "line one <here>\nline 'two'\x01");
    prts_report_begin(r, "s.lua", "first");
    prts_report_begin(r, "s.lua", "second"); /* ends the first as failed */
    ASSERT_TRUE(prts_report_is_open(r));
    prts_report_end(r, 2.0, NULL);
    ASSERT_EQ((size_t)4, prts_report_count(r));
    ASSERT_EQ((size_t)2, prts_report_failures(r));
    ASSERT_TRUE(prts_report_write_junit(r, TMP "/junit/unit.xml"));
    ASSERT_FALSE(prts_report_write_junit(r, "/proc/1stprts-not-writable/x.xml"));
    prts_report_destroy(r);
    prts_report_destroy(NULL);
    char *xml = NULL;
    ASSERT_TRUE(prts_fs_read_file(TMP "/junit/unit.xml", &xml, NULL));
    ASSERT_TRUE(strstr(xml, "<testsuite name=\"suite &amp; &lt;co&gt;\" tests=\"4\" failures=\"2\"") != NULL);
    ASSERT_TRUE(strstr(xml, "name=\"passes &quot;quoted&quot;\" time=\"0.500\"/>") != NULL);
    ASSERT_TRUE(strstr(xml, "<failure message=\"line one &lt;here&gt;\">line one &lt;here&gt;\nline &apos;two&apos;?</failure>") != NULL);
    ASSERT_TRUE(strstr(xml, "a new test case started before this one ended") != NULL);
    prts_free(xml);
}

static char *run_and_read(prts_engine_config *cfg, int want_code)
{
    char err[1024];
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine *e = prts_engine_create(cfg, err, sizeof(err));
    if (e) {
        int code = prts_engine_run(e);
        prts_engine_destroy(e);
        if (code != want_code) return NULL;
    }
    prts_log_set_min_level(PRTS_LOG_INFO);
    char *xml = NULL;
    return prts_fs_read_file(cfg->junit, &xml, NULL) ? xml : NULL;
}

UTEST(report, runners_write_junit)
{
    /* Spec: Report for CI */
    prts_engine_config cfg;
    base_config(&cfg);
    cfg.data_dir = FIX "/data_tests";
    cfg.test_lua = true;
    cfg.junit = TMP "/junit/lua.xml";
    char *xml = run_and_read(&cfg, 1);
    ASSERT_TRUE(xml != NULL);
    ASSERT_TRUE(strstr(xml, "<testsuite name=\"lua_unit_tests\"") != NULL);
    ASSERT_TRUE(strstr(xml, "name=\"(load)\"") != NULL);
    ASSERT_TRUE(strstr(xml, "<failure") != NULL);
    ASSERT_TRUE(strstr(xml, "classname=\"scripts/tests/test_mixed.lua\"") != NULL);
    prts_free(xml);

    const char *cases[][3] = {{FIX "/scenarios/helpers.lua", "0", "failures=\"0\""},
                              {FIX "/scenarios/fails.lua", "1", "<failure message=\""},
                              {FIX "/scenarios/syntax.lua", "-", "could not be started"},
                              {FIX "/scenarios/no_run.lua", "-", "could not be started"}};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(cases); i++) {
        base_config(&cfg);
        cfg.scenario = cases[i][0];
        cfg.junit = TMP "/junit/scenario.xml";
        prts_fs_remove_tree(cfg.junit);
        xml = run_and_read(&cfg, cases[i][1][0] == '1' ? 1 : 0);
        ASSERT_TRUE(xml != NULL);
        ASSERT_TRUE(strstr(xml, cases[i][2]) != NULL);
        ASSERT_TRUE(strstr(xml, "<testsuite name=\"scenarios\" tests=\"1\"") != NULL);
        prts_free(xml);
    }
    /* a run that stops before the scenario ends */
    base_config(&cfg);
    cfg.scenario = FIX "/scenarios/timeout.lua";
    cfg.ticks = 3;
    cfg.junit = TMP "/junit/unfinished.xml";
    xml = run_and_read(&cfg, 0);
    ASSERT_TRUE(xml != NULL);
    ASSERT_TRUE(strstr(xml, "the run ended before this test finished") != NULL);
    prts_free(xml);
    /* an unwritable report is logged */
    base_config(&cfg);
    cfg.scenario = FIX "/scenarios/helpers.lua";
    cfg.junit = "/proc/1stprts-not-writable/r.xml";
    char err[1024];
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ(0, prts_engine_run(e));
    capture_start();
    prts_engine_destroy(e);
    capture_stop();
    ASSERT_TRUE(logged("cannot write the JUnit report"));
    prts_str_free(&g_log);
}

UTEST(args, infrastructure_options)
{
    prts_engine_config cfg;
    char err[2048];
    char *ok[] = {"1stprts", "--junit", "j.xml", "--record", "r.rpl", "--scenario", "s.lua"};
    prts_engine_config_default(&cfg);
    ASSERT_EQ(0, prts_args_parse((int)PRTS_ARRAY_COUNT(ok), ok, &cfg, err, sizeof(err)));
    ASSERT_STREQ("j.xml", cfg.junit);
    ASSERT_STREQ("r.rpl", cfg.record);
    char *replay[] = {"1stprts", "--replay", "r.rpl"};
    prts_engine_config_default(&cfg);
    ASSERT_EQ(0, prts_args_parse(3, replay, &cfg, err, sizeof(err)));
    ASSERT_STREQ("r.rpl", cfg.replay);
    char *both[] = {"1stprts", "--replay", "r.rpl", "--scenario", "s.lua"};
    prts_engine_config_default(&cfg);
    ASSERT_EQ(1, prts_args_parse(5, both, &cfg, err, sizeof(err)));
    ASSERT_TRUE(strstr(err, "--replay cannot be combined") != NULL);
    char *missing[] = {"1stprts", "--record"};
    ASSERT_EQ(1, prts_args_parse(2, missing, &cfg, err, sizeof(err)));
    char *reload[] = {"1stprts", "--hot-reload"};
    prts_engine_config_default(&cfg);
#if defined(PRTS_DEV)
    ASSERT_EQ(0, prts_args_parse(2, reload, &cfg, err, sizeof(err)));
    ASSERT_TRUE(cfg.hot_reload);
#else
    ASSERT_EQ(1, prts_args_parse(2, reload, &cfg, err, sizeof(err)));
    ASSERT_TRUE(strstr(err, "developer builds") != NULL);
#endif
    ASSERT_TRUE(strstr(prts_args_usage(), "--replay") != NULL);
}
