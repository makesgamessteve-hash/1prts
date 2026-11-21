/* 1stPRTS engine: scene content built from script data (CPU side). MIT License. */
#ifndef PRTS_SCENE_H
#define PRTS_SCENE_H

#include "prts_collide.h"
#include "prts_math.h"
#include "prts_world.h"

typedef struct prts_vertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
    float r, g, b;
    float sun; /* baked sun visibility 0..1 (see prts_scene_bake_sun) */
    float sky; /* baked sky visibility 0..1: indoors gets less ambient light */
} prts_vertex;

typedef struct prts_mesh_cpu {
    prts_array verts;   /* prts_vertex */
    prts_array indices; /* uint32_t */
} prts_mesh_cpu;

void prts_mesh_init(prts_mesh_cpu *m);
void prts_mesh_free(prts_mesh_cpu *m);
/* Adds a quad (a, b, c, d counter-clockwise seen from the front) split into tiles of at
 * most `tile` meters, so per-vertex lighting has enough vertices to work with. */
void prts_mesh_add_quad(prts_mesh_cpu *m, prts_vec3 a, prts_vec3 b, prts_vec3 c, prts_vec3 d, prts_vec3 color,
                        float uv_scale, float tile);

#define PRTS_SCENE_NAME_MAX 48
#define PRTS_SCENE_PATH_MAX 128

typedef struct prts_material {
    char id[PRTS_SCENE_NAME_MAX];
    char texture[PRTS_SCENE_PATH_MAX];
    prts_vec3 tint;
    float uv_scale; /* meters per texture repeat */
    bool emissive;  /* ignores lighting and fog (torch flames, glowing runes) */
} prts_material;

typedef struct prts_mesh_group {
    int material;
    prts_entity owner; /* PRTS_ENTITY_NONE: static world geometry */
    prts_mesh_cpu mesh;
} prts_mesh_group;

typedef struct prts_light {
    prts_vec3 pos;
    prts_vec3 color;
    float radius;
} prts_light;

typedef struct prts_marker {
    char name[PRTS_SCENE_NAME_MAX];
    prts_dvec3 pos;
    float yaw;
} prts_marker;

typedef struct prts_zone {
    char name[PRTS_SCENE_NAME_MAX];
    prts_aabb box;
} prts_zone;

typedef struct prts_line_set {
    prts_entity owner;
    prts_vec3 color;
    prts_array points; /* prts_vec3 pairs (segments), in the owner's frame */
} prts_line_set;

typedef struct prts_scene_env {
    prts_vec3 sun_dir; /* direction towards the sun */
    prts_vec3 sun_color, ambient;
    prts_vec3 fog_color;
    float fog_near, fog_far;
    prts_vec3 sky_horizon, sky_zenith;
    float stars; /* 0..1 star visibility */
    bool moons;
} prts_scene_env;

typedef struct prts_terrain_params {
    float size;
    float spacing;
    uint64_t seed;
    float height;
    int octaves;
    int material;
} prts_terrain_params;

typedef struct prts_terrain_flat {
    float x, z, radius, height, blend;
} prts_terrain_flat;

typedef struct prts_scene {
    prts_array materials; /* prts_material */
    prts_array groups;    /* prts_mesh_group */
    prts_array lights;    /* prts_light */
    prts_array markers;   /* prts_marker */
    prts_array zones;     /* prts_zone */
    prts_array lines;     /* prts_line_set */
    prts_heightfield *terrain;
    prts_scene_env env;
    uint32_t revision; /* bumped on every change, so the renderer knows to re-upload */
} prts_scene;

prts_scene *prts_scene_create(void);
void prts_scene_destroy(prts_scene *s);
void prts_scene_clear(prts_scene *s);

/* Adds or replaces a material by id; returns its index. */
int prts_scene_material(prts_scene *s, const prts_material *m);
int prts_scene_find_material(const prts_scene *s, const char *id);
/* Mesh group for (material, owner), created on first use. */
prts_mesh_group *prts_scene_group(prts_scene *s, int material, prts_entity owner);
void prts_scene_add_box(prts_scene *s, prts_aabb box, int material, prts_entity owner, float tile);
/* Vertical prism (cylinder or cone): sides >= 3, top_radius 0 makes a cone. */
void prts_scene_add_prism(prts_scene *s, prts_vec3 base, float radius, float top_radius, float height, int sides,
                          int material, prts_entity owner);
/* Generates terrain from value noise, flattening the given areas. Replaces old terrain. */
void prts_scene_terrain(prts_scene *s, const prts_terrain_params *p, const prts_terrain_flat *flats, int nflats);
float prts_scene_height(const prts_scene *s, float x, float z);
void prts_scene_add_light(prts_scene *s, prts_light light);
void prts_scene_add_marker(prts_scene *s, const char *name, prts_dvec3 pos, float yaw);
const prts_marker *prts_scene_find_marker(const prts_scene *s, const char *name);
void prts_scene_add_zone(prts_scene *s, const char *name, prts_aabb box);
const prts_zone *prts_scene_find_zone(const prts_scene *s, const char *name);
/* Contour lines of the terrain inside area (x0,z0)-(x1,z1), scaled into a table-top
 * rectangle of the given size centred at `center` (the war table's illusion map). */
void prts_scene_add_contours(prts_scene *s, prts_entity owner, prts_vec3 center, float size_x, float size_z,
                             float relief, float x0, float z0, float x1, float z1, int levels, prts_vec3 color);
void prts_scene_env_default(prts_scene_env *env);
/* Bakes sun and sky visibility into every vertex by casting rays towards the sun and
 * straight up against the collision world (boxes and terrain). Moving entities use their
 * pose at bake time. */
void prts_scene_bake_sun(prts_scene *s, const prts_cworld *cw, const prts_world *w);

#endif
