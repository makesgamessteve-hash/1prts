/* 1stPRTS engine: the commander's first-person body. MIT License. */
#include "prts/prts_commander.h"

#include <math.h>
#include <string.h>

#define FOLLOW_MAX 1.0f   /* meters per tick the body follows the head (a recentre jumps further) */
#define HEAD_RADIUS 0.12f /* half size of the head's box: keeps the near plane out of walls */

/* view feel */
#define LAND_MIN 2.0f        /* m/s: softer landings do not dip the view */
#define DIP_PER_SPEED 0.55f  /* dip kick (m/s) per m/s of impact: 4 cm for a jump, 9 for a fall */
#define DIP_MAX_IMPACT 10.0f /* m/s */
#define JUMP_DIP 0.25f       /* m/s */
#define DIP_K 160.0f         /* spring stiffness (1/s^2) */
#define DIP_DAMP 17.7f       /* spring damping (1/s), a little under critical */
#define BOB_AMP 0.028f       /* m at running speed */
#define STEP_LENGTH 0.75f    /* m per step at walking pace; longer when faster */
#define TRAUMA_DECAY 1.4f    /* per second */
#define SHAKE_Y 0.02f        /* m at full trauma */
#define SHAKE_PITCH 0.03f    /* rad at full trauma: 2.5 % of a 70 degree view */

void prts_commander_params_default(prts_commander_params *p)
{
    p->walk_speed = 1.4f;
    p->run_speed = 3.5f;
    p->sprint_speed = 6.0f;
    p->crouch_speed = 1.0f;
    p->jump_speed = 4.2f;
    p->gravity = 9.81f;
    p->eye_stand = 1.70f;
    p->eye_crouch = 1.10f;
    p->height_stand = 1.85f;
    p->height_crouch = 1.25f;
    p->half_width = 0.3f;
    p->step_height = 0.4f;
    p->stamina_max = 10.0f;
    p->stamina_drain = 1.0f;
    p->stamina_regen = 0.5f;
    p->reach = 2.5f;
    p->look_sensitivity = 0.0025f;
}

void prts_commander_init(prts_commander *c, prts_entity e, const prts_commander_params *p)
{
    c->entity = e;
    c->p = *p;
    c->yaw = c->pitch = 0.0f;
    c->vertical_speed = 0.0f;
    c->on_ground = c->crouched = c->hit_wall = false;
    c->eye = c->eye_prev = p->eye_stand;
    c->stamina = p->stamina_max;
    memset(&c->xr, 0, sizeof(c->xr));
    c->followed = prts_v3(0, 0, 0);
    c->aiming = false;
    c->vel = prts_v3(0, 0, 0);
    c->jump_buffer = c->coyote = 0;
    c->jumping = false;
    memset(&c->feel, 0, sizeof(c->feel));
    c->feel.motion = 1.0f;
}

void prts_commander_impulse(prts_commander *c, float dip, float shake)
{
    c->feel.dip_vel -= dip;
    c->feel.trauma = prts_clampf(c->feel.trauma + shake, 0.0f, 1.0f);
}

float prts_commander_view_offset(const prts_commander *c, double alpha, float *pitch)
{
    const prts_commander_feel *f = &c->feel;
    if (pitch) *pitch = (float)(f->pitch_prev + (f->pitch - f->pitch_prev) * alpha);
    return (float)(f->offset_prev + (f->offset - f->offset_prev) * alpha);
}

/* The view's feedback motion for this tick: the dip spring, head bob with a footfall every
 * step, and the shake. Presentation only (see prts_commander_feel). */
static void feel_tick(prts_commander *c, float impact, float speed, float dt)
{
    prts_commander_feel *f = &c->feel;
    f->offset_prev = f->offset;
    f->pitch_prev = f->pitch;
    f->clock++;
    f->landed = impact;
    if (impact > LAND_MIN) f->dip_vel -= fminf(impact, DIP_MAX_IMPACT) * DIP_PER_SPEED;
    if (f->jumped) f->dip_vel -= JUMP_DIP;
    f->dip_vel += (-DIP_K * f->dip - DIP_DAMP * f->dip_vel) * dt;
    f->dip += f->dip_vel * dt;
    float want = c->on_ground ? BOB_AMP * prts_clampf(speed / 3.5f, 0.0f, 1.2f) : 0.0f;
    f->bob_amp += (want - f->bob_amp) * fminf(1.0f, 10.0f * dt);
    if (c->on_ground && speed > 0.3f) {
        /* the head is lowest when a foot comes down: at phase pi/2 + k pi */
        float step = STEP_LENGTH + 0.12f * speed;
        float before = floorf((f->bob_phase + (float)PRTS_PI * 0.5f) / (float)PRTS_PI);
        f->bob_phase += speed * dt * (float)PRTS_PI / step;
        if (floorf((f->bob_phase + (float)PRTS_PI * 0.5f) / (float)PRTS_PI) != before) f->steps++;
        if (f->bob_phase > 2.0f * (float)PRTS_PI) f->bob_phase -= 2.0f * (float)PRTS_PI;
    }
    f->trauma = fmaxf(0.0f, f->trauma - TRAUMA_DECAY * dt);
    float shake = f->trauma * f->trauma;
    float t = (float)f->clock;
    float n1 = 0.6f * sinf(t * 2.1f) + 0.4f * sinf(t * 3.7f + 1.0f);
    float n2 = 0.6f * sinf(t * 1.7f + 2.0f) + 0.4f * sinf(t * 4.3f);
    float scale = c->xr.active ? 0.0f : prts_clampf(f->motion, 0.0f, 1.0f);
    f->offset = scale * (f->dip - f->bob_amp * fabsf(sinf(f->bob_phase)) + SHAKE_Y * shake * n1);
    f->pitch = scale * SHAKE_PITCH * shake * n2;
}

void prts_commander_look(prts_commander *c, float dyaw, float dpitch)
{
    c->yaw = prts_wrap_angle(c->yaw + dyaw);
    c->pitch = prts_clampf(c->pitch + dpitch, -1.48f, 1.48f);
}

static float height_of(const prts_commander *c) { return c->crouched ? c->p.height_crouch : c->p.height_stand; }

prts_aabb prts_commander_box(const prts_commander *c, const prts_world *w)
{
    prts_dvec3 f = prts_world_pos(w, c->entity);
    float hw = c->p.half_width;
    prts_aabb b = {prts_v3((float)f.x - hw, (float)f.y, (float)f.z - hw),
                   prts_v3((float)f.x + hw, (float)f.y + height_of(c), (float)f.z + hw)};
    return b;
}

static prts_vec3 feet_of(const prts_aabb *box)
{
    return prts_v3((box->min.x + box->max.x) * 0.5f, box->min.y, (box->min.z + box->max.z) * 0.5f);
}

/* XR: the body steps under the tracked head, so walking in the play area walks the commander
 * and walls stop the body. c->followed tells the engine how far, to move the play area back. */
static void follow_head(prts_commander *c, prts_world *w, const prts_cworld *cw)
{
    c->followed = prts_v3(0, 0, 0);
    prts_vec3 d = prts_v3(c->xr.head.x, 0.0f, c->xr.head.z);
    float len = prts_v3_length(d);
    if (!c->xr.active || !(len > 1e-3f)) return;
    if (len > FOLLOW_MAX) d = prts_v3_scale(d, FOLLOW_MAX / len);
    prts_aabb box = prts_commander_box(c, w);
    prts_vec3 moved = prts_cworld_move(cw, &box, d, c->on_ground ? c->p.step_height : 0.0f, NULL);
    c->followed = prts_v3(moved.x, 0.0f, moved.z);
    prts_world_set_pos(w, c->entity, prts_dv3_from_v3(feet_of(&box)));
}

/* Teleport locomotion: aim while "teleport" is held, go when it is let go. */
static void teleport(prts_commander *c, prts_world *w, const prts_cworld *cw, const prts_input *in)
{
    bool was = c->aiming;
    c->aiming = c->xr.active && c->xr.teleport && prts_input_down(in, "teleport");
    prts_dvec3 target;
    if (!was || c->aiming || !prts_commander_teleport_target(c, w, cw, &target)) return;
    prts_world_set_parent(w, c->entity, PRTS_ENTITY_NONE);
    prts_world_set_pos(w, c->entity, target);
    prts_world_skip_interp(w, c->entity); /* no slide: the view is there at once */
    c->vertical_speed = 0.0f;
}

void prts_commander_tick(prts_commander *c, prts_world *w, const prts_cworld *cw, const prts_input *in, float dt)
{
    c->eye_prev = c->eye;
    follow_head(c, w, cw);
    teleport(c, w, cw, in);
    /* crouch: stand up only if there is room */
    bool want_crouch = prts_input_down(in, "crouch");
    if (want_crouch) {
        c->crouched = true;
    } else if (c->crouched) {
        c->crouched = false;
        prts_aabb tall = prts_commander_box(c, w);
        tall.min.y += 0.05f;
        if (prts_cworld_overlaps(cw, tall)) c->crouched = true;
    }
    float target_eye = c->crouched ? c->p.eye_crouch : c->p.eye_stand;
    float eye_step = 3.0f * dt;
    if (fabsf(target_eye - c->eye) <= eye_step) c->eye = target_eye;
    else c->eye += target_eye > c->eye ? eye_step : -eye_step;

    /* desired horizontal motion in the view's yaw frame */
    float fwd = prts_input_value(in, "move_forward") - prts_input_value(in, "move_back");
    float side = prts_input_value(in, "move_right") - prts_input_value(in, "move_left");
    float len = sqrtf(fwd * fwd + side * side);
    if (len > 1.0f) {
        fwd /= len;
        side /= len;
    }
    if (c->xr.active && c->xr.teleport) fwd = side = len = 0.0f; /* the stick aims teleports */
    bool moving = len > 0.01f;
    float speed = c->p.walk_speed;
    if (c->crouched) {
        speed = c->p.crouch_speed;
    } else if (prts_input_down(in, "sprint") && c->stamina > 0.0f && moving) {
        speed = c->p.sprint_speed;
        c->stamina = fmaxf(0.0f, c->stamina - c->p.stamina_drain * dt);
    } else if (prts_input_down(in, "run")) {
        speed = c->p.run_speed;
    }
    if (speed != c->p.sprint_speed) c->stamina = fminf(c->p.stamina_max, c->stamina + c->p.stamina_regen * dt);

    float world_yaw = c->yaw; /* the view yaw is kept in world space */
    float sy = sinf(world_yaw), cy = cosf(world_yaw);
    prts_vec3 dir = prts_v3(-sy * fwd + cy * side, 0.0f, -cy * fwd - sy * side);

    /* jump: a press is kept a few ticks until there is ground (input buffer), and walking off
     * an edge leaves a few ticks to jump (coyote time) */
    c->feel.jumped = false;
    if (prts_input_pressed(in, "jump")) c->jump_buffer = PRTS_COMMANDER_JUMP_BUFFER;
    if (c->jump_buffer > 0 && (c->on_ground || c->coyote > 0) && !c->crouched) {
        c->vertical_speed = c->p.jump_speed;
        c->jump_buffer = c->coyote = 0;
        c->feel.jumped = true;
        c->jumping = true;
    } else if (c->jump_buffer > 0) {
        c->jump_buffer--;
    }
    float gravity = c->p.gravity;
    /* hang time at the top of a jump (only a jump: falling off an edge keeps full gravity) */
    if (c->jumping && !c->on_ground && fabsf(c->vertical_speed) < PRTS_COMMANDER_APEX_SPEED) {
        gravity *= PRTS_COMMANDER_APEX_GRAVITY;
    }
    c->vertical_speed -= gravity * dt;
    if (c->on_ground && c->vertical_speed < -1.0f) c->vertical_speed = -1.0f;

    /* horizontal velocity eases towards the wanted one (at once in XR: comfort) */
    prts_vec3 want = prts_v3(dir.x * speed, 0.0f, dir.z * speed);
    bool eased = !c->xr.active;
    if (eased) {
        float rate = (c->on_ground ? (moving ? PRTS_COMMANDER_ACCEL : PRTS_COMMANDER_DECEL) : PRTS_COMMANDER_AIR_ACCEL) * dt;
        prts_vec3 d = prts_v3_sub(want, c->vel);
        float gap = prts_v3_length(d);
        c->vel = gap <= rate ? want : prts_v3_add(c->vel, prts_v3_scale(d, rate / gap));
    } else {
        c->vel = want;
    }

    bool was_on_ground = c->on_ground;
    float falling = c->vertical_speed;
    prts_vec3 delta = prts_v3(c->vel.x * dt, c->vertical_speed * dt, c->vel.z * dt);
    prts_aabb box = prts_commander_box(c, w);
    prts_move_result res;
    prts_vec3 moved = prts_cworld_move(cw, &box, delta, c->on_ground ? c->p.step_height : 0.0f, &res);
    c->on_ground = res.on_ground;
    /* a wall takes the speed into it away (sliding along it keeps the rest) */
    if (eased && res.hit_wall) c->vel = prts_v3(moved.x / dt, 0.0f, moved.z / dt);
    if (res.on_ground && !c->feel.jumped) c->jumping = false;
    if (res.on_ground) c->coyote = PRTS_COMMANDER_COYOTE;
    else if (c->coyote > 0 && !c->feel.jumped) c->coyote--;
    else c->coyote = 0;
    c->hit_wall = res.hit_wall;
    if (res.on_ground && c->vertical_speed < 0.0f) c->vertical_speed = 0.0f;
    if (!res.on_ground && delta.y > 0.0f && box.min.y - prts_commander_box(c, w).min.y < delta.y - 1e-4f) {
        c->vertical_speed = 0.0f; /* bumped the head */
    }

    prts_dvec3 feet = prts_dv3_from_v3(feet_of(&box));
    /* ride whatever we stand on: its frame becomes our parent */
    prts_entity ground = res.on_ground ? prts_cworld_owner(cw, res.ground_box) : PRTS_ENTITY_NONE;
    if (res.on_ground) prts_world_set_parent(w, c->entity, ground);
    prts_world_set_pos(w, c->entity, feet);
    prts_world_set_velocity(w, c->entity, prts_v3(c->vel.x, c->vertical_speed, c->vel.z));
    float impact = !was_on_ground && res.on_ground && falling < 0.0f ? -falling : 0.0f;
    feel_tick(c, impact, sqrtf(c->vel.x * c->vel.x + c->vel.z * c->vel.z), dt);
}

prts_dvec3 prts_commander_eye(const prts_commander *c, const prts_world *w, double alpha)
{
    prts_dvec3 f = prts_world_pos_interp(w, c->entity, alpha);
    double eye = (double)c->eye_prev + ((double)c->eye - (double)c->eye_prev) * alpha;
    return prts_dv3(f.x, f.y + eye, f.z);
}

prts_entity prts_commander_target(const prts_commander *c, const prts_world *w, const prts_cworld *cw)
{
    prts_dvec3 eye = prts_commander_eye(c, w, 1.0);
    prts_vec3 o = prts_v3((float)eye.x, (float)eye.y, (float)eye.z);
    prts_vec3 dir = prts_view_forward(c->yaw, c->pitch);
    if (c->xr.active && c->xr.has_pointer) {
        prts_dvec3 f = prts_world_pos(w, c->entity);
        prts_vec3 hand = prts_v3_add(prts_v3((float)f.x, (float)f.y, (float)f.z), c->xr.pointer_origin);
        prts_vec3 arm = prts_v3_sub(hand, o);
        float len = prts_v3_length(arm);
        if (len > 1e-3f && prts_cworld_occluded(cw, o, arm, len)) return PRTS_ENTITY_NONE; /* hand in a wall */
        o = hand;
        dir = c->xr.pointer_dir;
    }
    float dist = 0.0f;
    int id = prts_cworld_raycast_use(cw, o, dir, c->p.reach, &dist);
    prts_entity e = prts_cworld_owner(cw, id);
    return prts_world_usable(w, e) ? e : PRTS_ENTITY_NONE;
}

bool prts_commander_teleport_target(const prts_commander *c, const prts_world *w, const prts_cworld *cw,
                                    prts_dvec3 *target)
{
    prts_vec3 d = prts_v3_normalize(c->xr.pointer_dir);
    if (!c->xr.active || !c->xr.has_pointer || d.y > -0.02f) return false; /* aim down at the floor */
    prts_dvec3 fd = prts_world_pos(w, c->entity);
    prts_vec3 feet = prts_v3((float)fd.x, (float)fd.y, (float)fd.z);
    prts_vec3 o = prts_v3_add(feet, c->xr.pointer_origin);
    float dist = 0.0f;
    if (prts_cworld_raycast(cw, o, d, PRTS_TELEPORT_RANGE, &dist) == -1) return false;
    /* walkable floor: just above the hit (stepped back off a wall face) the ground is right
     * below; for a wall face it is much further down */
    prts_vec3 hit = prts_v3_add(o, prts_v3_scale(d, dist));
    prts_vec3 back = prts_v3_normalize(prts_v3(d.x, 0.0f, d.z));
    prts_vec3 probe = prts_v3(hit.x - back.x * 0.05f, hit.y + 0.3f, hit.z - back.z * 0.05f);
    float ground = 0.0f;
    if (!prts_cworld_ground(cw, probe, 0.6f, &ground) || probe.y - ground > 0.4f) return false;
    prts_vec3 t = prts_v3(probe.x, ground, probe.z);
    /* room to stand */
    float hw = c->p.half_width;
    prts_aabb stand = {prts_v3(t.x - hw, t.y + 0.05f, t.z - hw), prts_v3(t.x + hw, t.y + c->p.height_stand, t.z + hw)};
    if (prts_cworld_overlaps(cw, stand)) return false;
    /* no wall between: a level ray 1 m above the higher floor */
    float y = fmaxf(feet.y, t.y) + 1.0f;
    prts_vec3 across = prts_v3(t.x - feet.x, 0.0f, t.z - feet.z);
    float len = prts_v3_length(across);
    if (len > 0.01f && prts_cworld_occluded(cw, prts_v3(feet.x, y, feet.z), across, len)) return false;
    *target = prts_dv3_from_v3(t);
    return true;
}

prts_vec3 prts_commander_head_pushback(const prts_commander *c, const prts_cworld *cw, prts_dvec3 feet,
                                       prts_vec3 head)
{
    float lo = 0.3f + HEAD_RADIUS, hi = height_of(c) - HEAD_RADIUS;
    prts_vec3 f = prts_v3((float)feet.x, (float)feet.y, (float)feet.z);
    prts_vec3 start = prts_v3(f.x, f.y + prts_clampf(head.y, lo, hi), f.z);
    prts_vec3 want = prts_v3_add(f, head);
    prts_vec3 r = prts_v3(HEAD_RADIUS, HEAD_RADIUS, HEAD_RADIUS);
    prts_aabb box = {prts_v3_sub(start, r), prts_v3_add(start, r)};
    prts_cworld_move(cw, &box, prts_v3_sub(want, start), 0.0f, NULL);
    prts_vec3 got = prts_v3_scale(prts_v3_add(box.min, box.max), 0.5f);
    return prts_v3_sub(got, want);
}
