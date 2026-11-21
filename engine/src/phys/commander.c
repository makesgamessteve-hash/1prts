/* 1stPRTS engine: the commander's first-person body. MIT License. */
#include "prts/prts_commander.h"

#include <math.h>

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

void prts_commander_tick(prts_commander *c, prts_world *w, const prts_cworld *cw, const prts_input *in, float dt)
{
    c->eye_prev = c->eye;
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

    if (c->on_ground && prts_input_pressed(in, "jump") && !c->crouched) c->vertical_speed = c->p.jump_speed;
    c->vertical_speed -= c->p.gravity * dt;
    if (c->on_ground && c->vertical_speed < -1.0f) c->vertical_speed = -1.0f;

    prts_vec3 delta = prts_v3(dir.x * speed * dt, c->vertical_speed * dt, dir.z * speed * dt);
    prts_aabb box = prts_commander_box(c, w);
    prts_move_result res;
    prts_cworld_move(cw, &box, delta, c->on_ground ? c->p.step_height : 0.0f, &res);
    c->on_ground = res.on_ground;
    c->hit_wall = res.hit_wall;
    if (res.on_ground && c->vertical_speed < 0.0f) c->vertical_speed = 0.0f;
    if (!res.on_ground && delta.y > 0.0f && box.min.y - prts_commander_box(c, w).min.y < delta.y - 1e-4f) {
        c->vertical_speed = 0.0f; /* bumped the head */
    }

    prts_dvec3 feet = prts_dv3((box.min.x + box.max.x) * 0.5, box.min.y, (box.min.z + box.max.z) * 0.5);
    /* ride whatever we stand on: its frame becomes our parent */
    prts_entity ground = res.on_ground ? prts_cworld_owner(cw, res.ground_box) : PRTS_ENTITY_NONE;
    if (res.on_ground) prts_world_set_parent(w, c->entity, ground);
    prts_world_set_pos(w, c->entity, feet);
    prts_world_set_velocity(w, c->entity, prts_v3(dir.x * speed, c->vertical_speed, dir.z * speed));
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
    float dist = 0.0f;
    int id = prts_cworld_raycast(cw, o, prts_view_forward(c->yaw, c->pitch), c->p.reach, &dist);
    prts_entity e = prts_cworld_owner(cw, id);
    return prts_world_usable(w, e) ? e : PRTS_ENTITY_NONE;
}
