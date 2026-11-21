/* 1stPRTS engine: fixed-tick clock, entities and nested reference frames. MIT License. */
#include "prts/prts_world.h"

#include <math.h>
#include <string.h>

#include "prts/prts_rng.h"

void prts_clock_init(prts_clock *c, int ticks_per_second, int max_catchup)
{
    memset(c, 0, sizeof(*c));
    c->tick_seconds = 1.0 / (double)ticks_per_second;
    c->max_catchup = max_catchup;
}

int prts_clock_advance(prts_clock *c, double real_seconds)
{
    if (real_seconds > 0.0) c->accumulator += real_seconds;
    int n = 0;
    while (c->accumulator >= c->tick_seconds && n < c->max_catchup) {
        c->accumulator -= c->tick_seconds;
        n++;
    }
    if (c->accumulator >= c->tick_seconds) c->accumulator = fmod(c->accumulator, c->tick_seconds);
    return n;
}

double prts_clock_alpha(const prts_clock *c) { return c->accumulator / c->tick_seconds; }

typedef struct slot {
    uint32_t gen;
    bool alive;
    bool usable;
    char *name;
    prts_entity parent;
    prts_dvec3 local;
    float yaw;
    prts_vec3 vel;
    prts_dvec3 prev_pos;
    float prev_yaw;
    bool moving_pos, moving_yaw;
    prts_dvec3 target;
    float speed;
    float yaw_target;
    float yaw_speed;
} slot;

struct prts_world {
    prts_array slots; /* slot */
    size_t alive;
};

static uint32_t index_of(prts_entity e) { return (uint32_t)(e & 0xffffffffu) - 1u; }
static uint32_t gen_of(prts_entity e) { return (uint32_t)(e >> 32); }
static prts_entity make_entity(uint32_t index, uint32_t gen) { return ((prts_entity)gen << 32) | (index + 1u); }

static slot *get(const prts_world *w, prts_entity e)
{
    if (e == PRTS_ENTITY_NONE) return NULL;
    slot *s = prts_array_at(&w->slots, index_of(e));
    return (s && s->alive && s->gen == gen_of(e)) ? s : NULL;
}

prts_world *prts_world_create(void)
{
    prts_world *w = prts_calloc(1, sizeof(*w));
    prts_array_init(&w->slots, sizeof(slot));
    return w;
}

void prts_world_destroy(prts_world *w)
{
    if (!w) return;
    for (size_t i = 0; i < w->slots.len; i++) prts_free(((slot *)prts_array_at(&w->slots, i))->name);
    prts_array_free(&w->slots);
    prts_free(w);
}

prts_entity prts_world_spawn(prts_world *w, const char *name)
{
    uint32_t index = (uint32_t)w->slots.len;
    for (uint32_t i = 0; i < w->slots.len; i++) {
        if (!((slot *)prts_array_at(&w->slots, i))->alive) {
            index = i;
            break;
        }
    }
    slot *s = index == w->slots.len ? prts_array_push(&w->slots, NULL) : prts_array_at(&w->slots, index);
    uint32_t gen = s->gen + 1u;
    memset(s, 0, sizeof(*s));
    s->gen = gen;
    s->alive = true;
    s->name = name ? prts_strdup(name) : NULL;
    w->alive++;
    return make_entity(index, gen);
}

bool prts_world_alive(const prts_world *w, prts_entity e) { return get(w, e) != NULL; }

void prts_world_despawn(prts_world *w, prts_entity e)
{
    slot *s = get(w, e);
    if (!s) return;
    s->alive = false;
    prts_free(s->name);
    s->name = NULL;
    w->alive--;
    for (uint32_t i = 0; i < w->slots.len; i++) {
        slot *c = prts_array_at(&w->slots, i);
        if (c->alive && c->parent == e) prts_world_despawn(w, make_entity(i, c->gen));
    }
}

size_t prts_world_count(const prts_world *w) { return w->alive; }

const char *prts_world_name(const prts_world *w, prts_entity e)
{
    slot *s = get(w, e);
    return s && s->name ? s->name : "";
}

prts_entity prts_world_find(const prts_world *w, const char *name)
{
    for (uint32_t i = 0; i < w->slots.len; i++) {
        slot *s = prts_array_at(&w->slots, i);
        if (s->alive && s->name && strcmp(s->name, name) == 0) return make_entity(i, s->gen);
    }
    return PRTS_ENTITY_NONE;
}

void prts_world_each(const prts_world *w, prts_world_each_fn fn, void *user)
{
    for (uint32_t i = 0; i < w->slots.len; i++) {
        slot *s = prts_array_at(&w->slots, i);
        if (s->alive) fn(make_entity(i, s->gen), user);
    }
}

void prts_world_set_local(prts_world *w, prts_entity e, prts_dvec3 pos, float yaw)
{
    slot *s = get(w, e);
    if (!s) return;
    s->local = pos;
    s->yaw = yaw;
}

prts_dvec3 prts_world_local_pos(const prts_world *w, prts_entity e)
{
    slot *s = get(w, e);
    return s ? s->local : prts_dv3(0, 0, 0);
}

float prts_world_local_yaw(const prts_world *w, prts_entity e)
{
    slot *s = get(w, e);
    return s ? s->yaw : 0.0f;
}

static prts_dvec3 rotate_y_d(prts_dvec3 v, double yaw)
{
    double c = cos(yaw), sn = sin(yaw);
    return prts_dv3(v.x * c + v.z * sn, v.y, -v.x * sn + v.z * c);
}

float prts_world_yaw(const prts_world *w, prts_entity e)
{
    slot *s = get(w, e);
    if (!s) return 0.0f;
    return s->yaw + prts_world_yaw(w, s->parent);
}

prts_dvec3 prts_world_pos(const prts_world *w, prts_entity e)
{
    slot *s = get(w, e);
    if (!s) return prts_dv3(0, 0, 0);
    if (!get(w, s->parent)) return s->local;
    prts_dvec3 ppos = prts_world_pos(w, s->parent);
    return prts_dv3_add(ppos, rotate_y_d(s->local, prts_world_yaw(w, s->parent)));
}

static prts_dvec3 to_local(const prts_world *w, prts_entity parent, prts_dvec3 world_pos)
{
    if (!get(w, parent)) return world_pos;
    prts_dvec3 rel = prts_dv3_sub(world_pos, prts_world_pos(w, parent));
    return rotate_y_d(rel, -(double)prts_world_yaw(w, parent));
}

void prts_world_set_pos(prts_world *w, prts_entity e, prts_dvec3 world_pos)
{
    slot *s = get(w, e);
    if (!s) return;
    s->local = to_local(w, s->parent, world_pos);
}

prts_entity prts_world_parent(const prts_world *w, prts_entity e)
{
    slot *s = get(w, e);
    return s ? s->parent : PRTS_ENTITY_NONE;
}

bool prts_world_set_parent(prts_world *w, prts_entity e, prts_entity parent)
{
    slot *s = get(w, e);
    if (!s) return false;
    if (parent != PRTS_ENTITY_NONE && !get(w, parent)) return false;
    for (prts_entity p = parent; p != PRTS_ENTITY_NONE; p = prts_world_parent(w, p)) {
        if (p == e) return false;
    }
    if (s->parent == parent) return true;
    prts_dvec3 pos = prts_world_pos(w, e);
    float yaw = prts_world_yaw(w, e);
    s->parent = parent;
    s->local = to_local(w, parent, pos);
    s->yaw = yaw - prts_world_yaw(w, parent);
    s->moving_pos = s->moving_yaw = false;
    return true;
}

void prts_world_set_velocity(prts_world *w, prts_entity e, prts_vec3 v)
{
    slot *s = get(w, e);
    if (s) s->vel = v;
}

prts_vec3 prts_world_velocity(const prts_world *w, prts_entity e)
{
    slot *s = get(w, e);
    return s ? s->vel : prts_v3(0, 0, 0);
}

void prts_world_begin_tick(prts_world *w)
{
    for (uint32_t i = 0; i < w->slots.len; i++) {
        slot *s = prts_array_at(&w->slots, i);
        if (!s->alive) continue;
        prts_entity e = make_entity(i, s->gen);
        s->prev_pos = prts_world_pos(w, e);
        s->prev_yaw = prts_world_yaw(w, e);
    }
}

prts_dvec3 prts_world_pos_interp(const prts_world *w, prts_entity e, double alpha)
{
    slot *s = get(w, e);
    if (!s) return prts_dv3(0, 0, 0);
    return prts_dv3_lerp(s->prev_pos, prts_world_pos(w, e), alpha);
}

float prts_world_yaw_interp(const prts_world *w, prts_entity e, double alpha)
{
    slot *s = get(w, e);
    if (!s) return 0.0f;
    float cur = prts_world_yaw(w, e);
    return s->prev_yaw + prts_wrap_angle(cur - s->prev_yaw) * (float)alpha;
}

void prts_world_move_to(prts_world *w, prts_entity e, prts_dvec3 local_target, float speed)
{
    slot *s = get(w, e);
    if (!s) return;
    s->target = local_target;
    s->speed = speed;
    s->moving_pos = true;
}

void prts_world_rotate_to(prts_world *w, prts_entity e, float local_yaw_target, float speed)
{
    slot *s = get(w, e);
    if (!s) return;
    s->yaw_target = local_yaw_target;
    s->yaw_speed = speed;
    s->moving_yaw = true;
}

bool prts_world_moving(const prts_world *w, prts_entity e)
{
    slot *s = get(w, e);
    return s && (s->moving_pos || s->moving_yaw);
}

void prts_world_tick_movers(prts_world *w, float dt, prts_world_done_fn done, void *user)
{
    for (uint32_t i = 0; i < w->slots.len; i++) {
        slot *s = prts_array_at(&w->slots, i);
        if (!s->alive || !(s->moving_pos || s->moving_yaw)) continue;
        if (s->moving_pos) {
            prts_dvec3 d = prts_dv3_sub(s->target, s->local);
            double dist = sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
            double step = (double)s->speed * (double)dt;
            if (dist <= step || dist == 0.0) {
                s->local = s->target;
                s->moving_pos = false;
            } else {
                s->local = prts_dv3_add(s->local, prts_dv3(d.x / dist * step, d.y / dist * step, d.z / dist * step));
            }
        }
        if (s->moving_yaw) {
            float d = s->yaw_target - s->yaw;
            float step = s->yaw_speed * dt;
            if (fabsf(d) <= step) {
                s->yaw = s->yaw_target;
                s->moving_yaw = false;
            } else {
                s->yaw += d > 0 ? step : -step;
            }
        }
        if (!s->moving_pos && !s->moving_yaw && done) done(make_entity(i, s->gen), user);
    }
}

void prts_world_set_usable(prts_world *w, prts_entity e, bool usable)
{
    slot *s = get(w, e);
    if (s) s->usable = usable;
}

bool prts_world_usable(const prts_world *w, prts_entity e)
{
    slot *s = get(w, e);
    return s && s->usable;
}

uint64_t prts_world_hash(const prts_world *w)
{
    prts_hasher h;
    prts_hasher_init(&h);
    for (uint32_t i = 0; i < w->slots.len; i++) {
        slot *s = prts_array_at(&w->slots, i);
        prts_hasher_u64(&h, ((uint64_t)s->gen << 1) | (s->alive ? 1u : 0u));
        if (!s->alive) continue;
        prts_hasher_u64(&h, s->parent);
        prts_hasher_f64(&h, s->local.x);
        prts_hasher_f64(&h, s->local.y);
        prts_hasher_f64(&h, s->local.z);
        prts_hasher_f64(&h, s->yaw);
        prts_hasher_f64(&h, s->vel.x);
        prts_hasher_f64(&h, s->vel.y);
        prts_hasher_f64(&h, s->vel.z);
        prts_hasher_u64(&h, (uint64_t)s->moving_pos | ((uint64_t)s->moving_yaw << 1) | ((uint64_t)s->usable << 2));
        if (s->moving_pos) {
            prts_hasher_f64(&h, s->target.x);
            prts_hasher_f64(&h, s->target.y);
            prts_hasher_f64(&h, s->target.z);
            prts_hasher_f64(&h, s->speed);
        }
        if (s->moving_yaw) {
            prts_hasher_f64(&h, s->yaw_target);
            prts_hasher_f64(&h, s->yaw_speed);
        }
    }
    return h.h;
}
