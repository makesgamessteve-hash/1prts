/* 1stPRTS engine: playing in XR (design D13). Controllers drive the same actions as other input
 * schemes; the head turns the view and the body follows it across the play area; walls push
 * the view back; comfort options (smooth or teleport movement, snap or smooth turning, the
 * comfort vignette, seated mode) and the video settings used in XR; the pointer rays. Lua:
 * prts.xr and the scenario helpers t.xr_*. MIT License.
 *
 * The play area (the XR reference space) is anchored at the commander's feet plus `room`,
 * turned by `turn`. Head and hand poses are frame input, like mouse look: each tick reads them
 * as the commander's XR input (prts_commander_xr, recorded in replays), so the simulation stays
 * deterministic. When the body follows the head, `room` moves back by the same amount, so the
 * play area stays where it is in the world. */
#include <lauxlib.h>
#include <math.h>
#include <string.h>

#include "app/engine_internal.h"
#include "app/snapshot.h"
#include "prts/prts_xr.h"

#define DEG ((float)PRTS_PI / 180.0f)
#define POINTER_DIM 0.35f
#define VIGNETTE_RATE 4.0f /* strength per second while easing in or out */

static const prts_vec3 RAY_IDLE = {0.75f, 0.72f, 0.62f};
static const prts_vec3 RAY_TARGET = {1.0f, 0.78f, 0.32f};
static const prts_vec3 RAY_TELEPORT = {0.35f, 1.0f, 0.45f};
static const prts_vec3 RAY_BLOCKED = {1.0f, 0.3f, 0.25f};

void prts_engine_xr_init(prts_engine *e)
{
    prts_engine_xr *x = &e->xr;
    memset(x, 0, sizeof(*x));
    x->snap_turn = true;
    x->snap_angle = 45.0f * DEG;
    x->turn_speed = 120.0f * DEG;
    x->vignette = true;
    x->vignette_strength = 0.6f;
    /* Spec "Comfort defaults": vertex snapping and screen-wide retro effects are off in XR */
    x->preset = PRTS_RES_HD;
    x->snap = x->dither = x->affine = x->crosshair = false;
    x->hand = 1;
}

/* --- the play area ----------------------------------------------------------------------- */

/* A position in the reference space, relative to the commander's feet (world axes). */
static prts_vec3 to_feet(const prts_engine_xr *x, const float p[3], prts_vec3 room)
{
    prts_vec3 v = prts_v3_rotate_y(prts_v3(p[0], p[1], p[2]), x->turn);
    return prts_v3_add(prts_v3_add(v, room), prts_v3(0.0f, x->seat, 0.0f));
}

static prts_vec3 flat_head(const prts_xr *xr) { return prts_v3(xr->head.position[0], 0.0f, xr->head.position[2]); }

/* The first tracked frame of a session: keep looking where the flat view looked, with the
 * body right under the head; seated mode measures the seat. */
static void align(prts_engine *e, const prts_xr *xr)
{
    prts_engine_xr *x = &e->xr;
    float yaw = 0.0f, pitch = 0.0f;
    prts_xr_yaw_pitch(&xr->head, &yaw, &pitch);
    x->turn = prts_wrap_angle(e->commander.yaw - yaw);
    x->room = x->room_prev = prts_v3_scale(prts_v3_rotate_y(flat_head(xr), x->turn), -1.0f);
    x->seat = x->seated ? e->commander.p.eye_stand - xr->head.position[1] : 0.0f;
    x->align_pending = false;
}

/* Turning the play area turns it around the head, so the player does not swing sideways. */
static void turn_to(prts_engine_xr *x, const prts_xr *xr, float turn)
{
    prts_vec3 h = flat_head(xr);
    prts_vec3 before = prts_v3_rotate_y(h, x->turn), after = prts_v3_rotate_y(h, turn);
    prts_vec3 shift = prts_v3_sub(before, after);
    x->room = prts_v3_add(x->room, shift);
    x->room_prev = prts_v3_add(x->room_prev, shift);
    x->turn = prts_wrap_angle(turn);
}

static void start_session(prts_engine *e)
{
    prts_engine_xr *x = &e->xr;
    x->flat_scheme = e->dev.last_device;
    x->align_pending = true;
    x->snap_ready = true;
    x->aim_locked = false;
    x->hand = 1;
    x->vignette_level = 0.0f;
}

static void end_session(prts_engine *e)
{
    prts_engine_xr *x = &e->xr;
    e->dev.last_device = x->flat_scheme; /* back to the flat page's controls */
    memset(&e->commander.xr, 0, sizeof(e->commander.xr));
    x->vignette_level = 0.0f;
    x->pushback = prts_v3(0, 0, 0);
    prts_gfx_set_rays(e->gfx, NULL, 0);
}

void prts_engine_xr_before_input(prts_engine *e)
{
    const prts_xr *xr = prts_xr_get();
    prts_engine_xr *x = &e->xr;
    if (xr->active && !x->was_active) start_session(e);
    if (!xr->active && x->was_active) end_session(e);
    x->was_active = xr->active;
    prts_xr_feed_input(xr, &e->dev);
    if (xr->active) e->dev.last_device = PRTS_SCHEME_XR;
}

/* --- per frame: turning, the head's view direction, the pointer hand, the vignette -------- */

static int stick_hand(const prts_engine *e)
{
    return e->dev.xr_axes[1][3] < e->dev.xr_axes[0][3] ? 1 : 0; /* the stick pushed further forward */
}

/* The pointer hand: the hand whose trigger was pulled last; while a teleport is aimed, the hand
 * of the stick that aims it (kept until the teleport is done). */
static void choose_hand(prts_engine *e)
{
    prts_engine_xr *x = &e->xr;
    bool aiming = x->teleport && (prts_input_down(e->input, "teleport") || e->commander.aiming);
    if (aiming && !x->aim_locked) x->hand = stick_hand(e);
    x->aim_locked = aiming;
    for (int h = 0; h < 2; h++) {
        float trigger = e->dev.xr_buttons[h][0];
        if (!aiming && trigger >= 1.0f && x->prev_trigger[h] < 1.0f) x->hand = h;
        x->prev_trigger[h] = trigger;
    }
}

static float move_amount(const prts_input *in)
{
    return fabsf(prts_input_value(in, "move_forward") - prts_input_value(in, "move_back")) +
           fabsf(prts_input_value(in, "move_right") - prts_input_value(in, "move_left"));
}

bool prts_engine_xr_look(prts_engine *e, double dt)
{
    const prts_xr *xr = prts_xr_get();
    prts_engine_xr *x = &e->xr;
    if (!xr->active || !e->has_commander || prts_engine_replaying(e)) return false;
    if (!xr->has_pose) return true; /* tracking lost: the view stays as it was */
    if (x->align_pending) align(e, xr);
    /* look_x turns the play area: in steps (snap) or continuously (smooth) */
    float v = prts_clampf(prts_input_value(e->input, "look_x"), -1.0f, 1.0f);
    bool turning = false;
    if (x->snap_turn) {
        if (fabsf(v) < 0.3f) x->snap_ready = true;
        if (fabsf(v) > 0.7f && x->snap_ready) {
            turn_to(x, xr, x->turn - copysignf(x->snap_angle, v));
            x->snap_ready = false;
        }
    } else if (v != 0.0f) {
        turn_to(x, xr, x->turn - v * x->turn_speed * (float)dt);
        turning = true;
    }
    /* the head's view direction is the commander's: movement goes where the player looks */
    float yaw = 0.0f, pitch = 0.0f;
    prts_xr_yaw_pitch(&xr->head, &yaw, &pitch);
    e->commander.yaw = prts_wrap_angle(x->turn + yaw);
    e->commander.pitch = prts_clampf(pitch, -1.48f, 1.48f);
    choose_hand(e);
    /* Spec "Comfort options": the comfort vignette while moving or turning smoothly */
    bool moving = !x->teleport && move_amount(e->input) > 0.1f;
    float target = x->vignette && (moving || turning) ? x->vignette_strength : 0.0f;
    float step = VIGNETTE_RATE * (float)dt;
    x->vignette_level = fabsf(target - x->vignette_level) <= step ? target
                      : x->vignette_level + (target > x->vignette_level ? step : -step);
    return true;
}

/* --- per tick: the commander's XR input ------------------------------------------------------ */

static const prts_xr_controller *pointer_controller(const prts_xr *xr, int hand)
{
    const prts_xr_controller *c = prts_xr_hand_controller(xr, hand ? PRTS_XR_HAND_RIGHT : PRTS_XR_HAND_LEFT);
    if (c && c->has_aim) return c;
    c = prts_xr_hand_controller(xr, hand ? PRTS_XR_HAND_LEFT : PRTS_XR_HAND_RIGHT);
    return c && c->has_aim ? c : NULL;
}

void prts_engine_xr_before_tick(prts_engine *e)
{
    const prts_xr *xr = prts_xr_get();
    prts_engine_xr *x = &e->xr;
    prts_commander_xr *in = &e->commander.xr;
    x->room_prev = x->room;
    memset(in, 0, sizeof(*in));
    if (!xr->active || !e->has_commander || x->align_pending) return;
    in->active = true;
    in->teleport = x->teleport;
    if (!xr->has_pose) {
        in->head = prts_v3(0.0f, e->commander.p.eye_stand, 0.0f); /* lost: the body stays */
        return;
    }
    in->head = to_feet(x, xr->head.position, x->room);
    const prts_xr_controller *c = pointer_controller(xr, x->hand);
    if (!c) return;
    in->has_pointer = true;
    in->pointer_origin = to_feet(x, c->aim.position, x->room);
    in->pointer_dir = prts_v3_rotate_y(prts_xr_forward(&c->aim), x->turn);
}

void prts_engine_xr_after_tick(prts_engine *e)
{
    if (e->xr.was_active) e->xr.room = prts_v3_sub(e->xr.room, e->commander.followed);
}

/* --- rendering ---------------------------------------------------------------------------- */

/* The play area's origin and the head push-back at render time (interpolated like the world). */
static prts_dvec3 anchor(prts_engine *e, const prts_xr *xr, double alpha, prts_vec3 *room_out)
{
    prts_engine_xr *x = &e->xr;
    prts_dvec3 feet = prts_world_pos_interp(e->world, e->commander.entity, alpha);
    prts_vec3 room = prts_v3_lerp(x->room_prev, x->room, (float)alpha);
    /* Spec "Leaning toward a wall": the view does not pass through the wall's surface */
    x->pushback = prts_commander_head_pushback(&e->commander, e->cworld, feet, to_feet(x, xr->head.position, room));
    *room_out = prts_v3_add(room, x->pushback);
    return feet;
}

int prts_engine_xr_views(prts_engine *e, double alpha, prts_view *out, int max)
{
    const prts_xr *xr = prts_xr_get();
    if (!xr->active || !e->has_commander || !xr->has_pose) return 0;
    prts_vec3 room;
    prts_dvec3 feet = anchor(e, xr, alpha, &room);
    prts_vec3 origin = prts_v3_add(room, prts_v3(0.0f, e->xr.seat, 0.0f));
    return prts_xr_build_views(xr, prts_dv3_add(feet, prts_dv3_from_v3(origin)), e->xr.turn, out, max);
}

bool prts_engine_xr_head(prts_engine *e, double alpha, prts_dvec3 *out)
{
    const prts_xr *xr = prts_xr_get();
    if (!xr->active || !e->has_commander || !xr->has_pose) return false;
    prts_vec3 room;
    prts_dvec3 feet = anchor(e, xr, alpha, &room);
    *out = prts_dv3_add(feet, prts_dv3_from_v3(to_feet(&e->xr, xr->head.position, room)));
    return true;
}

prts_render_settings prts_engine_xr_render_settings(const prts_engine *e)
{
    prts_render_settings rs = e->render; /* fog and the rest as on the flat screen */
    const prts_engine_xr *x = &e->xr;
    rs.preset = x->preset;
    rs.dither = x->dither;
    rs.snap = x->snap;
    rs.affine = x->affine;
    rs.crosshair = x->crosshair;
    rs.vignette = x->vignette_level;
    return rs;
}

static prts_vec3 v3_of(prts_dvec3 d) { return prts_v3((float)d.x, (float)d.y, (float)d.z); }

/* Each hand's pointer ray, out to what it hits within reach; the pointer hand's is bright (gold
 * on something usable), and while a teleport is aimed it ends at the target (green) or where
 * it hits (red). Returns the number of rays. */
static int pointer_rays(prts_engine *e, const prts_xr *xr, prts_dvec3 feet, prts_vec3 room, prts_gfx_ray *rays)
{
    prts_engine_xr *x = &e->xr;
    const prts_commander *c = &e->commander;
    const prts_xr_controller *ptr = pointer_controller(xr, x->hand);
    bool aiming = x->teleport && c->aiming;
    int n = 0;
    for (int h = 0; h < 2; h++) {
        const prts_xr_controller *ctl = prts_xr_hand_controller(xr, h ? PRTS_XR_HAND_RIGHT : PRTS_XR_HAND_LEFT);
        if (!ctl || !ctl->has_aim) continue;
        prts_gfx_ray *r = &rays[n++];
        r->from = prts_dv3_add(feet, prts_dv3_from_v3(to_feet(x, ctl->aim.position, room)));
        prts_vec3 dir = prts_v3_rotate_y(prts_xr_forward(&ctl->aim), x->turn);
        float reach = ctl == ptr && aiming ? PRTS_TELEPORT_RANGE : c->p.reach, dist = reach;
        if (prts_cworld_raycast(e->cworld, v3_of(r->from), dir, reach, &dist) == -1) dist = reach;
        r->to = prts_dv3_add(r->from, prts_dv3_from_v3(prts_v3_scale(dir, dist)));
        r->color = prts_v3_scale(RAY_IDLE, POINTER_DIM);
        if (ctl != ptr) continue;
        prts_dvec3 target;
        if (aiming) {
            bool ok = prts_commander_teleport_target(c, e->world, e->cworld, &target);
            if (ok) r->to = target;
            r->color = ok ? RAY_TELEPORT : RAY_BLOCKED;
        } else {
            r->color = prts_commander_target(c, e->world, e->cworld) != PRTS_ENTITY_NONE ? RAY_TARGET : RAY_IDLE;
        }
    }
    return n;
}

bool prts_engine_xr_render(prts_engine *e, double alpha)
{
    const prts_xr *xr = prts_xr_get();
    prts_view views[PRTS_XR_MAX_VIEWS];
    int n = prts_engine_xr_views(e, alpha, views, PRTS_XR_MAX_VIEWS);
    if (n == 0) return false;
    prts_vec3 room = prts_v3_lerp(e->xr.room_prev, e->xr.room, (float)alpha);
    room = prts_v3_add(room, e->xr.pushback);
    prts_gfx_ray rays[PRTS_GFX_MAX_RAYS];
    int count = pointer_rays(e, xr, prts_world_pos_interp(e->world, e->commander.entity, alpha), room, rays);
    prts_gfx_set_rays(e->gfx, rays, count);
    prts_render_settings rs = prts_engine_xr_render_settings(e); /* UI surfaces: render_frame drew them */
    prts_gfx_render_views(e->gfx, e->scene, e->world, views, n, &rs, alpha, e->time);
    prts_gfx_set_rays(e->gfx, NULL, 0);
    return true;
}

/* --- prts.xr ------------------------------------------------------------------------------ */

static const char *LOCOMOTION[] = {"smooth", "teleport"};
static const char *TURNING[] = {"smooth", "snap"};
static const char *PRESETS[] = {"authentic", "retro", "crisp", "hd", "native"};

static void opt_bool(lua_State *L, const char *key, bool *v)
{
    if (lua_getfield(L, 1, key) != LUA_TNIL) *v = lua_toboolean(L, -1);
    lua_pop(L, 1);
}

static void opt_number(lua_State *L, const char *key, float lo, float hi, float scale, float *v)
{
    if (lua_getfield(L, 1, key) != LUA_TNIL) {
        if (!lua_isnumber(L, -1)) luaL_error(L, "xr setting '%s' must be a number", key);
        *v = prts_clampf((float)lua_tonumber(L, -1), lo, hi) * scale;
    }
    lua_pop(L, 1);
}

/* An option that is one of two names: sets *v to whether it is the second. */
static void opt_choice(lua_State *L, const char *key, const char *names[2], bool *v)
{
    if (lua_getfield(L, 1, key) != LUA_TNIL) {
        const char *s = lua_tostring(L, -1);
        if (!s || (strcmp(s, names[0]) != 0 && strcmp(s, names[1]) != 0)) {
            luaL_error(L, "xr setting '%s' must be \"%s\" or \"%s\"", key, names[0], names[1]);
        }
        *v = strcmp(s, names[1]) == 0;
    }
    lua_pop(L, 1);
}

static void set_string(lua_State *L, const char *key, const char *v)
{
    lua_pushstring(L, v);
    lua_setfield(L, -2, key);
}

static void set_bool(lua_State *L, const char *key, bool v)
{
    lua_pushboolean(L, v);
    lua_setfield(L, -2, key);
}

static void set_number(lua_State *L, const char *key, double v)
{
    lua_pushnumber(L, v);
    lua_setfield(L, -2, key);
}

/* prts.xr.settings([t]): comfort options and the video settings used in XR; returns them. */
static int l_xr_settings(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_engine_xr *x = &e->xr;
    if (lua_istable(L, 1)) {
        opt_choice(L, "locomotion", LOCOMOTION, &x->teleport);
        opt_choice(L, "turn", TURNING, &x->snap_turn);
        opt_number(L, "snap_angle", 5.0f, 180.0f, DEG, &x->snap_angle);
        opt_number(L, "turn_speed", 10.0f, 720.0f, DEG, &x->turn_speed);
        opt_bool(L, "vignette", &x->vignette);
        opt_number(L, "vignette_strength", 0.0f, 1.0f, 1.0f, &x->vignette_strength);
        bool seated = x->seated;
        opt_bool(L, "seated", &x->seated);
        if (seated != x->seated) x->align_pending = prts_xr_get()->active; /* measure the seat again */
        if (lua_getfield(L, 1, "preset") != LUA_TNIL) {
            const char *p = lua_tostring(L, -1);
            if (!p || !prts_res_preset_parse(p, &x->preset)) return luaL_error(L, "unknown preset '%s'", p ? p : "?");
        }
        lua_pop(L, 1);
        opt_bool(L, "dither", &x->dither);
        opt_bool(L, "snap", &x->snap);
        opt_bool(L, "affine", &x->affine);
        opt_bool(L, "crosshair", &x->crosshair);
    }
    lua_createtable(L, 0, 12);
    set_string(L, "locomotion", LOCOMOTION[x->teleport]);
    set_string(L, "turn", TURNING[x->snap_turn]);
    set_number(L, "snap_angle", roundf(x->snap_angle / DEG));
    set_number(L, "turn_speed", roundf(x->turn_speed / DEG));
    set_bool(L, "vignette", x->vignette);
    set_number(L, "vignette_strength", x->vignette_strength);
    set_bool(L, "seated", x->seated);
    set_string(L, "preset", PRESETS[PRTS_CLAMP((int)x->preset, 0, 4)]);
    set_bool(L, "dither", x->dither);
    set_bool(L, "snap", x->snap);
    set_bool(L, "affine", x->affine);
    set_bool(L, "crosshair", x->crosshair);
    return 1;
}

/* prts.xr.state(): what the session is doing, for scripts and tests. */
static int l_xr_state(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const prts_engine_xr *x = &e->xr;
    const prts_xr *xr = prts_xr_get();
    lua_createtable(L, 0, 12);
    set_bool(L, "supported", xr->supported);
    set_bool(L, "active", xr->active);
    set_bool(L, "tracking", xr->active && xr->has_pose);
    set_number(L, "sessions", (double)xr->sessions);
    set_number(L, "views", xr->view_count);
    set_bool(L, "left", prts_xr_hand_controller(xr, PRTS_XR_HAND_LEFT) != NULL);
    set_bool(L, "right", prts_xr_hand_controller(xr, PRTS_XR_HAND_RIGHT) != NULL);
    set_string(L, "pointer", x->hand ? "right" : "left");
    set_number(L, "turn", x->turn);
    set_number(L, "seat", x->seat);
    set_number(L, "vignette", x->vignette_level);
    set_bool(L, "aiming", e->commander.aiming);
    set_number(L, "pushback", prts_v3_length(x->pushback));
    /* where the play area's origin is in the world (its floor, under the head when aligned) */
    prts_dvec3 o = e->has_commander ? prts_world_pos(e->world, e->commander.entity) : prts_dv3(0, 0, 0);
    o = prts_dv3_add(o, prts_dv3_from_v3(prts_v3_add(x->room, prts_v3(0.0f, x->seat, 0.0f))));
    lua_createtable(L, 3, 0);
    const double ov[3] = {o.x, o.y, o.z};
    for (int i = 0; i < 3; i++) {
        lua_pushnumber(L, ov[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "origin");
    return 1;
}

/* prts.xr.exit(): leaves VR (the menu). Returns whether a session was running. */
static int l_xr_exit(lua_State *L)
{
    prts_xr *xr = prts_xr_get();
    lua_pushboolean(L, xr->active);
    prts_xr_request_exit(xr);
    return 1;
}

void prts_engine_open_xr_api(prts_engine *e)
{
    static const prts_script_fn xr[] = {
        {"settings", l_xr_settings}, {"state", l_xr_state}, {"exit", l_xr_exit}, {NULL, NULL}};
    prts_script_register(e->script, "xr", xr);
}

/* --- scenario helpers: a simulated headset ------------------------------------------------- */

static float num_at(lua_State *L, int t, int i, float def)
{
    lua_rawgeti(L, t, i);
    float v = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : def;
    lua_pop(L, 1);
    return v;
}

static float num_field(lua_State *L, int t, const char *key, float def)
{
    lua_getfield(L, t, key);
    float v = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : def;
    lua_pop(L, 1);
    return v;
}

/* Reads field `key` of table t as {x, y, z} into v (unchanged if absent). */
static bool vec_field(lua_State *L, int t, const char *key, float v[3])
{
    bool ok = lua_getfield(L, t, key) == LUA_TTABLE;
    for (int i = 0; ok && i < 3; i++) v[i] = num_at(L, lua_gettop(L), i + 1, v[i]);
    lua_pop(L, 1);
    return ok;
}

static void quat_mul(const float a[4], const float b[4], float out[4])
{
    out[0] = a[3] * b[0] + b[3] * a[0] + a[1] * b[2] - a[2] * b[1];
    out[1] = a[3] * b[1] + b[3] * a[1] + a[2] * b[0] - a[0] * b[2];
    out[2] = a[3] * b[2] + b[3] * a[2] + a[0] * b[1] - a[1] * b[0];
    out[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
}

/* A pose table { pos = {x, y, z}, quat = {x, y, z, w} | dir = {x, y, z} | yaw =, pitch = }
 * (radians, the view convention) written as 7 floats. */
static void pack_pose(lua_State *L, int t, float *d)
{
    float pos[3] = {0, 0, 0}, dir[3] = {0, 0, -1}, q[4] = {0, 0, 0, 1};
    vec_field(L, t, "pos", pos);
    float yaw = num_field(L, t, "yaw", 0.0f), pitch = num_field(L, t, "pitch", 0.0f);
    if (vec_field(L, t, "dir", dir)) {
        yaw = atan2f(-dir[0], -dir[2]);
        pitch = atan2f(dir[1], sqrtf(dir[0] * dir[0] + dir[2] * dir[2]));
    }
    float qy[4] = {0, sinf(yaw * 0.5f), 0, cosf(yaw * 0.5f)}, qx[4] = {sinf(pitch * 0.5f), 0, 0, cosf(pitch * 0.5f)};
    quat_mul(qy, qx, q);
    if (lua_getfield(L, t, "quat") == LUA_TTABLE) {
        for (int i = 0; i < 4; i++) q[i] = num_at(L, lua_gettop(L), i + 1, q[i]);
    }
    lua_pop(L, 1);
    memcpy(d, pos, sizeof(pos));
    memcpy(d + 3, q, sizeof(q));
}

/* Two eyes 64 mm apart on the head pose, 90 degrees square, side by side in the framebuffer. */
static void pack_views(const prts_xr *xr, const float *head, float *d)
{
    prts_xr_pose pose;
    memcpy(pose.position, head, sizeof(pose.position));
    memcpy(pose.orientation, head + 3, sizeof(pose.orientation));
    prts_vec3 axis[3] = {prts_xr_rotate(&pose, prts_v3(1, 0, 0)), prts_xr_rotate(&pose, prts_v3(0, 1, 0)),
                         prts_xr_rotate(&pose, prts_v3(0, 0, 1))};
    float n = 0.05f, f = 1000.0f;
    for (int v = 0; v < PRTS_XR_MAX_VIEWS; v++) {
        float *m = d + PRTS_XR_F_VIEWS + v * PRTS_XR_F_VIEW_SIZE;
        prts_vec3 eye = prts_v3_add(prts_v3(head[0], head[1], head[2]), prts_v3_scale(axis[0], v ? 0.032f : -0.032f));
        for (int i = 0; i < 3; i++) { /* the inverse of the eye's rigid transform */
            const float *r = &axis[i].x;
            for (int j = 0; j < 3; j++) m[j * 4 + i] = r[j];
            m[12 + i] = -prts_v3_dot(axis[i], eye);
        }
        m[15] = 1.0f;
        float *p = m + 16;
        p[0] = p[5] = 1.0f;
        p[10] = -(f + n) / (f - n);
        p[11] = -1.0f;
        p[14] = -2.0f * f * n / (f - n);
        int w = PRTS_MAX(2, xr->fb_width) / 2;
        float vp[4] = {(float)(v * w), 0.0f, (float)w, (float)PRTS_MAX(1, xr->fb_height)};
        memcpy(m + 32, vp, sizeof(vp));
    }
}

/* A hand: { aim = pose, grip = pose, trigger, squeeze, buttons = { b0, b1, ... },
 * stick = { x, y }, axes = { a0, a1, a2, a3 } }; buttons count as pressed from 0.5. */
static void pack_hand(lua_State *L, int t, prts_xr_hand hand, float *c)
{
    c[0] = 1.0f;
    c[1] = (float)hand;
    if (lua_getfield(L, t, "grip") == LUA_TTABLE) {
        c[2] = 1.0f;
        pack_pose(L, lua_gettop(L), c + 3);
    }
    lua_pop(L, 1);
    if (lua_getfield(L, t, "aim") == LUA_TTABLE) {
        c[10] = 1.0f;
        pack_pose(L, lua_gettop(L), c + 11);
    }
    lua_pop(L, 1);
    float b[PRTS_XR_MAX_BUTTONS] = {0}, a[PRTS_XR_MAX_AXES] = {0};
    if (lua_getfield(L, t, "buttons") == LUA_TTABLE) {
        for (int i = 0; i < PRTS_XR_MAX_BUTTONS; i++) b[i] = num_at(L, lua_gettop(L), i + 1, 0.0f);
    }
    lua_pop(L, 1);
    if (lua_getfield(L, t, "axes") == LUA_TTABLE) {
        for (int i = 0; i < PRTS_XR_MAX_AXES; i++) a[i] = num_at(L, lua_gettop(L), i + 1, 0.0f);
    }
    lua_pop(L, 1);
    b[0] = num_field(L, t, "trigger", b[0]);
    b[1] = num_field(L, t, "squeeze", b[1]);
    if (lua_getfield(L, t, "stick") == LUA_TTABLE) {
        a[2] = num_at(L, lua_gettop(L), 1, 0.0f);
        a[3] = num_at(L, lua_gettop(L), 2, 0.0f);
    }
    lua_pop(L, 1);
    unsigned mask = 0;
    for (int i = 0; i < PRTS_XR_MAX_BUTTONS; i++) {
        c[20 + i] = b[i];
        if (b[i] >= 0.5f) mask |= 1u << i;
    }
    c[18] = (float)PRTS_XR_MAX_BUTTONS;
    c[19] = (float)mask;
    c[28] = (float)PRTS_XR_MAX_AXES;
    memcpy(c + 29, a, sizeof(a));
}

/* t.xr_begin([width, height]): a simulated headset starts a session. */
static int t_xr_begin(lua_State *L)
{
    prts_xr *xr = prts_xr_get();
    prts_xr_set_supported(xr, true);
    prts_xr_begin(xr, 0, (int)luaL_optinteger(L, 1, 2000), (int)luaL_optinteger(L, 2, 1000));
    return 0;
}

/* t.xr_frame{ head = pose, left = hand, right = hand, lost = bool }: the next XR frame's poses
 * (kept until the next call), packed as web/prts_xr.js packs them. */
static int t_xr_frame(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_xr *xr = prts_xr_get();
    if (!xr->active) return luaL_error(L, "no XR session (t.xr_begin)");
    float d[PRTS_XR_FRAME_FLOATS];
    memset(d, 0, sizeof(d));
    lua_getfield(L, 1, "lost");
    bool lost = lua_toboolean(L, -1);
    lua_pop(L, 1);
    if (!lost) {
        d[PRTS_XR_F_HAS_POSE] = 1.0f;
        d[PRTS_XR_F_VIEW_COUNT] = (float)PRTS_XR_MAX_VIEWS;
        float *head = d + PRTS_XR_F_HEAD;
        head[1] = 1.6f;
        head[6] = 1.0f;
        if (lua_getfield(L, 1, "head") == LUA_TTABLE) {
            head[1] = 0.0f;
            pack_pose(L, lua_gettop(L), head);
        }
        lua_pop(L, 1);
        pack_views(xr, head, d);
    }
    static const char *hands[2] = {"left", "right"};
    int slot = 0;
    for (int h = 0; h < 2; h++) {
        if (lua_getfield(L, 1, hands[h]) == LUA_TTABLE) {
            float *c = d + PRTS_XR_F_CONTROLLERS + slot++ * PRTS_XR_F_CONTROLLER_SIZE;
            pack_hand(L, lua_gettop(L), h ? PRTS_XR_HAND_RIGHT : PRTS_XR_HAND_LEFT, c);
        }
        lua_pop(L, 1);
    }
    prts_xr_apply_frame(xr, d, PRTS_XR_FRAME_FLOATS);
    return 0;
}

/* t.xr_end(): the headset ends the session. */
static int t_xr_end(lua_State *L)
{
    (void)L;
    prts_xr_end(prts_xr_get());
    return 0;
}

/* t.xr_view() -> x, y, z: where the head's view is drawn from (after the wall push-back). */
static int t_xr_view(lua_State *L)
{
    prts_dvec3 p;
    if (!prts_engine_xr_head(prts_engine_from_lua(L), 1.0, &p)) return 0;
    lua_pushnumber(L, p.x);
    lua_pushnumber(L, p.y);
    lua_pushnumber(L, p.z);
    return 3;
}

void prts_engine_open_test_xr(prts_engine *e, int table)
{
    /* api-docs: t */
    static const luaL_Reg fns[] = {
        {"xr_begin", t_xr_begin}, {"xr_frame", t_xr_frame}, {"xr_end", t_xr_end}, {"xr_view", t_xr_view}, {NULL, NULL}};
    lua_State *L = prts_script_lua(e->script);
    lua_pushvalue(L, table);
    luaL_setfuncs(L, fns, 0);
    lua_pop(L, 1);
}
