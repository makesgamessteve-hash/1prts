/* Unit tests: plain data serialization, simulation snapshots (saves) and prts.save. MIT License. */
#include <SDL3/SDL.h>
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

#include "app/engine_internal.h"
#include "app/snapshot.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"
#include "prts/prts_serial.h"
#include "utest.h"

#define TMP PRTS_TEST_TMP

static const char *eval(prts_script *s, const char *expr)
{
    static char buf[1024];
    char code[2048];
    snprintf(code, sizeof(code), "__r = tostring(%s)", expr);
    if (!prts_script_run_string(s, code, "=test")) return "<error>";
    lua_State *L = prts_script_lua(s);
    lua_getglobal(L, "__r");
    snprintf(buf, sizeof(buf), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return buf;
}

/* Serializes global `name` and returns the bytes (caller frees the writer). */
static bool write_global(prts_script *s, const char *name, prts_writer *w, bool strict, char *err)
{
    lua_State *L = prts_script_lua(s);
    lua_getglobal(L, name);
    bool ok = prts_script_write_plain(s, -1, w, strict, "test data", err, 512);
    lua_pop(L, 1);
    return ok;
}

UTEST(plain, round_trip_order_and_sharing)
{
    prts_vfs *v = prts_vfs_create();
    prts_script *s = prts_script_create(v, 1);
    lua_State *L = prts_script_lua(s);
    ASSERT_TRUE(prts_script_run_string(s,
        "shared = { 'x' }\n"
        "a = { n = 1, f = 0.5, big = math.maxinteger, s = 'keep\\0hall', yes = true, no = false,\n"
        "      [1] = 'one', [2.5] = 'half', [-3] = 'neg', [true] = 't', [false] = 'f',\n"
        "      nested = { deep = { deeper = { 42 } } }, s1 = shared, s2 = shared }\n"
        "a.self = a\n"
        /* the same content built in another order gives the same bytes */
        "b = {} for _, k in ipairs({ 'z', 'y', 'x', 'w' }) do b[k] = #k end b[3] = 1 b[1] = 1 b[2] = 1\n"
        "c = { w = 1, x = 1, y = 1, z = 1, 1, 1, 1 }\n", "=data"));
    prts_writer w = {0};
    char err[512];
    ASSERT_TRUE(write_global(s, "a", &w, true, err));
    prts_reader r;
    prts_reader_init(&r, w.data, w.len);
    ASSERT_TRUE(prts_script_read_plain(s, &r));
    lua_setglobal(L, "a2");
    ASSERT_EQ(w.len, r.pos);
    ASSERT_STREQ("1", eval(s, "a2.n"));
    ASSERT_STREQ("true", eval(s, "math.type(a2.n) == 'integer' and math.type(a2.f) == 'float'"));
    ASSERT_STREQ("true", eval(s, "a2.big == math.maxinteger"));
    ASSERT_STREQ("9", eval(s, "#a2.s"));
    ASSERT_STREQ("true", eval(s, "a2.yes == true and a2.no == false and a2[true] == 't' and a2[false] == 'f'"));
    ASSERT_STREQ("one half neg", eval(s, "a2[1] .. ' ' .. a2[2.5] .. ' ' .. a2[-3]"));
    ASSERT_STREQ("42", eval(s, "a2.nested.deep.deeper[1]"));
    ASSERT_STREQ("true", eval(s, "a2.self == a2 and a2.s1 == a2.s2 and a2.s1 ~= shared"));
    prts_writer wb = {0}, wc = {0};
    ASSERT_TRUE(write_global(s, "b", &wb, true, err));
    ASSERT_TRUE(write_global(s, "c", &wc, false, err));
    ASSERT_EQ(wb.len, wc.len);
    ASSERT_EQ(0, memcmp(wb.data, wc.data, wb.len));
    prts_writer_free(&wb);
    prts_writer_free(&wc);
    /* a nil value round-trips too */
    lua_pushnil(L);
    w.len = 0;
    ASSERT_TRUE(prts_script_write_plain(s, -1, &w, true, NULL, err, sizeof(err)));
    lua_pop(L, 1);
    prts_reader_init(&r, w.data, w.len);
    ASSERT_TRUE(prts_script_read_plain(s, &r));
    ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
    prts_writer_free(&w);
    ASSERT_EQ(0, lua_gettop(L));
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(plain, functions_userdata_and_threads_are_named)
{
    prts_vfs *v = prts_vfs_create();
    prts_script *s = prts_script_create(v, 1);
    lua_State *L = prts_script_lua(s);
    char err[512];
    prts_writer w = {0};
    struct {
        const char *code;
        const char *message;
    } cases[] = {
        {"x = { door = { on_open = function() end } }", "test data field 'door.on_open' is a function"},
        {"x = { co = coroutine.create(print) }", "test data field 'co' is a thread"},
        {"x = { list = { 1, 2, print } }", "test data field 'list[3]' is a function"},
        {"x = { [2.5] = { [true] = print } }", "test data field '[2.5][true]' is a function"},
        {"x = { [{}] = 1 }", "test data has a table as a key"},
        {"x = print", "test data is a function"},
    };
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(cases); i++) {
        ASSERT_TRUE(prts_script_run_string(s, cases[i].code, "=t"));
        w.len = 0;
        err[0] = '\0';
        ASSERT_FALSE(write_global(s, "x", &w, true, err));
        ASSERT_TRUE(strstr(err, cases[i].message) != NULL);
        ASSERT_TRUE(strstr(err, "must be plain") != NULL);
        w.len = 0;
        ASSERT_TRUE(write_global(s, "x", &w, false, err)); /* hashing never fails */
    }
    /* userdata (only C can make it) */
    lua_newtable(L);
    lua_pushlightuserdata(L, &w);
    lua_setfield(L, -2, "handle");
    w.len = 0;
    ASSERT_FALSE(prts_script_write_plain(s, -1, &w, true, "entity 'x' data", err, sizeof(err)));
    ASSERT_TRUE(strstr(err, "entity 'x' data field 'handle' is a userdata") != NULL);
    lua_pop(L, 1);
    /* very deep nesting */
    ASSERT_TRUE(prts_script_run_string(s, "x = {} local t = x for i = 1, 300 do t.n = {} t = t.n end", "=t"));
    w.len = 0;
    ASSERT_FALSE(write_global(s, "x", &w, true, err));
    ASSERT_TRUE(strstr(err, "nested too deeply") != NULL);
    prts_writer_free(&w);
    ASSERT_EQ(0, lua_gettop(L));
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(plain, malformed_data_is_rejected)
{
    prts_vfs *v = prts_vfs_create();
    prts_script *s = prts_script_create(v, 1);
    lua_State *L = prts_script_lua(s);
    static const unsigned char bad[][12] = {
        {9},                                  /* unknown tag */
        {8, 6},                               /* hash-only stand-in */
        {3, 1, 2},                            /* truncated integer */
        {5, 9, 0, 0, 0, 'a'},                 /* string longer than the data */
        {6, 1, 0, 0, 0, 0, 2},                /* nil key */
        {6, 1, 0, 0, 0, 6, 0, 0, 0, 0, 2},    /* table as a key */
        {6, 1, 0, 0, 0, 2, 0},                /* nil value */
        {6, 1, 0, 0, 0, 2, 7, 5, 0, 0, 0},    /* reference to an unknown table */
        {6, 1, 0, 0, 0, 7, 1, 0, 0, 0, 2},    /* reference as a key */
        {6, 200, 0, 0, 0, 2, 2},              /* more pairs than bytes */
        {4, 0, 0, 0, 0, 0, 0, 0xf8, 0x7f},    /* NaN is fine as a value */
    };
    size_t sizes[] = {1, 2, 3, 6, 7, 11, 7, 11, 11, 7, 9};
    for (size_t i = 0; i + 1 < PRTS_ARRAY_COUNT(bad); i++) {
        prts_reader r;
        prts_reader_init(&r, bad[i], sizes[i]);
        ASSERT_FALSE(prts_script_read_plain(s, &r));
        ASSERT_TRUE(r.failed);
        ASSERT_EQ(0, lua_gettop(L));
    }
    prts_reader r;
    prts_reader_init(&r, bad[10], sizes[10]);
    ASSERT_TRUE(prts_script_read_plain(s, &r));
    lua_pop(L, 1);
    /* NaN as a key */
    unsigned char nan_key[] = {6, 1, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0xf8, 0x7f, 2};
    prts_reader_init(&r, nan_key, sizeof(nan_key));
    ASSERT_FALSE(prts_script_read_plain(s, &r));
    /* nesting deeper than the limit */
    static unsigned char deep[2000];
    for (size_t i = 0; i + 6 < sizeof(deep); i += 6) {
        unsigned char rec[6] = {6, 1, 0, 0, 0, 2}; /* { [true] = <next table> } */
        memcpy(deep + i, rec, 6);
    }
    prts_reader_init(&r, deep, sizeof(deep));
    ASSERT_FALSE(prts_script_read_plain(s, &r));
    ASSERT_EQ(0, lua_gettop(L));
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

/* --- snapshots -------------------------------------------------------------------------- */

static prts_engine *boot(const char *user)
{
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = PRTS_TEST_DATA;
    cfg.user_dir = user;
    cfg.headless = true;
    cfg.seed = 7;
    char err[1024];
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    prts_log_set_min_level(PRTS_LOG_INFO);
    return e;
}

static void frames(prts_engine *e, int n)
{
    for (int i = 0; i < n; i++) prts_engine_frame(e, e->clock.tick_seconds);
}

static bool door_solid(prts_engine *e)
{
    prts_entity door = prts_world_find(e->world, "keep.door");
    for (size_t i = 0; i < prts_cworld_box_count(e->cworld); i++) {
        if (prts_cworld_owner(e->cworld, (int)i) == door) return prts_cworld_enabled(e->cworld, (int)i);
    }
    return false;
}

static void drive(prts_engine *e)
{
    prts_input_override(e->input, "move_forward", 1.0f, 30);
    prts_input_override(e->input, "move_left", 1.0f, 12);
    e->commander.yaw += 0.25f;
}

UTEST(snapshot, save_and_load_continue_bit_for_bit)
{
    prts_engine *e = boot(TMP "/snap_user");
    ASSERT_TRUE(e != NULL);
    char err[512];
    /* open the door, walk and change script data, then save */
    ASSERT_TRUE(prts_script_run_string(e->script,
        "door = prts.entity.find('keep.door')\n"
        "prts.events.emit('use', door, 'keep.door')\n"
        "prts.entity.data(door).note = { visits = 2, tags = { 'oak', 'iron' } }\n"
        "x = math.random(1000)\n", "=setup"));
    drive(e);
    frames(e, 10);
    prts_entity scratch = prts_world_spawn(e->world, "scratch");
    prts_world_despawn(e->world, scratch); /* a free slot with a newer generation */
    prts_cworld_set_owner_enabled(e->cworld, prts_world_find(e->world, "keep.door"), false);
    uint64_t at_save = prts_engine_hash(e);
    ASSERT_TRUE(prts_engine_save(e, TMP "/snap/a.sav", err, sizeof(err)));
    prts_cworld_set_owner_enabled(e->cworld, prts_world_find(e->world, "keep.door"), true);
    drive(e);
    frames(e, 30);
    uint64_t later = prts_engine_hash(e);
    uint64_t tick = e->clock.tick;
    ASSERT_NE(at_save, later);
    /* load: the same state, and the same inputs give the same future */
    ASSERT_TRUE(prts_script_run_string(e->script, "prts.entity.data(door).note = 'changed'", "=t"));
    ASSERT_TRUE(prts_engine_load(e, TMP "/snap/a.sav", err, sizeof(err)));
    ASSERT_EQ(at_save, prts_engine_hash(e));
    ASSERT_STREQ("2", eval(e->script, "prts.entity.data(door).note.visits"));
    ASSERT_STREQ("iron", eval(e->script, "prts.entity.data(door).note.tags[2]"));
    ASSERT_FALSE(door_solid(e)); /* collision flags are part of the save */
    drive(e);
    frames(e, 30);
    ASSERT_EQ(later, prts_engine_hash(e));
    ASSERT_EQ(tick, e->clock.tick);
    prts_engine_destroy(e);
}

UTEST(snapshot, script_data_must_be_plain)
{
    /* 6.3: functions, userdata and threads in entity data are rejected, naming the field */
    prts_engine *e = boot(TMP "/snap_user");
    ASSERT_TRUE(e != NULL);
    char err[512];
    ASSERT_TRUE(prts_script_run_string(e->script,
        "prts.entity.data(prts.entity.find('keep.door')).on_open = function() end", "=t"));
    ASSERT_FALSE(prts_engine_save(e, TMP "/snap/bad.sav", err, sizeof(err)));
    ASSERT_TRUE(strstr(err, "entity 'keep.door' data field 'on_open' is a function") != NULL);
    ASSERT_TRUE(prts_script_run_string(e->script,
        "prts.entity.data(prts.entity.find('keep.door')).on_open = nil\n"
        "prts.entity.data(prts.entity.spawn{}).co = coroutine.create(print)", "=t"));
    ASSERT_FALSE(prts_engine_save(e, TMP "/snap/bad.sav", err, sizeof(err)));
    ASSERT_TRUE(strstr(err, "data field 'co' is a thread") != NULL);
    /* hashing still works with such data */
    ASSERT_TRUE(prts_engine_hash(e) != 0);
    prts_engine_destroy(e);
}

static bool load_bytes(prts_engine *e, const unsigned char *data, size_t size, char *err)
{
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    bool ok = prts_engine_snapshot_read(e, data, size, false, err, 512);
    prts_log_set_min_level(PRTS_LOG_INFO);
    return ok;
}

/* Rewrites the trailing checksum after a test edits the bytes. */
static void reseal(prts_writer *w)
{
    uint64_t h = prts_hash_bytes(w->data, w->len - 8u);
    for (int i = 0; i < 8; i++) w->data[w->len - 8u + (size_t)i] = (unsigned char)(h >> (8 * i));
}

UTEST(snapshot, damaged_or_foreign_saves_are_refused)
{
    prts_engine *e = boot(TMP "/snap_user");
    ASSERT_TRUE(e != NULL);
    char err[512];
    prts_writer w = {0};
    ASSERT_TRUE(prts_engine_snapshot_write(e, &w, err, sizeof(err)));
    uint64_t before = prts_engine_hash(e);
    ASSERT_TRUE(load_bytes(e, w.data, w.len, err));
    ASSERT_FALSE(load_bytes(e, w.data, 10, err));
    ASSERT_TRUE(strstr(err, "not a 1stPRTS save") != NULL);
    /* format version */
    w.data[8] = 9;
    ASSERT_FALSE(load_bytes(e, w.data, w.len, err));
    ASSERT_TRUE(strstr(err, "save format 9 is not supported") != NULL);
    w.data[8] = (unsigned char)PRTS_SNAPSHOT_FORMAT;
    /* checksum */
    w.data[30] ^= 0x40;
    ASSERT_FALSE(load_bytes(e, w.data, w.len, err));
    ASSERT_TRUE(strstr(err, "damaged") != NULL);
    w.data[30] ^= 0x40;
    /* engine version (a warning) and API version (refused) */
    w.data[16] = '9';
    reseal(&w);
    ASSERT_TRUE(load_bytes(e, w.data, w.len, err));
    w.data[16] = PRTS_ENGINE_VERSION[0];
    size_t api_at = 12 + 4 + strlen(PRTS_ENGINE_VERSION);
    w.data[api_at] = 7;
    reseal(&w);
    ASSERT_FALSE(load_bytes(e, w.data, w.len, err));
    ASSERT_TRUE(strstr(err, "scripting API 7") != NULL);
    w.data[api_at] = PRTS_API_VERSION;
    /* a different mod list only warns */
    size_t id_at = api_at + 4 + 4 + 4;
    w.data[id_at] = 'B';
    reseal(&w);
    ASSERT_TRUE(load_bytes(e, w.data, w.len, err));
    w.data[id_at] = 'b';
    reseal(&w);
    /* truncated in the middle (checksum still right) */
    prts_writer cut = {0};
    prts_w_bytes(&cut, w.data, w.len / 2);
    prts_w_u64(&cut, 0);
    reseal(&cut);
    ASSERT_FALSE(load_bytes(e, cut.data, cut.len, err));
    ASSERT_TRUE(strstr(err, "damaged") != NULL);
    prts_writer_free(&cut);
    /* a map with other collision geometry */
    prts_cworld_add_box(e->cworld, prts_aabb_make(prts_v3(0, 0, 0), prts_v3(1, 1, 1)), PRTS_ENTITY_NONE);
    ASSERT_FALSE(load_bytes(e, w.data, w.len, err));
    ASSERT_TRUE(strstr(err, "collision geometry differs") != NULL);
    /* nothing changed by any refused load */
    ASSERT_EQ(before, prts_engine_hash(e));
    ASSERT_FALSE(prts_engine_load(e, TMP "/snap/none.sav", err, sizeof(err)));
    ASSERT_TRUE(strstr(err, "cannot read") != NULL);
    ASSERT_FALSE(prts_engine_save(e, "/proc/1stprts/x.sav", err, sizeof(err)));
    ASSERT_TRUE(strstr(err, "cannot write") != NULL);
    prts_writer_free(&w);
    prts_engine_destroy(e);
}

UTEST(snapshot, damaged_sections_are_named)
{
    prts_engine *e = boot(TMP "/snap_user");
    ASSERT_TRUE(e != NULL);
    char err[512];
    ASSERT_TRUE(prts_script_run_string(e->script, "prts.entity.data(prts.entity.find('keep.door')).x = 1", "=t"));
    prts_writer w = {0};
    ASSERT_TRUE(prts_engine_snapshot_write(e, &w, err, sizeof(err)));
    /* find the sections by writing the parts again */
    prts_writer world = {0};
    prts_world_write(e->world, &world);
    size_t world_at = 0;
    for (size_t i = 0; i + world.len <= w.len; i++) {
        if (memcmp(w.data + i, world.data, world.len) == 0) {
            world_at = i;
            break;
        }
    }
    ASSERT_TRUE(world_at > 0);
    size_t boxes_at = world_at + world.len;
    prts_writer edit = {0};
    /* entity section: an absurd slot count */
    prts_w_bytes(&edit, w.data, w.len);
    edit.data[world_at + 3] = 0xff;
    reseal(&edit);
    ASSERT_FALSE(load_bytes(e, edit.data, edit.len, err));
    ASSERT_TRUE(strstr(err, "(entities)") != NULL);
    /* collision section: more boxes than bytes */
    memcpy(edit.data, w.data, w.len);
    prts_cworld *saved = e->cworld;
    e->cworld = prts_cworld_create();
    for (int i = 0; i < 100000; i++) prts_cworld_add_box(e->cworld, prts_aabb_make(prts_v3(0, 0, 0), prts_v3(1, 1, 1)), 0);
    edit.data[boxes_at] = (unsigned char)(100000 & 0xff);
    edit.data[boxes_at + 1] = (unsigned char)((100000 >> 8) & 0xff);
    edit.data[boxes_at + 2] = (unsigned char)(100000 >> 16);
    reseal(&edit);
    ASSERT_FALSE(load_bytes(e, edit.data, edit.len, err));
    ASSERT_TRUE(strstr(err, "(collision)") != NULL);
    prts_cworld_destroy(e->cworld);
    e->cworld = saved;
    /* script data section: rebuilt by hand after the commander */
    size_t edata_at = boxes_at + 4u + prts_cworld_box_count(e->cworld) + 1u + 8u + 17u * 4u + 8u * 4u + 1u + 2u + 8u + 4u;
    const unsigned char tails[][14] = {
        {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2},    /* one entity whose data is `true` */
        {5, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 6, 0}, /* five entities, data cut short */
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 7},             /* no data, no agents, no grid, then trailing bytes */
        {0, 0, 0, 0, 1, 0, 0, 0, 5},                /* one agent, cut short */
        {0, 0, 0, 0, 0, 0, 0, 0, 1, 9},             /* a navigation grid this map does not have */
    };
    const size_t tail_sizes[] = {13, 14, 10, 9, 10};
    const char *why[] = {"(script data)", "(script data)", "(units)", "(units)", "(navigation)"};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(tails); i++) {
        edit.len = 0;
        prts_w_bytes(&edit, w.data, edata_at);
        prts_w_bytes(&edit, tails[i], tail_sizes[i]);
        prts_w_u64(&edit, 0);
        reseal(&edit);
        ASSERT_FALSE(load_bytes(e, edit.data, edit.len, err));
        ASSERT_TRUE(strstr(err, why[i]) != NULL);
    }
    /* the untouched prefix with empty script data, no agents and no grid loads */
    edit.len = 0;
    prts_w_bytes(&edit, w.data, edata_at);
    prts_w_u32(&edit, 0);
    prts_w_u32(&edit, 0);
    prts_w_u8(&edit, 0);
    prts_w_u64(&edit, 0);
    reseal(&edit);
    ASSERT_TRUE(load_bytes(e, edit.data, edit.len, err));
    prts_writer_free(&edit);
    prts_writer_free(&world);
    prts_writer_free(&w);
    prts_engine_destroy(e);
}

UTEST(snapshot, save_slots_from_scripts)
{
    prts_engine *e = boot(TMP "/snap_user");
    ASSERT_TRUE(e != NULL);
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    ASSERT_STREQ("true", eval(e->script, "prts.save.write('slot_1')"));
    ASSERT_TRUE(prts_fs_is_file(TMP "/snap_user/saves/slot_1.sav"));
    uint64_t saved = prts_engine_hash(e);
    frames(e, 5);
    ASSERT_STREQ("true", eval(e->script, "prts.save.load('slot_1')"));
    /* applied at the start of the next tick, then that tick runs */
    ASSERT_NE(saved, prts_engine_hash(e));
    prts_engine_tick(e);
    ASSERT_EQ((uint64_t)1, e->clock.tick);
    ASSERT_STREQ("<error>", eval(e->script, "prts.save.write('../x')"));
    ASSERT_STREQ("<error>", eval(e->script, "prts.save.load('')"));
    ASSERT_STREQ("saves/none.sav: not found", eval(e->script, "select(2, prts.save.load('none'))"));
    ASSERT_STREQ("true", eval(e->script, "(function() prts.entity.data(prts.commander.entity()).f = print return true end)()"));
    ASSERT_TRUE(strstr(eval(e->script, "select(2, prts.save.write('slot_2'))"), "is a function") != NULL);
    /* a damaged slot is refused when requested */
    ASSERT_TRUE(prts_fs_write_file(TMP "/snap_user/saves/junk.sav", "junk", 4));
    ASSERT_STREQ("not a 1stPRTS save", eval(e->script, "select(2, prts.save.load('junk'))"));
    /* the user folder must be writable */
    prts_vfs_set_user_dir(e->vfs, NULL);
    ASSERT_STREQ("true", eval(e->script, "(function() prts.entity.data(prts.commander.entity()).f = nil return true end)()"));
    ASSERT_TRUE(strstr(eval(e->script, "select(2, prts.save.write('slot_3'))"), "cannot write") != NULL);
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_engine_destroy(e);
}

UTEST(snapshot, world_round_trip_and_truncation)
{
    prts_world *w = prts_world_create();
    prts_entity lift = prts_world_spawn(w, "lift");
    prts_entity rider = prts_world_spawn(w, NULL);
    prts_world_set_parent(w, rider, lift);
    prts_world_set_local(w, rider, prts_dv3(1.25, 2.5, -3.0), 0.5f);
    prts_world_set_velocity(w, rider, prts_v3(1, 0, 0));
    prts_world_move_to(w, lift, prts_dv3(0, 9, 0), 1.6f);
    prts_world_rotate_to(w, rider, 1.0f, 2.0f);
    prts_world_set_usable(w, lift, true);
    prts_world_despawn(w, prts_world_spawn(w, "gone"));
    prts_writer out = {0};
    prts_world_write(w, &out);
    prts_world *copy = prts_world_create();
    prts_reader r;
    prts_reader_init(&r, out.data, out.len);
    ASSERT_TRUE(prts_world_read(copy, &r));
    ASSERT_EQ(prts_world_hash(w), prts_world_hash(copy));
    ASSERT_EQ(prts_world_count(w), prts_world_count(copy));
    ASSERT_STREQ("lift", prts_world_name(copy, lift));
    ASSERT_EQ(lift, prts_world_parent(copy, rider));
    /* a cut-off copy is refused and leaves the world as it was */
    prts_reader_init(&r, out.data, out.len - 3);
    ASSERT_FALSE(prts_world_read(copy, &r));
    ASSERT_EQ(prts_world_hash(w), prts_world_hash(copy));
    prts_writer_free(&out);
    prts_world_destroy(copy);
    prts_world_destroy(w);
}

UTEST(snapshot, engine_without_script_data_or_found_data_folder)
{
    /* no entity has script data yet: the section is empty */
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = PRTS_TEST_FIXTURES "/data_tests";
    cfg.user_dir = TMP "/snap_user";
    cfg.headless = true;
    char err[1024];
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(prts_engine_hash(e) != 0);
    ASSERT_TRUE(prts_engine_save(e, TMP "/snap/empty.sav", err, sizeof(err)));
    ASSERT_TRUE(prts_engine_load(e, TMP "/snap/empty.sav", err, sizeof(err)));
    ASSERT_FALSE(e->has_commander);
    prts_engine_destroy(e);
    /* without --data the engine finds data/ next to the executable (the build copies it there) */
    cfg.data_dir = NULL;
    e = prts_engine_create(&cfg, err, sizeof(err));
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(prts_str_ends_with(e->data_dir, "data"));
    prts_engine_destroy(e);
}

UTEST(engine, boot_without_opengl_is_refused_with_a_message)
{
    /* Spec: OpenGL context unavailable (SDL's dummy video driver has no OpenGL at all) */
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = PRTS_TEST_DATA;
    cfg.user_dir = TMP "/snap_user";
    cfg.hidden = true;
    char err[1024];
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    prts_log_set_min_level(PRTS_LOG_INFO);
    SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
    ASSERT_TRUE(e == NULL);
    ASSERT_TRUE(strstr(err, "Banner & Bastion needs OpenGL 3.3") != NULL); /* named after the game's data */
}

UTEST(snapshot, a_save_that_stops_matching_before_it_is_applied)
{
    prts_engine *e = boot(TMP "/snap_user");
    ASSERT_TRUE(e != NULL);
    ASSERT_STREQ("true", eval(e->script, "prts.save.write('later')"));
    ASSERT_STREQ("true", eval(e->script, "prts.save.load('later')"));
    prts_cworld_add_box(e->cworld, prts_aabb_make(prts_v3(0, 0, 0), prts_v3(1, 1, 1)), PRTS_ENTITY_NONE);
    prts_log_reset_error_count();
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    prts_engine_tick(e);
    prts_log_set_min_level(PRTS_LOG_INFO);
    ASSERT_EQ((size_t)1, prts_log_error_count());
    ASSERT_EQ((uint64_t)1, e->clock.tick);
    prts_engine_destroy(e);
}
