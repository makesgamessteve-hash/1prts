/* 1stPRTS engine: game scripting API (prts.scene, entity, input, commander, render). MIT License. */
#include <lauxlib.h>
#include <math.h>
#include <string.h>

#include "app/engine_internal.h"

static const char *REG_EDATA = PRTS_REG_EDATA;

/* --- argument helpers ---------------------------------------------------------- */

static float field_number(lua_State *L, int t, const char *key, float def)
{
    lua_getfield(L, t, key);
    float v = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : def;
    lua_pop(L, 1);
    return v;
}

static bool field_bool(lua_State *L, int t, const char *key, bool def)
{
    lua_getfield(L, t, key);
    bool v = lua_isnil(L, -1) ? def : lua_toboolean(L, -1);
    lua_pop(L, 1);
    return v;
}

static const char *field_string(lua_State *L, int t, const char *key, const char *def)
{
    lua_getfield(L, t, key);
    const char *v = lua_isstring(L, -1) ? lua_tostring(L, -1) : def;
    lua_pop(L, 1); /* the string stays alive in the table */
    return v;
}

/* Reads {x, y, z} or {x=, y=, z=} from table field `key` (or from index t itself if key is NULL). */
static bool read_vec3(lua_State *L, int t, const char *key, prts_vec3 *out)
{
    if (key) lua_getfield(L, t, key);
    else lua_pushvalue(L, t);
    bool ok = lua_istable(L, -1);
    if (ok) {
        int v = lua_gettop(L);
        lua_rawgeti(L, v, 1);
        lua_rawgeti(L, v, 2);
        lua_rawgeti(L, v, 3);
        if (lua_isnumber(L, -3)) {
            *out = prts_v3((float)lua_tonumber(L, -3), (float)lua_tonumber(L, -2), (float)lua_tonumber(L, -1));
        } else {
            *out = prts_v3(field_number(L, v, "x", 0), field_number(L, v, "y", 0), field_number(L, v, "z", 0));
        }
        lua_pop(L, 3);
    }
    lua_pop(L, 1);
    return ok;
}

static prts_vec3 check_vec3(lua_State *L, int t, const char *key)
{
    prts_vec3 v = prts_v3(0, 0, 0);
    if (!read_vec3(L, t, key, &v)) luaL_error(L, "field '%s' must be a {x, y, z} table", key);
    return v;
}

prts_entity prts_engine_check_entity(lua_State *L, int idx)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_entity ent = (prts_entity)luaL_checkinteger(L, idx);
    if (!prts_world_alive(e->world, ent)) luaL_error(L, "stale entity handle");
    return ent;
}

static prts_entity opt_entity(lua_State *L, int t, const char *key)
{
    lua_getfield(L, t, key);
    prts_entity ent = PRTS_ENTITY_NONE;
    if (!lua_isnil(L, -1)) ent = prts_engine_check_entity(L, -1);
    lua_pop(L, 1);
    return ent;
}

static int material_index(lua_State *L, prts_engine *e, int t)
{
    const char *id = field_string(L, t, "material", NULL);
    if (!id) return luaL_error(L, "field 'material' is required");
    int m = prts_scene_find_material(e->scene, id);
    if (m < 0) return luaL_error(L, "unknown material '%s'", id);
    return m;
}

/* --- prts.scene ------------------------------------------------------------------ */

static int l_scene_material(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_material m;
    memset(&m, 0, sizeof(m));
    const char *id = field_string(L, 1, "id", NULL);
    if (!id) return luaL_error(L, "field 'id' is required");
    strncpy(m.id, id, sizeof(m.id) - 1);
    strncpy(m.texture, field_string(L, 1, "texture", ""), sizeof(m.texture) - 1);
    m.tint = prts_v3(1, 1, 1);
    read_vec3(L, 1, "tint", &m.tint);
    m.uv_scale = field_number(L, 1, "uv_scale", 2.0f);
    m.emissive = field_bool(L, 1, "emissive", false);
    m.magic = field_bool(L, 1, "magic", false);
    strncpy(m.surface, field_string(L, 1, "surface", ""), sizeof(m.surface) - 1);
    lua_pushinteger(L, prts_scene_material(e->scene, &m));
    return 1;
}

static int l_scene_box(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_aabb b = prts_aabb_make(check_vec3(L, 1, "min"), check_vec3(L, 1, "max"));
    prts_entity owner = opt_entity(L, 1, "entity");
    if (field_bool(L, 1, "visible", true)) { /* an invisible box needs no material */
        int m = material_index(L, e, 1);
        if (field_bool(L, 1, "fit", false)) prts_scene_add_box_fitted(e->scene, b, m, owner);
        else prts_scene_add_box(e->scene, b, m, owner, field_number(L, 1, "tile", 2.0f));
    }
    int id = -1;
    if (field_bool(L, 1, "use_only", false)) id = prts_cworld_add_use_box(e->cworld, b, owner);
    else if (field_bool(L, 1, "solid", true)) id = prts_cworld_add_box(e->cworld, b, owner);
    lua_pushinteger(L, id);
    return 1;
}

/* prts.scene.wedge{ min, max, axis = "x"|"z", sign = 1|-1, material, entity, tile }: a ramp */
static int l_scene_wedge(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_aabb b = prts_aabb_make(check_vec3(L, 1, "min"), check_vec3(L, 1, "max"));
    const char *axis = field_string(L, 1, "axis", "x");
    if (strcmp(axis, "x") != 0 && strcmp(axis, "z") != 0) return luaL_error(L, "axis must be \"x\" or \"z\"");
    int sign = field_number(L, 1, "sign", 1.0f) < 0.0f ? -1 : 1;
    prts_scene_add_wedge(e->scene, b, axis[0] == 'z' ? 2 : 0, sign, material_index(L, e, 1), opt_entity(L, 1, "entity"),
                         field_number(L, 1, "tile", 2.0f));
    return 0;
}

/* prts.scene.tube{ base, radius, inner, height, sides, open = { i, ... }, cap, material, entity, tile } */
static int l_scene_tube(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_tube t;
    memset(&t, 0, sizeof(t));
    t.base = check_vec3(L, 1, "base");
    t.radius = field_number(L, 1, "radius", 4.0f);
    t.inner = field_number(L, 1, "inner", t.radius - 1.0f);
    t.height = field_number(L, 1, "height", 4.0f);
    t.sides = (int)field_number(L, 1, "sides", 16.0f);
    t.cap = field_bool(L, 1, "cap", true);
    t.tile = field_number(L, 1, "tile", 2.0f);
    if (t.inner >= t.radius) return luaL_error(L, "inner radius must be smaller than the radius");
    lua_getfield(L, 1, "open");
    if (lua_istable(L, -1)) {
        for (int i = 1; lua_rawgeti(L, -1, i) == LUA_TNUMBER; i++) {
            lua_Integer side = lua_tointeger(L, -1);
            if (side >= 0 && side < 64) t.open |= (uint64_t)1 << side;
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    prts_scene_add_tube(e->scene, &t, material_index(L, e, 1), opt_entity(L, 1, "entity"));
    return 0;
}

static int l_scene_prism(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_vec3 base = check_vec3(L, 1, "base");
    float r = field_number(L, 1, "radius", 1.0f), top = field_number(L, 1, "top_radius", 0.0f);
    float h = field_number(L, 1, "height", 1.0f);
    int sides = (int)field_number(L, 1, "sides", 6.0f);
    prts_entity owner = opt_entity(L, 1, "entity");
    prts_scene_add_prism(e->scene, base, r, top, h, sides, material_index(L, e, 1), owner);
    if (field_bool(L, 1, "solid", false)) {
        float rr = fmaxf(r, top) * 0.7f;
        prts_aabb b = prts_aabb_make(prts_v3(base.x - rr, base.y, base.z - rr), prts_v3(base.x + rr, base.y + h, base.z + rr));
        prts_cworld_add_box(e->cworld, b, owner);
    }
    return 0;
}

static int l_scene_terrain(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_terrain_params p;
    p.size = field_number(L, 1, "size", 1024.0f);
    p.spacing = field_number(L, 1, "spacing", 4.0f);
    p.seed = (uint64_t)field_number(L, 1, "seed", 1.0f);
    p.height = field_number(L, 1, "height", 20.0f);
    p.octaves = (int)field_number(L, 1, "octaves", 4.0f);
    p.material = material_index(L, e, 1);
    if (p.size < p.spacing * 2.0f || p.spacing <= 0.0f) return luaL_error(L, "terrain size/spacing out of range");
    prts_terrain_flat flats[32];
    int n = 0;
    lua_getfield(L, 1, "flats");
    if (lua_istable(L, -1)) {
        int ft = lua_gettop(L);
        for (int i = 1; n < 32 && lua_rawgeti(L, ft, i) == LUA_TTABLE; i++) {
            int f = lua_gettop(L);
            prts_terrain_flat fl = {field_number(L, f, "x", 0), field_number(L, f, "z", 0), field_number(L, f, "radius", 10),
                                    field_number(L, f, "height", 0), field_number(L, f, "blend", 10)};
            flats[n++] = fl;
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    prts_scene_terrain(e->scene, &p, flats, n);
    prts_cworld_set_heightfield(e->cworld, e->scene->terrain);
    /* holes = { { x0, z0, x1, z1 }, ... }: open ground (mouths, shafts) */
    lua_getfield(L, 1, "holes");
    if (lua_istable(L, -1)) {
        int ht = lua_gettop(L);
        for (int i = 1; lua_rawgeti(L, ht, i) == LUA_TTABLE; i++) {
            float r[4];
            for (int k = 0; k < 4; k++) {
                lua_rawgeti(L, -1 - k, k + 1);
                r[k] = (float)luaL_checknumber(L, -1);
            }
            lua_pop(L, 5);
            prts_scene_terrain_hole(e->scene, r[0], r[1], r[2], r[3]);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return 0;
}

/* prts.scene.hole{ min = { x, z }, max = { x, z } } -> quads opened */
static int l_scene_hole(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    float v[4];
    const char *keys[2] = {"min", "max"};
    for (int k = 0; k < 2; k++) {
        lua_getfield(L, 1, keys[k]);
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_rawgeti(L, -1, 1);
        lua_rawgeti(L, -2, 2);
        v[k * 2] = (float)luaL_checknumber(L, -2);
        v[k * 2 + 1] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 3);
    }
    lua_pushinteger(L, prts_scene_terrain_hole(e->scene, v[0], v[1], v[2], v[3]));
    return 1;
}

/* prts.scene.hole_at(x, z) -> true if the terrain is open there */
static int l_scene_hole_at(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    float x = (float)luaL_checknumber(L, 1), z = (float)luaL_checknumber(L, 2);
    lua_pushboolean(L, e->scene->terrain && prts_heightfield_hole(e->scene->terrain, x, z));
    return 1;
}

static int l_scene_height(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    lua_pushnumber(L, prts_scene_height(e->scene, (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2)));
    return 1;
}

static int l_scene_light(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_light l = {check_vec3(L, 1, "pos"), prts_v3(1, 0.8f, 0.5f), field_number(L, 1, "radius", 8.0f),
                    opt_entity(L, 1, "entity")};
    read_vec3(L, 1, "color", &l.color);
    prts_scene_add_light(e->scene, l);
    return 0;
}

static int l_scene_marker(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *name = luaL_checkstring(L, 1);
    prts_vec3 p = check_vec3(L, 2, NULL);
    prts_scene_add_marker(e->scene, name, prts_dv3_from_v3(p), (float)luaL_optnumber(L, 3, 0.0));
    return 0;
}

static int l_scene_marker_pos(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const prts_marker *m = prts_scene_find_marker(e->scene, luaL_checkstring(L, 1));
    if (!m) return 0;
    lua_pushnumber(L, m->pos.x);
    lua_pushnumber(L, m->pos.y);
    lua_pushnumber(L, m->pos.z);
    lua_pushnumber(L, m->yaw);
    return 4;
}

/* prts.scene.in_zone(name, x, y, z) -> true if the point is inside the named zone */
static int l_scene_in_zone(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *name = luaL_checkstring(L, 1);
    const prts_zone *z = prts_scene_find_zone(e->scene, name);
    if (!z) return luaL_error(L, "unknown zone '%s'", name);
    prts_vec3 p = prts_v3((float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4));
    lua_pushboolean(L, p.x >= z->box.min.x && p.x <= z->box.max.x && p.y >= z->box.min.y && p.y <= z->box.max.y &&
                           p.z >= z->box.min.z && p.z <= z->box.max.z);
    return 1;
}

static int l_scene_zone(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    luaL_checktype(L, 3, LUA_TTABLE);
    prts_aabb b = prts_aabb_make(check_vec3(L, 2, NULL), check_vec3(L, 3, NULL));
    prts_scene_add_zone(e->scene, name, b);
    return 0;
}

static int l_scene_env(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_scene_env *env = &e->scene->env;
    read_vec3(L, 1, "sun_dir", &env->sun_dir);
    read_vec3(L, 1, "sun_color", &env->sun_color);
    read_vec3(L, 1, "ambient", &env->ambient);
    read_vec3(L, 1, "fog_color", &env->fog_color);
    read_vec3(L, 1, "sky_horizon", &env->sky_horizon);
    read_vec3(L, 1, "sky_zenith", &env->sky_zenith);
    env->fog_near = field_number(L, 1, "fog_near", env->fog_near);
    env->fog_far = field_number(L, 1, "fog_far", env->fog_far);
    env->stars = field_number(L, 1, "stars", env->stars);
    env->moons = field_bool(L, 1, "moons", env->moons);
    /* fog and sky colours are read every frame; only light changes need the scene baked again */
    bool lit = false;
    const char *light_fields[] = {"sun_dir", "sun_color", "ambient"};
    for (int i = 0; i < 3; i++) {
        lua_getfield(L, 1, light_fields[i]);
        lit = lit || !lua_isnil(L, -1);
        lua_pop(L, 1);
    }
    if (lit) e->scene->revision++;
    return 0;
}

static int l_scene_illusion(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_vec3 center = check_vec3(L, 1, "center");
    prts_vec3 color = prts_v3(1.0f, 0.8f, 0.35f);
    read_vec3(L, 1, "color", &color);
    lua_getfield(L, 1, "area");
    luaL_checktype(L, -1, LUA_TTABLE);
    int a = lua_gettop(L);
    float x0 = field_number(L, a, "x0", -500), z0 = field_number(L, a, "z0", -500);
    float x1 = field_number(L, a, "x1", 500), z1 = field_number(L, a, "z1", 500);
    lua_pop(L, 1);
    prts_scene_add_contours(e->scene, opt_entity(L, 1, "entity"), center, field_number(L, 1, "size_x", 2.0f),
                            field_number(L, 1, "size_z", 2.0f), field_number(L, 1, "relief", 0.3f), x0, z0, x1, z1,
                            (int)field_number(L, 1, "levels", 8), color);
    return 0;
}

/* prts.scene.surface{ name=, pos=, yaw=, pitch=, fov= (degrees), size = { w, h }, fog = false } */
static int l_scene_surface(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_surface sf;
    memset(&sf, 0, sizeof(sf));
    const char *name = field_string(L, 1, "name", NULL);
    if (!name) return luaL_error(L, "field 'name' is required");
    strncpy(sf.name, name, sizeof(sf.name) - 1);
    sf.pos = prts_dv3_from_v3(check_vec3(L, 1, "pos"));
    sf.yaw = field_number(L, 1, "yaw", 0.0f);
    sf.pitch = field_number(L, 1, "pitch", 0.0f);
    sf.fov_y = field_number(L, 1, "fov", 60.0f) * (float)PRTS_PI / 180.0f;
    prts_vec3 size = prts_v3(128, 72, 0);
    read_vec3(L, 1, "size", &size);
    sf.fog = field_bool(L, 1, "fog", false);
    sf.w = PRTS_CLAMP((int)size.x, 8, 1024);
    sf.h = PRTS_CLAMP((int)size.y, 8, 1024);
    lua_pushinteger(L, prts_scene_add_surface(e->scene, &sf));
    return 1;
}

/* prts.scene.aim_surface(name, pos [, yaw [, pitch]]): moves a surface's camera (no rebuild) */
static int l_scene_aim_surface(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    int i = prts_scene_find_surface(e->scene, luaL_checkstring(L, 1));
    if (i < 0) return luaL_error(L, "unknown surface '%s'", lua_tostring(L, 1));
    prts_surface *sf = prts_array_at(&e->scene->surfaces, (size_t)i);
    luaL_checktype(L, 2, LUA_TTABLE);
    float v[3];
    for (int k = 0; k < 3; k++) {
        lua_rawgeti(L, 2, k + 1);
        v[k] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    sf->pos = prts_dv3_from_v3(prts_v3(v[0], v[1], v[2]));
    if (!lua_isnoneornil(L, 3)) sf->yaw = (float)luaL_checknumber(L, 3);
    if (!lua_isnoneornil(L, 4)) sf->pitch = (float)luaL_checknumber(L, 4);
    return 0;
}

/* Loads (once) a sprite definition from its Lua metadata file through the VFS. */
static int load_sprite_def(lua_State *L, prts_engine *e, const char *path)
{
    int found = prts_scene_find_sprite_def(e->scene, path);
    if (found >= 0) return found;
    char *code = NULL;
    if (prts_vfs_read(e->vfs, path, &code, NULL) != PRTS_VFS_OK) return luaL_error(L, "cannot read sprite '%s'", path);
    lua_State *main = prts_script_lua(e->script);
    int base = lua_gettop(main);
    bool ok = prts_script_eval_data(e->script, code, path);
    prts_free(code);
    /* the data is evaluated on the main state; a caller in a coroutine (a scenario) needs it on its own stack */
    if (ok && main != L) lua_xmove(main, L, lua_gettop(main) - base);
    if (!ok || !lua_istable(L, -1)) return luaL_error(L, "sprite '%s' is not a data table", path);
    int t = lua_gettop(L);
    prts_sprite_def def;
    memset(&def, 0, sizeof(def));
    strncpy(def.path, path, sizeof(def.path) - 1);
    strncpy(def.texture, field_string(L, t, "texture", ""), sizeof(def.texture) - 1);
    prts_vec3 frame = prts_v3(32, 64, 0);
    read_vec3(L, t, "frame", &frame);
    def.frame_w = (int)frame.x;
    def.frame_h = (int)frame.y;
    def.columns = PRTS_MAX(1, (int)field_number(L, t, "columns", 1));
    def.rows = PRTS_MAX(1, (int)field_number(L, t, "rows", 1));
    def.height = field_number(L, t, "height", 1.8f);
    def.directions = PRTS_MAX(1, (int)field_number(L, t, "directions", 8));
    lua_getfield(L, t, "anims");
    int anims = lua_gettop(L);
    for (int i = 1; def.anim_count < PRTS_SPRITE_MAX_ANIMS && lua_istable(L, anims) && lua_rawgeti(L, anims, i) == LUA_TTABLE; i++) {
        int a = lua_gettop(L);
        prts_sprite_anim *an = &def.anims[def.anim_count++];
        strncpy(an->name, field_string(L, a, "name", "idle"), sizeof(an->name) - 1);
        an->first_row = (int)field_number(L, a, "first_row", 0);
        an->frames = PRTS_MAX(1, (int)field_number(L, a, "frames", 1));
        an->fps = field_number(L, a, "fps", 1.0f);
        lua_pop(L, 1);
    }
    lua_settop(L, t - 1);
    if (def.anim_count == 0) return luaL_error(L, "sprite '%s' has no animations", path);
    return prts_scene_add_sprite_def(e->scene, &def);
}

/* prts.scene.sprite{ sprite = "sprites/x.lua", pos =, yaw =, entity =, anim = } -> id */
static int l_scene_sprite(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    const char *path = field_string(L, 1, "sprite", NULL);
    if (!path) return luaL_error(L, "field 'sprite' is required");
    prts_sprite sp;
    memset(&sp, 0, sizeof(sp));
    sp.def = load_sprite_def(L, e, path);
    sp.owner = opt_entity(L, 1, "entity");
    read_vec3(L, 1, "pos", &sp.pos);
    sp.yaw = field_number(L, 1, "yaw", 0.0f);
    const prts_sprite_def *def = prts_array_at(&e->scene->sprite_defs, (size_t)sp.def);
    sp.anim = PRTS_MAX(0, prts_scene_find_anim(def, field_string(L, 1, "anim", "idle")));
    sp.anim_start = (double)e->clock.tick * e->clock.tick_seconds;
    lua_pushinteger(L, prts_scene_add_sprite(e->scene, &sp));
    return 1;
}

static void push_vec3(lua_State *L, prts_vec3 v)
{
    lua_createtable(L, 3, 0);
    lua_pushnumber(L, v.x);
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, v.y);
    lua_rawseti(L, -2, 2);
    lua_pushnumber(L, v.z);
    lua_rawseti(L, -2, 3);
}

/* prts.scene.model{path=, pos=, yaw=, scale=, entity=, name=, solid=}: loads a glTF or
 * .vox model. Returns ok, {min=, max=}. A missing model shows a placeholder (ok = false). */
static int l_scene_model(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    const char *path = field_string(L, 1, "path", NULL);
    if (!path) return luaL_error(L, "field 'path' is required");
    prts_model_place pl;
    memset(&pl, 0, sizeof(pl));
    read_vec3(L, 1, "pos", &pl.pos);
    pl.yaw = field_number(L, 1, "yaw", 0.0f);
    pl.scale = field_number(L, 1, "scale", 0.0f);
    pl.owner = opt_entity(L, 1, "entity");
    prts_aabb b;
    bool ok = prts_model_add(e->scene, e->vfs, path, &pl, field_string(L, 1, "name", NULL), &b);
    if (field_bool(L, 1, "solid", false)) prts_cworld_add_box(e->cworld, b, pl.owner);
    lua_pushboolean(L, ok);
    lua_createtable(L, 0, 2);
    push_vec3(L, b.min);
    lua_setfield(L, -2, "min");
    push_vec3(L, b.max);
    lua_setfield(L, -2, "max");
    return 2;
}

/* prts.scene.sprite_anim(id, name): switches animation (restarting it). */
static int l_scene_sprite_anim(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    lua_Integer id = luaL_checkinteger(L, 1);
    prts_sprite *sp = id >= 0 ? prts_array_at(&e->scene->sprites, (size_t)id) : NULL;
    if (!sp) return luaL_error(L, "unknown sprite %d", (int)id);
    const prts_sprite_def *def = prts_array_at(&e->scene->sprite_defs, (size_t)sp->def);
    int a = prts_scene_find_anim(def, luaL_checkstring(L, 2));
    if (a < 0) return luaL_error(L, "sprite has no animation '%s'", lua_tostring(L, 2));
    if (a != sp->anim) {
        sp->anim = a;
        sp->anim_start = (double)e->clock.tick * e->clock.tick_seconds;
    }
    return 0;
}

/* --- cells and portals ------------------------------------------------------------- */

static int check_cell(lua_State *L, prts_engine *e, const char *name)
{
    int c = prts_scene_find_cell(e->scene, name);
    if (c < 0) return luaL_error(L, "unknown cell '%s'", name);
    return c;
}

static int l_scene_cell(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    const char *name = field_string(L, 1, "name", NULL);
    if (!name) return luaL_error(L, "field 'name' is required");
    if (prts_scene_find_cell(e->scene, name) >= 0) return luaL_error(L, "cell '%s' already exists", name);
    int c = prts_scene_add_cell(e->scene, name, prts_aabb_make(check_vec3(L, 1, "min"), check_vec3(L, 1, "max")));
    if (c < 0) return luaL_error(L, "too many cells (at most %d)", PRTS_MAX_CELLS);
    lua_pushinteger(L, c);
    return 1;
}

static int l_scene_portal(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    const char *name = field_string(L, 1, "name", NULL);
    if (!name) return luaL_error(L, "field 'name' is required");
    int a = check_cell(L, e, field_string(L, 1, "a", "outside"));
    int b = check_cell(L, e, field_string(L, 1, "b", "outside"));
    prts_aabb box = prts_aabb_make(check_vec3(L, 1, "min"), check_vec3(L, 1, "max"));
    prts_scene_add_portal(e->scene, name, a, b, box, field_bool(L, 1, "open", true));
    return 0;
}

static int l_scene_set_portal(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *name = luaL_checkstring(L, 1);
    if (!prts_scene_set_portal(e->scene, name, lua_toboolean(L, 2))) return luaL_error(L, "unknown portal '%s'", name);
    return 0;
}

static int l_scene_portal_open(lua_State *L)
{
    lua_pushboolean(L, prts_scene_portal_open(prts_engine_from_lua(L)->scene, luaL_checkstring(L, 1)));
    return 1;
}

/* prts.scene.cells{ "a", "b" } tags geometry added afterwards; prts.scene.cells() resets to all. */
static int l_scene_cells(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_cell_mask mask = PRTS_CELLS_ALL;
    if (lua_istable(L, 1)) {
        mask = 0;
        for (int i = 1; lua_rawgeti(L, 1, i) == LUA_TSTRING; i++) {
            mask |= PRTS_CELL_BIT(check_cell(L, e, lua_tostring(L, -1)));
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    e->scene->current_cells = mask;
    return 0;
}

static int l_scene_cell_at(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_vec3 p = prts_v3((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3));
    lua_pushstring(L, prts_scene_cell_name(e->scene, prts_scene_cell_at(e->scene, p)));
    return 1;
}

/* --- prts.entity ------------------------------------------------------------------- */

static int l_entity_spawn(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_entity ent = prts_world_spawn(e->world, field_string(L, 1, "name", NULL));
    prts_entity parent = opt_entity(L, 1, "parent");
    prts_vec3 pos = prts_v3(0, 0, 0);
    read_vec3(L, 1, "pos", &pos);
    prts_world_set_parent(e->world, ent, parent);
    prts_world_set_local(e->world, ent, prts_dv3_from_v3(pos), field_number(L, 1, "yaw", 0.0f));
    prts_world_set_usable(e->world, ent, field_bool(L, 1, "usable", false));
    prts_cworld_sync(e->cworld, e->world);
    lua_pushinteger(L, (lua_Integer)ent);
    return 1;
}

static int l_entity_pos(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_dvec3 p = prts_world_pos(e->world, prts_engine_check_entity(L, 1));
    lua_pushnumber(L, p.x);
    lua_pushnumber(L, p.y);
    lua_pushnumber(L, p.z);
    return 3;
}

static int l_entity_local_pos(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_dvec3 p = prts_world_local_pos(e->world, prts_engine_check_entity(L, 1));
    lua_pushnumber(L, p.x);
    lua_pushnumber(L, p.y);
    lua_pushnumber(L, p.z);
    return 3;
}

static int l_entity_yaw(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    lua_pushnumber(L, prts_world_local_yaw(e->world, prts_engine_check_entity(L, 1)));
    return 1;
}

static int l_entity_move_to(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_entity ent = prts_engine_check_entity(L, 1);
    prts_vec3 target = check_vec3(L, 2, NULL);
    prts_world_move_to(e->world, ent, prts_dv3_from_v3(target), (float)luaL_checknumber(L, 3));
    return 0;
}

/* prts.entity.place(e, pos [, yaw]): sets the local position (and yaw) at once, ending any move
 * or turn under way. Vehicles driven each tick and things held by the commander use it. */
static int l_entity_place(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_entity ent = prts_engine_check_entity(L, 1);
    prts_vec3 pos = check_vec3(L, 2, NULL);
    float yaw = lua_isnoneornil(L, 3) ? prts_world_yaw(e->world, ent) : (float)luaL_checknumber(L, 3);
    prts_world_stop(e->world, ent);
    prts_world_set_local(e->world, ent, prts_dv3_from_v3(pos), yaw);
    return 0;
}

static int l_entity_rotate_to(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_entity ent = prts_engine_check_entity(L, 1);
    prts_world_rotate_to(e->world, ent, (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3));
    return 0;
}

static int l_entity_moving(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    lua_pushboolean(L, prts_world_moving(e->world, prts_engine_check_entity(L, 1)));
    return 1;
}

static int l_entity_set_solid(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_cworld_set_owner_enabled(e->cworld, prts_engine_check_entity(L, 1), lua_toboolean(L, 2));
    return 0;
}

static int l_entity_name(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    lua_pushstring(L, prts_world_name(e->world, prts_engine_check_entity(L, 1)));
    return 1;
}

static int l_entity_find(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_entity ent = prts_world_find(e->world, luaL_checkstring(L, 1));
    if (ent == PRTS_ENTITY_NONE) return 0;
    lua_pushinteger(L, (lua_Integer)ent);
    return 1;
}

static int l_entity_alive(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    lua_pushboolean(L, prts_world_alive(e->world, (prts_entity)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_entity_despawn(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    prts_entity ent = prts_engine_check_entity(L, 1);
    prts_cworld_set_owner_enabled(e->cworld, ent, false);
    prts_world_despawn(e->world, ent);
    return 0;
}

/* Per-entity script data: a plain Lua table kept by the engine. */
static int l_entity_data(lua_State *L)
{
    prts_entity ent = prts_engine_check_entity(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, REG_EDATA);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, REG_EDATA);
    }
    if (lua_rawgeti(L, -1, (lua_Integer)ent) != LUA_TTABLE) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_rawseti(L, -3, (lua_Integer)ent);
    }
    return 1;
}

/* --- prts.input ------------------------------------------------------------------- */

static int l_input_action(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    const char *id = field_string(L, 1, "id", NULL);
    if (!id) return luaL_error(L, "field 'id' is required");
    prts_input_define(e->input, id);
    prts_engine_apply_bindings(L, e, id, 1); /* defaults, then the player's overrides (api_settings.c) */
    return 0;
}

static int l_input_value(lua_State *L)
{
    lua_pushnumber(L, prts_input_value(prts_engine_from_lua(L)->input, luaL_checkstring(L, 1)));
    return 1;
}

static int l_input_down(lua_State *L)
{
    lua_pushboolean(L, prts_input_down(prts_engine_from_lua(L)->input, luaL_checkstring(L, 1)));
    return 1;
}

static int l_input_scheme(lua_State *L)
{
    lua_pushstring(L, prts_input_scheme_name(prts_input_scheme_active(prts_engine_from_lua(L)->input)));
    return 1;
}

/* --- prts.commander ------------------------------------------------------------------ */

static void read_params(lua_State *L, int t, prts_commander_params *p)
{
    p->walk_speed = field_number(L, t, "walk_speed", p->walk_speed);
    p->run_speed = field_number(L, t, "run_speed", p->run_speed);
    p->sprint_speed = field_number(L, t, "sprint_speed", p->sprint_speed);
    p->crouch_speed = field_number(L, t, "crouch_speed", p->crouch_speed);
    p->jump_speed = field_number(L, t, "jump_speed", p->jump_speed);
    p->eye_stand = field_number(L, t, "eye_stand", p->eye_stand);
    p->eye_crouch = field_number(L, t, "eye_crouch", p->eye_crouch);
    p->step_height = field_number(L, t, "step_height", p->step_height);
    p->reach = field_number(L, t, "reach", p->reach);
    p->look_sensitivity = field_number(L, t, "look_sensitivity", p->look_sensitivity);
}

static int l_commander_spawn(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    prts_commander_params p;
    prts_commander_params_default(&p);
    read_params(L, 1, &p);
    prts_dvec3 pos = prts_dv3(0, 0, 0);
    float yaw = field_number(L, 1, "yaw", 0.0f);
    const char *marker = field_string(L, 1, "marker", NULL);
    prts_vec3 v;
    if (marker) {
        const prts_marker *m = prts_scene_find_marker(e->scene, marker);
        if (!m) return luaL_error(L, "unknown marker '%s'", marker);
        pos = m->pos;
        yaw = m->yaw;
    } else if (read_vec3(L, 1, "pos", &v)) {
        pos = prts_dv3_from_v3(v);
    }
    if (e->has_commander) prts_world_despawn(e->world, e->commander.entity);
    prts_entity ent = prts_world_spawn(e->world, "commander");
    prts_world_set_local(e->world, ent, pos, 0.0f);
    prts_commander_init(&e->commander, ent, &p);
    e->commander.yaw = yaw;
    e->has_commander = true;
    lua_pushinteger(L, (lua_Integer)ent);
    return 1;
}

/* prts.commander.tune{ walk_speed, ... }: changes the spawned commander's parameters in place
 * (kept in saves); returns the parameters as they are now. */
static int l_commander_tune(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    if (!e->has_commander) return luaL_error(L, "prts.commander.tune: no commander");
    prts_commander_params *p = &e->commander.p;
    if (lua_istable(L, 1)) {
        prts_commander_params n = *p;
        read_params(L, 1, &n);
        const float speeds[] = {n.walk_speed, n.run_speed, n.sprint_speed, n.crouch_speed};
        for (size_t i = 0; i < PRTS_ARRAY_COUNT(speeds); i++) {
            if (!(speeds[i] >= 0.0f && speeds[i] <= 50.0f)) return luaL_error(L, "speeds must be between 0 and 50");
        }
        *p = n;
    }
    lua_createtable(L, 0, 6);
    lua_pushnumber(L, p->walk_speed);
    lua_setfield(L, -2, "walk_speed");
    lua_pushnumber(L, p->run_speed);
    lua_setfield(L, -2, "run_speed");
    lua_pushnumber(L, p->sprint_speed);
    lua_setfield(L, -2, "sprint_speed");
    lua_pushnumber(L, p->crouch_speed);
    lua_setfield(L, -2, "crouch_speed");
    lua_pushnumber(L, p->eye_stand);
    lua_setfield(L, -2, "eye_stand");
    lua_pushnumber(L, p->step_height);
    lua_setfield(L, -2, "step_height");
    return 1;
}

/* prts.commander.place(pos [, yaw]): puts the commander's feet at a world position at once (a
 * seat, a saddle), keeping the view's pitch; the frame they then stand on carries them. */
static int l_commander_place(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    if (!e->has_commander) return luaL_error(L, "prts.commander.place: no commander");
    prts_vec3 pos = check_vec3(L, 1, NULL);
    prts_world_set_parent(e->world, e->commander.entity, PRTS_ENTITY_NONE);
    prts_world_set_pos(e->world, e->commander.entity, prts_dv3_from_v3(pos));
    prts_world_skip_interp(e->world, e->commander.entity);
    e->commander.ride_frame = PRTS_ENTITY_NONE;
    if (!lua_isnoneornil(L, 2)) e->commander.yaw = (float)luaL_checknumber(L, 2);
    e->commander.vel = prts_v3(0, 0, 0);
    e->commander.vertical_speed = 0.0f;
    return 0;
}

static int l_commander_entity(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    if (!e->has_commander) return 0;
    lua_pushinteger(L, (lua_Integer)e->commander.entity);
    return 1;
}

static int l_commander_view(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    lua_pushnumber(L, e->commander.yaw);
    lua_pushnumber(L, e->commander.pitch);
    lua_pushnumber(L, e->commander.eye);
    lua_pushboolean(L, e->commander.on_ground);
    return 4;
}

/* The view's feedback state (prts_commander_feel), for scripts that play footsteps and landing
 * sounds, and for tests. */
static int l_commander_feel(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const prts_commander_feel *f = &e->commander.feel;
    lua_createtable(L, 0, 9);
    lua_pushnumber(L, f->offset);
    lua_setfield(L, -2, "offset");
    lua_pushnumber(L, f->pitch);
    lua_setfield(L, -2, "pitch");
    lua_pushnumber(L, f->trauma);
    lua_setfield(L, -2, "trauma");
    lua_pushinteger(L, (lua_Integer)f->steps);
    lua_setfield(L, -2, "steps");
    lua_pushnumber(L, f->landed);
    lua_setfield(L, -2, "landed");
    lua_pushboolean(L, f->jumped);
    lua_setfield(L, -2, "jumped");
    lua_pushnumber(L, sqrtf(e->commander.vel.x * e->commander.vel.x + e->commander.vel.z * e->commander.vel.z));
    lua_setfield(L, -2, "speed");
    lua_pushnumber(L, e->feel_motion);
    lua_setfield(L, -2, "motion");
    lua_pushboolean(L, e->has_commander && e->commander.xr.active);
    lua_setfield(L, -2, "xr");
    return 1;
}

/* prts.commander.impulse{ dip, shake }: kicks the view down (m/s) and/or shakes it (0..1). */
static int l_commander_impulse(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    float dip = field_number(L, 1, "dip", 0.0f), shake = field_number(L, 1, "shake", 0.0f);
    if (!(dip >= 0.0f && dip <= 10.0f)) return luaL_error(L, "dip must be between 0 and 10 (m/s)");
    if (!(shake >= 0.0f && shake <= 1.0f)) return luaL_error(L, "shake must be between 0 and 1");
    prts_commander_impulse(&e->commander, dip, shake);
    return 0;
}

/* prts.commander.set_feel{ motion }: how much the view moves on its own (0 off .. 1 full). */
static int l_commander_set_feel(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    float motion = field_number(L, 1, "motion", e->feel_motion);
    if (!(motion >= 0.0f && motion <= 1.0f)) return luaL_error(L, "motion must be between 0 and 1");
    e->feel_motion = motion;
    return 0;
}

/* --- prts.render ------------------------------------------------------------------ */

static int l_render_settings(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    if (lua_istable(L, 1)) {
        prts_render_settings *r = &e->render;
        const char *preset = field_string(L, 1, "preset", NULL);
        if (preset && !prts_res_preset_parse(preset, &r->preset)) return luaL_error(L, "unknown preset '%s'", preset);
        r->dither = field_bool(L, 1, "dither", r->dither);
        r->snap = field_bool(L, 1, "snap", r->snap);
        r->affine = field_bool(L, 1, "affine", r->affine);
        r->fog = field_bool(L, 1, "fog", r->fog);
        r->crosshair = field_bool(L, 1, "crosshair", r->crosshair);
        r->fov_y = field_number(L, 1, "fov", r->fov_y * 180.0f / (float)PRTS_PI) * (float)PRTS_PI / 180.0f;
    }
    lua_createtable(L, 0, 10);
    static const char *presets[] = {"authentic", "retro", "crisp", "hd", "native"};
    lua_pushstring(L, presets[PRTS_CLAMP((int)e->render.preset, 0, 4)]);
    lua_setfield(L, -2, "preset");
    lua_pushboolean(L, e->render.affine);
    lua_setfield(L, -2, "affine");
    lua_pushboolean(L, e->render.crosshair);
    lua_setfield(L, -2, "crosshair");
    lua_pushnumber(L, (lua_Number)lroundf(e->render.fov_y * 180.0f / (float)PRTS_PI));
    lua_setfield(L, -2, "fov");
    lua_pushinteger(L, prts_res_preset_lines(e->render.preset));
    lua_setfield(L, -2, "lines");
    lua_pushboolean(L, e->render.dither);
    lua_setfield(L, -2, "dither");
    lua_pushboolean(L, e->render.snap);
    lua_setfield(L, -2, "snap");
    lua_pushboolean(L, e->render.fog);
    lua_setfield(L, -2, "fog");
    lua_pushstring(L, prts_gfx_info(e->gfx));
    lua_setfield(L, -2, "renderer");
    return 1;
}

void prts_engine_open_game_api(prts_engine *e)
{
    static const prts_script_fn scene[] = {
        {"material", l_scene_material}, {"box", l_scene_box},       {"prism", l_scene_prism},
        {"terrain", l_scene_terrain},   {"hole", l_scene_hole},       {"hole_at", l_scene_hole_at},   {"height", l_scene_height}, {"light", l_scene_light},
        {"marker", l_scene_marker},     {"marker_pos", l_scene_marker_pos}, {"zone", l_scene_zone},
        {"env", l_scene_env},           {"illusion", l_scene_illusion}, {"cell", l_scene_cell},
        {"portal", l_scene_portal},     {"set_portal", l_scene_set_portal}, {"portal_open", l_scene_portal_open},
        {"cells", l_scene_cells},       {"cell_at", l_scene_cell_at},   {"surface", l_scene_surface},   {"aim_surface", l_scene_aim_surface},
        {"sprite", l_scene_sprite},     {"sprite_anim", l_scene_sprite_anim}, {"model", l_scene_model},
        {"wedge", l_scene_wedge},       {"tube", l_scene_tube},       {"in_zone", l_scene_in_zone},
        {NULL, NULL}};
    static const prts_script_fn entity[] = {
        {"spawn", l_entity_spawn},   {"pos", l_entity_pos},         {"local_pos", l_entity_local_pos},
        {"yaw", l_entity_yaw},       {"move_to", l_entity_move_to}, {"place", l_entity_place}, {"rotate_to", l_entity_rotate_to},
        {"moving", l_entity_moving}, {"set_solid", l_entity_set_solid}, {"name", l_entity_name},
        {"find", l_entity_find},     {"alive", l_entity_alive},     {"despawn", l_entity_despawn},
        {"data", l_entity_data},     {NULL, NULL}};
    static const prts_script_fn input[] = {
        {"action", l_input_action}, {"value", l_input_value}, {"down", l_input_down}, {"scheme", l_input_scheme}, {NULL, NULL}};
    static const prts_script_fn commander[] = {
        {"spawn", l_commander_spawn}, {"tune", l_commander_tune}, {"place", l_commander_place}, {"entity", l_commander_entity}, {"view", l_commander_view},
        {"feel", l_commander_feel},   {"impulse", l_commander_impulse}, {"set_feel", l_commander_set_feel},
        {NULL, NULL}};
    static const prts_script_fn render[] = {{"settings", l_render_settings}, {NULL, NULL}};
    prts_script_register(e->script, "scene", scene);
    prts_script_register(e->script, "entity", entity);
    prts_script_register(e->script, "input", input);
    prts_script_register(e->script, "commander", commander);
    prts_script_register(e->script, "render", render);
}
