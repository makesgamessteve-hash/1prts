/* 1stPRTS engine: replays (start snapshot + per-tick inputs + state hashes) and save slots.
 * MIT License.
 *
 * Replay file (little-endian):
 *   "1PRTSRPL"  u32 format  u32 snapshot_size  snapshot bytes
 *   u32 actions { str name }  u32 ticks
 *   per tick: f32 yaw  f32 pitch  f32 value[actions]  u8 pressed[(actions + 7) / 8]  u64 state_hash
 *   u64 FNV-1a checksum of everything before it
 * The start snapshot is taken at the start of the first recorded tick, so whatever a scenario
 * set up before its first wait is part of it. After that only inputs and look direction are
 * recorded: anything else that changes the simulation shows up as a hash mismatch. */
#include <lauxlib.h>
#include <stdio.h>
#include <string.h>

#include "app/engine_internal.h"
#include "app/report.h"
#include "app/snapshot.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"

static const char RPL_MAGIC[] = "1PRTSRPL";
#define MAGIC_LEN 8u
#define MAX_ACTIONS 256u

typedef struct prts_replay prts_replay;

struct prts_replay {
    bool recording, playing;
    /* recording */
    bool started;
    prts_writer snapshot;
    uint32_t actions;
    prts_writer ticks;
    uint32_t count;
    /* playback */
    char *file;
    size_t file_size;
    prts_reader reader; /* over the tick records */
    int map[MAX_ACTIONS]; /* recorded action -> current action index */
    uint32_t index;
    uint64_t expected;
    /* a save slot to load at the start of the next tick */
    char *pending;
    size_t pending_size;
};

static prts_replay *replay_of(prts_engine *e)
{
    if (!e->replay) e->replay = prts_calloc(1, sizeof(prts_replay));
    return e->replay;
}

bool prts_engine_replaying(const prts_engine *e) { return e->replay && e->replay->playing; }

static size_t pressed_bytes(uint32_t actions) { return (actions + 7u) / 8u; }

static bool playback_open(prts_engine *e, prts_replay *rp, char *err, size_t err_size)
{
    const char *path = e->cfg.replay;
    if (!prts_fs_read_file(path, &rp->file, &rp->file_size)) {
        snprintf(err, err_size, "cannot read the replay %s", path);
        return false;
    }
    const unsigned char *data = (const unsigned char *)rp->file;
    size_t size = rp->file_size;
    if (size < MAGIC_LEN + 16u || memcmp(data, RPL_MAGIC, MAGIC_LEN) != 0) {
        snprintf(err, err_size, "%s is not a 1stPRTS replay", path);
        return false;
    }
    prts_reader r;
    prts_reader_init(&r, data, size - 8u);
    r.pos = MAGIC_LEN;
    uint32_t format = prts_r_u32(&r);
    prts_reader sum;
    prts_reader_init(&sum, data + size - 8u, 8u);
    if (format != PRTS_REPLAY_FORMAT || prts_r_u64(&sum) != prts_hash_bytes(data, size - 8u)) {
        snprintf(err, err_size, "the replay %s is damaged or has an unsupported format", path);
        return false;
    }
    uint32_t snap = prts_r_u32(&r);
    if (r.failed || snap > r.len - r.pos) {
        snprintf(err, err_size, "the replay %s is damaged", path);
        return false;
    }
    char why[256];
    if (!prts_engine_snapshot_read(e, data + r.pos, snap, false, why, sizeof(why))) {
        snprintf(err, err_size, "the replay's start state cannot be loaded: %s", why);
        return false;
    }
    r.pos += snap;
    uint32_t actions = prts_r_u32(&r);
    if (actions > MAX_ACTIONS) r.failed = true;
    for (uint32_t i = 0; i < actions && !r.failed; i++) {
        char *name = prts_r_str(&r, 128);
        rp->map[i] = name ? prts_input_find(e->input, name) : -1;
        if (name && rp->map[i] < 0) {
            snprintf(err, err_size, "the replay uses the action '%s', which this game does not define", name);
            prts_free(name);
            return false;
        }
        prts_free(name);
    }
    rp->actions = actions;
    rp->count = prts_r_u32(&r);
    size_t record = 8u + 4u * (size_t)actions + pressed_bytes(actions) + 8u;
    if (r.failed || (r.len - r.pos) / record != rp->count || (r.len - r.pos) % record != 0) {
        snprintf(err, err_size, "the replay %s is damaged", path);
        return false;
    }
    prts_reader_init(&rp->reader, data + r.pos, r.len - r.pos);
    rp->playing = true;
    prts_log_info("replaying %s: %u ticks from tick %llu", path, rp->count, (unsigned long long)e->clock.tick);
    if (e->report) prts_report_begin(e->report, "replay", path);
    return true;
}

bool prts_engine_replay_start(prts_engine *e, char *err, size_t err_size)
{
    if (e->cfg.replay) return playback_open(e, replay_of(e), err, err_size);
    if (e->cfg.record) replay_of(e)->recording = true;
    return true;
}

static void record_start(prts_engine *e, prts_replay *rp)
{
    char err[512];
    rp->started = true;
    if (!prts_engine_snapshot_write(e, &rp->snapshot, err, sizeof(err))) {
        prts_log_error("cannot record a replay: %s", err);
        rp->recording = false;
        e->exit_code = 1;
        return;
    }
    size_t n = prts_input_action_count(e->input);
    rp->actions = (uint32_t)PRTS_MIN(n, (size_t)MAX_ACTIONS);
}

static void record_inputs(prts_engine *e, prts_replay *rp)
{
    if (!rp->started) record_start(e, rp);
    if (!rp->recording) return;
    prts_w_f32(&rp->ticks, e->commander.yaw);
    prts_w_f32(&rp->ticks, e->commander.pitch);
    unsigned char pressed[MAX_ACTIONS / 8u];
    memset(pressed, 0, sizeof(pressed));
    for (uint32_t i = 0; i < rp->actions; i++) {
        float v;
        bool p;
        prts_input_tick_state(e->input, (int)i, &v, &p);
        prts_w_f32(&rp->ticks, v);
        if (p) pressed[i / 8u] |= (unsigned char)(1u << (i % 8u));
    }
    prts_w_bytes(&rp->ticks, pressed, pressed_bytes(rp->actions));
}

static void play_inputs(prts_engine *e, prts_replay *rp)
{
    prts_reader *r = &rp->reader;
    e->commander.yaw = prts_r_f32(r);
    e->commander.pitch = prts_r_f32(r);
    float values[MAX_ACTIONS];
    for (uint32_t i = 0; i < rp->actions; i++) values[i] = prts_r_f32(r);
    unsigned char pressed[MAX_ACTIONS / 8u];
    prts_r_bytes(r, pressed, pressed_bytes(rp->actions));
    /* only recorded input reaches the simulation: actions the recording lacks stay idle */
    for (size_t a = 0; a < prts_input_action_count(e->input); a++) prts_input_set_tick_state(e->input, (int)a, 0.0f, false);
    for (uint32_t i = 0; i < rp->actions; i++) {
        prts_input_set_tick_state(e->input, rp->map[i], values[i], (pressed[i / 8u] >> (i % 8u)) & 1u);
    }
    rp->expected = prts_r_u64(r);
}

void prts_engine_replay_before_tick(prts_engine *e)
{
    prts_replay *rp = e->replay;
    if (!rp) return;
    if (rp->pending) {
        char err[256];
        if (!prts_engine_snapshot_read(e, rp->pending, rp->pending_size, false, err, sizeof(err))) {
            prts_log_error("loading the save failed: %s", err); /* checked when requested, so rare */
        }
        prts_free(rp->pending);
        rp->pending = NULL;
    }
    if (rp->recording) record_inputs(e, rp);
    if (rp->playing && rp->index < rp->count) play_inputs(e, rp);
}

static void playback_check(prts_engine *e, prts_replay *rp)
{
    if (rp->index >= rp->count) return;
    uint64_t got = prts_engine_hash(e);
    rp->index++;
    if (got != rp->expected) {
        char msg[256];
        snprintf(msg, sizeof(msg), "replay diverged at tick %llu (record %u of %u): recorded state hash %016llx, "
                 "replayed %016llx", (unsigned long long)e->clock.tick, rp->index, rp->count,
                 (unsigned long long)rp->expected, (unsigned long long)got);
        prts_log_error("%s", msg);
        if (e->report) prts_report_end(e->report, (double)rp->index * e->clock.tick_seconds, msg);
        rp->playing = false;
        e->exit_code = 1;
        e->quit = true;
    } else if (rp->index == rp->count) {
        prts_dvec3 p = e->has_commander ? prts_world_pos(e->world, e->commander.entity) : prts_dv3(0, 0, 0);
        prts_log_info("replay verified: all %u ticks match the recording; the commander ends at %.3f %.3f %.3f",
                      rp->count, p.x, p.y, p.z);
        if (e->report) prts_report_end(e->report, (double)rp->count * e->clock.tick_seconds, NULL);
        rp->playing = false;
        e->quit = true;
    }
}

void prts_engine_replay_after_tick(prts_engine *e)
{
    prts_replay *rp = e->replay;
    if (!rp) return;
    if (rp->recording) {
        prts_w_u64(&rp->ticks, prts_engine_hash(e));
        rp->count++;
    }
    if (rp->playing) playback_check(e, rp);
}

static void write_recording(prts_engine *e, prts_replay *rp)
{
    if (!rp->started) record_start(e, rp);
    if (!rp->recording) return;
    prts_writer w = {0};
    prts_w_bytes(&w, RPL_MAGIC, MAGIC_LEN);
    prts_w_u32(&w, PRTS_REPLAY_FORMAT);
    prts_w_u32(&w, (uint32_t)rp->snapshot.len);
    prts_w_bytes(&w, rp->snapshot.data, rp->snapshot.len);
    prts_w_u32(&w, rp->actions);
    for (uint32_t i = 0; i < rp->actions; i++) prts_w_str(&w, prts_input_action_name(e->input, (int)i));
    prts_w_u32(&w, rp->count);
    prts_w_bytes(&w, rp->ticks.data, rp->ticks.len);
    prts_w_u64(&w, prts_hash_bytes(w.data, w.len));
    if (prts_fs_write_file(e->cfg.record, w.data, w.len)) {
        prts_log_info("replay recorded: %u ticks to %s", rp->count, e->cfg.record);
    } else {
        prts_log_error("cannot write the replay %s", e->cfg.record);
        e->exit_code = 1;
    }
    prts_writer_free(&w);
}

void prts_engine_replay_finish(prts_engine *e)
{
    prts_replay *rp = e->replay;
    if (!rp) return;
    if (rp->recording) write_recording(e, rp);
    prts_writer_free(&rp->snapshot);
    prts_writer_free(&rp->ticks);
    prts_free(rp->file);
    prts_free(rp->pending);
    prts_free(rp);
    e->replay = NULL;
}

/* --- prts.save: slots in the user area -------------------------------------------------------- */

static const char *check_slot(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    size_t n = strlen(name);
    bool ok = n > 0 && n <= 64;
    for (const char *p = name; *p && ok; p++) {
        ok = (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_' || *p == '-';
    }
    if (!ok) luaL_error(L, "invalid save slot name '%s' (use 1-64 letters, digits, '_' or '-')", name);
    return lua_pushfstring(L, "saves/%s.sav", name);
}

static int fail_result(lua_State *L, const char *msg)
{
    lua_pushnil(L);
    lua_pushstring(L, msg);
    return 2;
}

static int l_save_write(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *vpath = check_slot(L);
    char err[512];
    prts_writer w = {0};
    bool ok = prts_engine_snapshot_write(e, &w, err, sizeof(err));
    if (ok && prts_vfs_user_write(e->vfs, vpath, w.data, w.len) != PRTS_VFS_OK) {
        snprintf(err, sizeof(err), "cannot write %s in the user folder", vpath);
        ok = false;
    }
    prts_writer_free(&w);
    if (!ok) return fail_result(L, err);
    lua_pushboolean(L, 1);
    return 1;
}

static int l_save_load(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *vpath = check_slot(L);
    if (e->replay && (e->replay->recording || e->replay->playing)) {
        return fail_result(L, "saves cannot be loaded while a replay is recorded or played");
    }
    char *data = NULL;
    size_t size = 0;
    prts_vfs_result r = prts_vfs_user_read(e->vfs, vpath, &data, &size);
    if (r != PRTS_VFS_OK) return fail_result(L, lua_pushfstring(L, "%s: %s", vpath, prts_vfs_result_name(r)));
    char err[256];
    if (!prts_engine_snapshot_read(e, data, size, true, err, sizeof(err))) {
        prts_free(data);
        return fail_result(L, err);
    }
    prts_replay *rp = replay_of(e);
    prts_free(rp->pending);
    rp->pending = data;
    rp->pending_size = size;
    lua_pushboolean(L, 1);
    return 1;
}

void prts_engine_open_save_api(prts_engine *e)
{
    static const prts_script_fn save[] = {{"write", l_save_write}, {"load", l_save_load}, {NULL, NULL}};
    prts_script_register(e->script, "save", save);
}
