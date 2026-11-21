/* 1stPRTS engine: saves, versioned binary snapshots of the simulation state. MIT License.
 *
 * Layout (all numbers little-endian, floats as IEEE-754 bits):
 *   "1PRTSNAP"  u32 format  str engine_version  u32 api_version
 *   u32 mounts { str id, str version }
 *   u64 tick  u64 rng_state  u64 rng_inc
 *   world (prts_world_write)
 *   u32 collision boxes { u8 enabled }
 *   u8 has_commander [ u64 entity, 17 f32 params, 8 f32 state, u8 flags, u8 jump buffer, u8 coyote,
 *                      u64 ridden frame, f32 its yaw ]
 *     state: yaw, pitch, vertical speed, eye, eye before, stamina, velocity x, velocity z
 *     flags: 1 on the ground, 2 crouched, 4 against a wall, 8 aiming a teleport, 16 in a jump
 *   u32 entities with script data { u64 entity, plain table }
 *   agents (prts_agents_write)  u8 has_nav [ navigation blocks (prts_nav_write) ]
 *   u64 FNV-1a checksum of everything before it */
#include "app/snapshot.h"

#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"

static const char SNAP_MAGIC[] = "1PRTSNAP";
#define MAGIC_LEN 8u

/* --- script data per entity --------------------------------------------------------------- */

static int compare_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}

/* Pushes the entity data table and fills keys with the living entities that have data, in
 * handle order. Returns false (pushing nothing) if no entity has data yet. */
static bool push_edata(const prts_engine *e, prts_array *keys)
{
    lua_State *L = prts_script_lua(e->script);
    prts_array_init(keys, sizeof(uint64_t));
    if (lua_getfield(L, LUA_REGISTRYINDEX, PRTS_REG_EDATA) != LUA_TTABLE) {
        lua_pop(L, 1);
        return false;
    }
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pop(L, 1);
        if (!lua_isinteger(L, -1)) continue;
        uint64_t ent = (uint64_t)lua_tointeger(L, -1);
        if (prts_world_alive(e->world, (prts_entity)ent)) prts_array_push(keys, &ent);
    }
    if (keys->len > 1) qsort(keys->data, keys->len, keys->elem, compare_u64);
    return true;
}

static void describe_entity(const prts_engine *e, uint64_t ent, char *out, size_t size)
{
    const char *name = prts_world_name(e->world, (prts_entity)ent);
    if (*name) snprintf(out, size, "entity '%s' data", name);
    else snprintf(out, size, "entity %llu data", (unsigned long long)ent);
}

void prts_engine_hash_script_data(const prts_engine *e, prts_hasher *h)
{
    prts_array keys;
    if (!push_edata(e, &keys)) return;
    lua_State *L = prts_script_lua(e->script);
    int t = lua_gettop(L);
    prts_writer w = {0};
    for (size_t i = 0; i < keys.len; i++) {
        uint64_t ent = *(uint64_t *)prts_array_at(&keys, i);
        lua_rawgeti(L, t, (lua_Integer)ent);
        w.len = 0;
        prts_script_write_plain(e->script, -1, &w, false, NULL, NULL, 0);
        prts_hasher_u64(h, ent);
        prts_hasher_bytes(h, w.data, w.len);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    prts_writer_free(&w);
    prts_array_free(&keys);
}

/* --- writing -------------------------------------------------------------------------------- */

static void write_params(prts_writer *w, const prts_commander_params *p)
{
    const float v[] = {p->walk_speed,    p->run_speed,   p->sprint_speed,  p->crouch_speed,  p->jump_speed,
                       p->gravity,       p->eye_stand,   p->eye_crouch,    p->height_stand,  p->height_crouch,
                       p->half_width,    p->step_height, p->stamina_max,   p->stamina_drain, p->stamina_regen,
                       p->reach,         p->look_sensitivity};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(v); i++) prts_w_f32(w, v[i]);
}

static void read_params(prts_reader *r, prts_commander_params *p)
{
    float *v[] = {&p->walk_speed,    &p->run_speed,   &p->sprint_speed,  &p->crouch_speed,  &p->jump_speed,
                  &p->gravity,       &p->eye_stand,   &p->eye_crouch,    &p->height_stand,  &p->height_crouch,
                  &p->half_width,    &p->step_height, &p->stamina_max,   &p->stamina_drain, &p->stamina_regen,
                  &p->reach,         &p->look_sensitivity};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(v); i++) *v[i] = prts_r_f32(r);
}

bool prts_engine_snapshot_write(prts_engine *e, prts_writer *w, char *err, size_t err_size)
{
    size_t start = w->len;
    prts_w_bytes(w, SNAP_MAGIC, MAGIC_LEN);
    prts_w_u32(w, PRTS_SNAPSHOT_FORMAT);
    prts_w_str(w, PRTS_ENGINE_VERSION);
    prts_w_u32(w, PRTS_API_VERSION);
    size_t mounts = prts_vfs_mount_count(e->vfs);
    prts_w_u32(w, (uint32_t)mounts);
    for (size_t i = 0; i < mounts; i++) {
        prts_w_str(w, prts_vfs_mount_id(e->vfs, i));
        prts_w_str(w, i < PRTS_ARRAY_COUNT(e->mod_versions) ? e->mod_versions[i] : NULL);
    }
    prts_w_u64(w, e->clock.tick);
    const prts_rng *rng = prts_script_rng(e->script);
    prts_w_u64(w, rng->state);
    prts_w_u64(w, rng->inc);
    prts_world_write(e->world, w);
    size_t boxes = prts_cworld_box_count(e->cworld);
    prts_w_u32(w, (uint32_t)boxes);
    for (size_t i = 0; i < boxes; i++) prts_w_u8(w, prts_cworld_enabled(e->cworld, (int)i) ? 1u : 0u);
    prts_w_u8(w, e->has_commander ? 1u : 0u);
    if (e->has_commander) {
        const prts_commander *c = &e->commander;
        prts_w_u64(w, c->entity);
        write_params(w, &c->p);
        const float state[] = {c->yaw, c->pitch, c->vertical_speed, c->eye, c->eye_prev, c->stamina, c->vel.x, c->vel.z};
        for (size_t i = 0; i < PRTS_ARRAY_COUNT(state); i++) prts_w_f32(w, state[i]);
        prts_w_u8(w, (uint8_t)((c->on_ground ? 1u : 0u) | (c->crouched ? 2u : 0u) | (c->hit_wall ? 4u : 0u) |
                               (c->aiming ? 8u : 0u) | (c->jumping ? 16u : 0u)));
        prts_w_u8(w, c->jump_buffer);
        prts_w_u8(w, c->coyote);
        prts_w_u64(w, c->ride_frame);
        prts_w_f32(w, c->ride_yaw);
    }
    prts_array keys;
    bool ok = true;
    if (push_edata(e, &keys)) {
        lua_State *L = prts_script_lua(e->script);
        int t = lua_gettop(L);
        prts_w_u32(w, (uint32_t)keys.len);
        for (size_t i = 0; i < keys.len && ok; i++) {
            uint64_t ent = *(uint64_t *)prts_array_at(&keys, i);
            char what[160];
            describe_entity(e, ent, what, sizeof(what));
            prts_w_u64(w, ent);
            lua_rawgeti(L, t, (lua_Integer)ent);
            ok = prts_script_write_plain(e->script, -1, w, true, what, err, err_size);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    } else {
        prts_w_u32(w, 0);
    }
    prts_array_free(&keys);
    if (!ok) {
        w->len = start;
        return false;
    }
    prts_agents_write(e->agents, w);
    prts_w_u8(w, e->nav ? 1u : 0u);
    if (e->nav) prts_nav_write(e->nav, w);
    prts_w_u64(w, prts_hash_bytes(w->data + start, w->len - start));
    return true;
}

/* --- reading -------------------------------------------------------------------------------- */

static bool snap_fail(char *err, size_t err_size, const char *msg)
{
    snprintf(err, err_size, "%s", msg);
    return false;
}

bool prts_engine_snapshot_read(prts_engine *e, const void *data, size_t size, bool dry_run, char *err,
                               size_t err_size)
{
    if (size < MAGIC_LEN + 12u || memcmp(data, SNAP_MAGIC, MAGIC_LEN) != 0) {
        return snap_fail(err, err_size, "not a 1stPRTS save");
    }
    prts_reader r;
    prts_reader_init(&r, data, size);
    r.pos = MAGIC_LEN;
    uint32_t format = prts_r_u32(&r);
    if (format != PRTS_SNAPSHOT_FORMAT) {
        snprintf(err, err_size, "save format %u is not supported (this build reads format %u)", format,
                 PRTS_SNAPSHOT_FORMAT);
        return false;
    }
    prts_reader sum;
    prts_reader_init(&sum, (const unsigned char *)data + size - 8u, 8u);
    if (prts_r_u64(&sum) != prts_hash_bytes(data, size - 8u)) return snap_fail(err, err_size, "the save is damaged");
    r.len = size - 8u;

    char *version = prts_r_str(&r, 64);
    uint32_t api = prts_r_u32(&r);
    if (version && strcmp(version, PRTS_ENGINE_VERSION) != 0) {
        prts_log_warn("this save was written by engine %s (running %s)", version, PRTS_ENGINE_VERSION);
    }
    prts_free(version);
    if (api != PRTS_API_VERSION) {
        snprintf(err, err_size, "the save uses scripting API %u, but this engine provides API %d", api, PRTS_API_VERSION);
        return false;
    }
    uint32_t mounts = prts_r_u32(&r);
    bool same_mods = mounts == prts_vfs_mount_count(e->vfs);
    for (uint32_t i = 0; i < mounts && !r.failed; i++) {
        char *id = prts_r_str(&r, 256), *ver = prts_r_str(&r, 64);
        if (id && (i >= prts_vfs_mount_count(e->vfs) || strcmp(id, prts_vfs_mount_id(e->vfs, i)) != 0)) {
            same_mods = false;
        }
        prts_free(id);
        prts_free(ver);
    }
    if (!same_mods && !r.failed) prts_log_warn("this save was made with a different set of mods");
    uint64_t tick = prts_r_u64(&r);
    prts_rng rng;
    rng.state = prts_r_u64(&r);
    rng.inc = prts_r_u64(&r);
    size_t world_at = r.pos;
    prts_world *check = prts_world_create();
    bool world_ok = prts_world_read(check, &r);
    prts_world_destroy(check);
    if (!world_ok) return snap_fail(err, err_size, "the save is damaged (entities)");
    uint32_t boxes = prts_r_u32(&r);
    if (r.failed || boxes != prts_cworld_box_count(e->cworld)) {
        return snap_fail(err, err_size, "the save does not match this map (its collision geometry differs)");
    }
    size_t boxes_at = r.pos;
    if (boxes > r.len - r.pos) return snap_fail(err, err_size, "the save is damaged (collision)");
    r.pos += boxes;
    bool has_commander = prts_r_u8(&r) != 0;
    prts_commander cmd;
    memset(&cmd, 0, sizeof(cmd));
    if (has_commander) {
        cmd.entity = prts_r_u64(&r);
        read_params(&r, &cmd.p);
        prts_commander *c = &cmd;
        float *state[] = {&c->yaw, &c->pitch, &c->vertical_speed, &c->eye, &c->eye_prev, &c->stamina, &c->vel.x, &c->vel.z};
        for (size_t i = 0; i < PRTS_ARRAY_COUNT(state); i++) *state[i] = prts_r_f32(&r);
        uint8_t flags = prts_r_u8(&r);
        cmd.on_ground = (flags & 1u) != 0;
        cmd.crouched = (flags & 2u) != 0;
        cmd.hit_wall = (flags & 4u) != 0;
        cmd.aiming = (flags & 8u) != 0;
        cmd.jumping = (flags & 16u) != 0;
        cmd.jump_buffer = prts_r_u8(&r);
        cmd.coyote = prts_r_u8(&r);
        cmd.ride_frame = prts_r_u64(&r);
        cmd.ride_yaw = prts_r_f32(&r);
    }
    /* script data: built into a new table, which replaces the old one only if all is well */
    lua_State *L = prts_script_lua(e->script);
    int top = lua_gettop(L);
    lua_newtable(L);
    uint32_t count = prts_r_u32(&r);
    for (uint32_t i = 0; i < count && !r.failed; i++) {
        uint64_t ent = prts_r_u64(&r);
        if (!prts_script_read_plain(e->script, &r)) break;
        if (!lua_istable(L, -1)) r.failed = true;
        lua_rawseti(L, top + 1, (lua_Integer)ent);
    }
    if (r.failed) {
        lua_settop(L, top);
        return snap_fail(err, err_size, "the save is damaged (script data)");
    }
    prts_agents *agents = prts_agents_create();
    bool agents_ok = prts_agents_read(agents, &r);
    bool has_nav = prts_r_u8(&r) != 0;
    size_t nav_at = r.pos;
    bool nav_ok = !has_nav || (e->nav && prts_nav_read(e->nav, &r, false));
    if (!agents_ok || !nav_ok || r.failed || r.pos != r.len) {
        lua_settop(L, top);
        prts_agents_destroy(agents);
        return snap_fail(err, err_size, nav_ok ? "the save is damaged (units)" : "the save does not match this map (navigation)");
    }
    if (dry_run) {
        lua_settop(L, top);
        prts_agents_destroy(agents);
        return true;
    }
    prts_agents_destroy(e->agents);
    e->agents = agents;
    if (has_nav) {
        prts_reader nr = r;
        nr.pos = nav_at;
        prts_nav_read(e->nav, &nr, true);
    }
    lua_setfield(L, LUA_REGISTRYINDEX, PRTS_REG_EDATA);
    r.pos = world_at;
    prts_world_read(e->world, &r);
    const unsigned char *flags = (const unsigned char *)data + boxes_at;
    for (uint32_t i = 0; i < boxes; i++) prts_cworld_set_enabled(e->cworld, (int)i, flags[i] != 0);
    e->has_commander = has_commander;
    if (has_commander) e->commander = cmd;
    e->clock.tick = tick;
    *prts_script_rng(e->script) = rng;
    prts_script_set_time(e->script, tick, e->clock.tick_seconds);
    prts_cworld_sync(e->cworld, e->world);
    return true;
}

bool prts_engine_save(prts_engine *e, const char *real_path, char *err, size_t err_size)
{
    prts_writer w = {0};
    bool ok = prts_engine_snapshot_write(e, &w, err, err_size);
    if (ok && !prts_fs_write_file(real_path, w.data, w.len)) {
        snprintf(err, err_size, "cannot write %s", real_path);
        ok = false;
    }
    prts_writer_free(&w);
    return ok;
}

bool prts_engine_load(prts_engine *e, const char *real_path, char *err, size_t err_size)
{
    char *data = NULL;
    size_t size = 0;
    if (!prts_fs_read_file(real_path, &data, &size)) {
        snprintf(err, err_size, "cannot read %s", real_path);
        return false;
    }
    bool ok = prts_engine_snapshot_read(e, data, size, false, err, err_size);
    prts_free(data);
    return ok;
}
