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
    bool magic;     /* shimmering illusion surface (war table, scrying pools) */
    char surface[PRTS_SCENE_NAME_MAX]; /* live view texture from a surface camera, if set */
} prts_material;

/* Interior cells: bit 0 is "outside"; a geometry group is drawn when any of its cells is
 * visible. PRTS_CELLS_ALL marks geometry that belongs everywhere. */
typedef uint64_t prts_cell_mask;
#define PRTS_MAX_CELLS 64
#define PRTS_CELLS_ALL (~(prts_cell_mask)0)
#define PRTS_CELL_BIT(i) ((prts_cell_mask)1 << (i))

typedef struct prts_cell {
    char name[48];
    prts_aabb box; /* volume; cell 0 (outside) has none */
} prts_cell;

typedef struct prts_portal {
    char name[48];
    int a, b;      /* the two cells it connects */
    prts_aabb box; /* the opening */
    bool open;
} prts_portal;

typedef struct prts_mesh_group {
    int material;
    prts_entity owner; /* PRTS_ENTITY_NONE: static world geometry */
    prts_cell_mask cells;
    prts_mesh_cpu mesh;
} prts_mesh_group;

typedef struct prts_light {
    prts_vec3 pos;
    prts_vec3 color;
    float radius;
} prts_light;

/* A camera that renders the scene into a texture every frame (in-world displays). */
typedef struct prts_surface {
    char name[PRTS_SCENE_NAME_MAX];
    prts_dvec3 pos;
    float yaw, pitch, fov_y;
    int w, h;
    bool fog; /* scrying views usually see through the haze */
} prts_surface;

/* Billboard sprites: people and creatures drawn from a sheet with one row per (animation,
 * direction) and one column per frame. Direction 0 faces the viewer, then clockwise in
 * 45-degree steps as seen from above. */
#define PRTS_SPRITE_MAX_ANIMS 8
typedef struct prts_sprite_anim {
    char name[16];
    int first_row; /* row of direction 0; directions follow */
    int frames;
    float fps;
} prts_sprite_anim;

typedef struct prts_sprite_def {
    char path[PRTS_SCENE_PATH_MAX];    /* metadata file it was loaded from */
    char texture[PRTS_SCENE_PATH_MAX];
    int frame_w, frame_h, columns, rows;
    float height; /* meters */
    int directions;
    int anim_count;
    prts_sprite_anim anims[PRTS_SPRITE_MAX_ANIMS];
} prts_sprite_def;

typedef struct prts_sprite {
    int def;
    prts_entity owner; /* follows this entity's position and facing, if set */
    prts_vec3 pos;     /* in the owner's frame (or the world) */
    float yaw;
    int anim;
    double anim_start;
    prts_cell_mask cells;
} prts_sprite;

/* Which of `directions` directions to show for a sprite facing `facing_yaw`, seen from
 * `to_viewer` (horizontal vector from the sprite to the eye). */
int prts_sprite_direction(float facing_yaw, prts_vec3 to_viewer, int directions);
/* Animation frame at a time (looping). */
int prts_sprite_frame(const prts_sprite_anim *a, double seconds);

/* An encoded image (PNG) embedded in a model file, used as a texture under its name. */
typedef struct prts_scene_image {
    char name[PRTS_SCENE_PATH_MAX];
    char *data;
    size_t size;
} prts_scene_image;

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
    prts_cell_mask cells;
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
    prts_array surfaces; /* prts_surface */
    prts_array sprite_defs; /* prts_sprite_def */
    prts_array sprites;     /* prts_sprite */
    prts_array cells;   /* prts_cell; index 0 is outside */
    prts_array portals; /* prts_portal */
    prts_array images;  /* prts_scene_image */
    prts_cell_mask current_cells; /* cells given to geometry added from now on */
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
/* A ramp filling the box: its top rises from the box's floor at one end to its top at the
 * other, along `axis` (0 = x, 2 = z) in the direction `sign` (+1 or -1). Drawn only; scripts
 * add collision (for example as small invisible steps). */
void prts_scene_add_wedge(prts_scene *s, prts_aabb b, int axis, int sign, int material, prts_entity owner, float tile);
/* Like add_box, but each face maps the whole texture once (for display surfaces). */
void prts_scene_add_box_fitted(prts_scene *s, prts_aabb box, int material, prts_entity owner);
int prts_scene_add_surface(prts_scene *s, const prts_surface *surface);
int prts_scene_add_sprite_def(prts_scene *s, const prts_sprite_def *def);
int prts_scene_find_sprite_def(const prts_scene *s, const char *path);
int prts_scene_find_anim(const prts_sprite_def *def, const char *name);
int prts_scene_add_sprite(prts_scene *s, const prts_sprite *sprite);
int prts_scene_find_surface(const prts_scene *s, const char *name);
/* Stores a copy of an encoded image under a texture name (replacing one of the same name). */
void prts_scene_add_image(prts_scene *s, const char *name, const void *data, size_t size);
const prts_scene_image *prts_scene_find_image(const prts_scene *s, const char *name);
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

/* --- cells and portals --------------------------------------------------------------- */

/* Adds a cell (a room or a group of rooms); returns its index (1..63) or -1 when full. */
int prts_scene_add_cell(prts_scene *s, const char *name, prts_aabb volume);
/* Index of a cell by name ("outside" is 0); -1 if unknown. */
int prts_scene_find_cell(const prts_scene *s, const char *name);
const char *prts_scene_cell_name(const prts_scene *s, int cell);
/* The smallest cell whose volume contains p, or 0 (outside). */
int prts_scene_cell_at(const prts_scene *s, prts_vec3 p);
int prts_scene_add_portal(prts_scene *s, const char *name, int a, int b, prts_aabb opening, bool open);
/* Opens or closes a portal (a door); returns false for unknown names. */
bool prts_scene_set_portal(prts_scene *s, const char *name, bool open);
bool prts_scene_portal_open(const prts_scene *s, const char *name);
/* Cells visible from an eye: start in the eye's cell, then walk through open portals that
 * are inside the (shrinking) screen rectangle. view_proj is eye-relative. */
prts_cell_mask prts_scene_visible_cells(const prts_scene *s, prts_vec3 eye, prts_mat4 view_proj);
/* Cells reachable from a cell through open portals (sound travels this way). */
prts_cell_mask prts_scene_reachable_cells(const prts_scene *s, int cell);
/* Bakes sun and sky visibility into every vertex by casting rays towards the sun and
 * straight up against the collision world (boxes and terrain). Moving entities use their
 * pose at bake time. */
void prts_scene_bake_sun(prts_scene *s, const prts_cworld *cw, const prts_world *w);

#endif
