/* 1stPRTS engine: audio scripting API (prts.audio) and the per-frame audio update. MIT License. */
#include <lauxlib.h>
#include <string.h>

#include "app/engine_internal.h"

static prts_audio *A(lua_State *L) { return prts_engine_from_lua(L)->audio; }

static prts_audio_bus to_bus(lua_State *L, const char *name)
{
    prts_audio_bus b = PRTS_BUS_EFFECTS;
    if (!prts_audio_bus_parse(name, &b)) luaL_error(L, "unknown audio bus '%s'", name ? name : "(none)");
    return b;
}

static float opt_field(lua_State *L, const char *key, float def)
{
    lua_getfield(L, 1, key);
    int is_num = 0;
    lua_Number n = lua_tonumberx(L, -1, &is_num);
    if (!is_num && !lua_isnil(L, -1)) luaL_error(L, "field '%s' must be a number", key);
    lua_pop(L, 1);
    return is_num ? (float)n : def;
}

/* Reads {x, y, z} or {x=, y=, z=} from field `key` of the options table. */
static bool opt_vec3(lua_State *L, const char *key, prts_dvec3 *out)
{
    lua_getfield(L, 1, key);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    if (!lua_istable(L, -1)) luaL_error(L, "field '%s' must be a {x, y, z} table", key);
    int t = lua_gettop(L);
    bool array = lua_rawgeti(L, t, 1) != LUA_TNIL;
    lua_pop(L, 1);
    double c[3] = {0.0, 0.0, 0.0};
    for (int i = 0; i < 3; i++) {
        if (array) lua_rawgeti(L, t, i + 1);
        else lua_getfield(L, t, i == 0 ? "x" : i == 1 ? "y" : "z");
        c[i] = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    *out = prts_dv3(c[0], c[1], c[2]);
    return true;
}

static void push_handle(lua_State *L, prts_voice_id v)
{
    if (v == PRTS_VOICE_NONE) lua_pushnil(L);
    else lua_pushinteger(L, (lua_Integer)v);
}

/* Handles of finished sounds are harmless: functions taking one just return false. */
static prts_voice_id opt_handle(lua_State *L, int idx) { return (prts_voice_id)luaL_optinteger(L, idx, 0); }

/* prts.audio.play{ sound =, at = {x, y, z} | entity =, bus = "effects", volume = 1, loop = false,
 * range = 40 } -> handle (nil if every voice is busy) */
static int l_play(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "sound");
    const char *sound = lua_tostring(L, -1);
    if (!sound) return luaL_error(L, "field 'sound' is required");
    lua_getfield(L, 1, "bus");
    prts_voice_params p;
    prts_voice_params_default(&p);
    if (!lua_isnil(L, -1)) p.bus = to_bus(L, lua_tostring(L, -1));
    lua_pop(L, 1);
    p.volume = opt_field(L, "volume", 1.0f);
    p.max_distance = opt_field(L, "range", 0.0f);
    lua_getfield(L, 1, "loop");
    p.loop = lua_toboolean(L, -1);
    lua_getfield(L, 1, "entity");
    if (!lua_isnil(L, -1)) {
        prts_entity ent = prts_engine_check_entity(L, -1);
        p.positional = true;
        p.pos = prts_world_pos(e->world, ent);
        p.tag = ent;
    } else {
        p.positional = opt_vec3(L, "at", &p.pos);
    }
    lua_pop(L, 2);
    push_handle(L, prts_audio_play(e->audio, sound, &p));
    return 1;
}

/* prts.audio.stop(h [, fade_seconds]) -> true if it was playing */
static int l_stop(lua_State *L)
{
    prts_voice_id v = opt_handle(L, 1);
    lua_pushboolean(L, prts_mixer_stop(prts_audio_mixer(A(L)), v, (float)luaL_optnumber(L, 2, 0.0)));
    return 1;
}

/* prts.audio.music{ track = "music/x.ogg" | nil, fade = 1, volume = 1 } cross-fades; no
 * track fades the music out. Returns the music's handle. */
static int l_music(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "track");
    const char *track = lua_tostring(L, -1);
    float fade = opt_field(L, "fade", 1.0f), volume = opt_field(L, "volume", 1.0f);
    push_handle(L, prts_audio_music(A(L), track, fade, volume));
    return 1;
}

static int l_set_volume(lua_State *L)
{
    prts_audio_bus b = to_bus(L, luaL_checkstring(L, 1));
    prts_mixer_set_bus_volume(prts_audio_mixer(A(L)), b, (float)luaL_checknumber(L, 2));
    return 0;
}

static int l_volume(lua_State *L)
{
    prts_audio_bus b = to_bus(L, luaL_checkstring(L, 1));
    lua_pushnumber(L, prts_mixer_bus_volume(prts_audio_mixer(A(L)), b));
    return 1;
}

/* prts.audio.muffle(h, amount 0..1) -> true if the sound is playing */
static int l_muffle(lua_State *L)
{
    prts_voice_id v = opt_handle(L, 1);
    lua_pushboolean(L, prts_audio_set_muffle(A(L), v, (float)luaL_checknumber(L, 2)));
    return 1;
}

static void set_num(lua_State *L, const char *key, double v)
{
    lua_pushnumber(L, v);
    lua_setfield(L, -2, key);
}

static void set_bool(lua_State *L, const char *key, bool v)
{
    lua_pushboolean(L, v);
    lua_setfield(L, -2, key);
}

/* prts.audio.voice(h) -> nil once the sound has finished, or its state. For tests and
 * interfaces only: simulation code must not depend on audio. */
static int l_voice(lua_State *L)
{
    prts_voice_info info;
    prts_voice_id v = opt_handle(L, 1);
    if (!prts_mixer_voice_info(prts_audio_mixer(A(L)), v, &info)) return 0;
    lua_createtable(L, 0, 11);
    lua_pushstring(L, prts_audio_bus_name(info.bus));
    lua_setfield(L, -2, "bus");
    set_num(L, "volume", info.volume);
    set_num(L, "muffle", info.muffle);
    set_num(L, "occlusion", info.occlusion);
    set_num(L, "fade", info.fade);
    set_num(L, "left", info.gain_left);
    set_num(L, "right", info.gain_right);
    set_num(L, "cutoff", info.cutoff_hz);
    set_bool(L, "loop", info.loop);
    set_bool(L, "positional", info.positional);
    set_bool(L, "stopping", info.stopping);
    set_bool(L, "placeholder", prts_audio_is_placeholder(A(L), v));
    return 1;
}

/* prts.audio.info() -> { device = "sdl" | "null", rate, voices, left, right, held, mixed, music } */
static int l_info(lua_State *L)
{
    prts_audio *a = A(L);
    float l = 0.0f, r = 0.0f;
    prts_audio_levels(a, &l, &r);
    lua_createtable(L, 0, 8);
    lua_pushstring(L, prts_audio_has_device(a) ? "sdl" : "null");
    lua_setfield(L, -2, "device");
    set_num(L, "rate", prts_mixer_rate(prts_audio_mixer(a)));
    set_num(L, "voices", prts_mixer_voice_count(prts_audio_mixer(a)));
    set_num(L, "left", l);
    set_num(L, "right", r);
    set_bool(L, "held", prts_audio_held(a));
    set_num(L, "mixed", (double)prts_audio_frames_mixed(a) / PRTS_AUDIO_RATE);
    push_handle(L, prts_audio_music_voice(a));
    lua_setfield(L, -2, "music");
    return 1;
}

void prts_engine_open_audio_api(prts_engine *e)
{
    static const prts_script_fn audio[] = {
        {"play", l_play},     {"stop", l_stop},     {"music", l_music}, {"set_volume", l_set_volume},
        {"volume", l_volume}, {"muffle", l_muffle}, {"voice", l_voice}, {"info", l_info},
        {NULL, NULL}};
    prts_script_register(e->script, "audio", audio);
}

typedef struct follow_ctx {
    prts_engine *e;
    double alpha;
} follow_ctx;

/* Sounds played on an entity follow it, and stop when it is gone. */
static void follow_entity(prts_voice_id v, const prts_voice_info *info, void *user)
{
    follow_ctx *c = user;
    prts_entity ent = (prts_entity)info->tag;
    if (ent == PRTS_ENTITY_NONE) return;
    prts_mixer *m = prts_audio_mixer(c->e->audio);
    if (prts_world_alive(c->e->world, ent)) prts_mixer_set_position(m, v, prts_world_pos_interp(c->e->world, ent, c->alpha));
    else prts_mixer_stop(m, v, 0.05f);
}

typedef struct occlude_ctx {
    prts_engine *e;
    prts_cell_mask heard; /* cells sound reaches the listener from */
} occlude_ctx;

/* Spec: Door closes on a noisy room. A positional sound in a cell the listener's cell cannot
 * reach through open portals is muffled; it clears as soon as a connecting portal opens. */
static void occlude(prts_voice_id v, const prts_voice_info *info, void *user)
{
    occlude_ctx *c = user;
    float amount = 0.0f;
    if (info->positional) {
        int cell = prts_scene_cell_at(c->e->scene, prts_v3((float)info->pos.x, (float)info->pos.y, (float)info->pos.z));
        amount = (c->heard & PRTS_CELL_BIT(cell)) ? 0.0f : 1.0f;
    }
    prts_mixer_set_occlusion(prts_audio_mixer(c->e->audio), v, amount);
}

void prts_engine_audio_frame(prts_engine *e, double real_seconds, double alpha)
{
    occlude_ctx oc = {e, PRTS_CELLS_ALL};
    if (e->has_commander) {
        prts_dvec3 eye = prts_commander_eye(&e->commander, e->world, alpha);
        prts_audio_set_listener(e->audio, eye, e->commander.yaw);
        int cell = prts_scene_cell_at(e->scene, prts_v3((float)eye.x, (float)eye.y, (float)eye.z));
        oc.heard = prts_scene_reachable_cells(e->scene, cell);
    }
    follow_ctx c = {e, alpha};
    prts_mixer_each(prts_audio_mixer(e->audio), follow_entity, &c);
    prts_mixer_each(prts_audio_mixer(e->audio), occlude, &oc);
    prts_audio_update(e->audio, real_seconds);
}
