/* 1stPRTS engine: retro renderer (OpenGL 3.3 core / WebGL 2) and null back end. MIT License. */
#ifndef PRTS_GFX_H
#define PRTS_GFX_H

#include "prts_scene.h"
#include "prts_vfs.h"
#include "prts_world.h"

typedef struct prts_gfx prts_gfx;
typedef void *(*prts_gfx_getproc)(const char *name);

typedef enum prts_res_preset {
    PRTS_RES_AUTHENTIC = 0, /* 180 lines */
    PRTS_RES_RETRO,         /* 270 lines (default) */
    PRTS_RES_CRISP,         /* 360 lines */
    PRTS_RES_HD,            /* 540 lines */
    PRTS_RES_NATIVE
} prts_res_preset;

typedef struct prts_render_settings {
    prts_res_preset preset;
    bool dither;   /* palette quantization + ordered dither */
    bool snap;     /* vertex snapping ("wobble") */
    bool affine;   /* affine texture mapping */
    bool fog;
    bool crosshair;
    float fov_y;   /* radians */
} prts_render_settings;

void prts_render_settings_default(prts_render_settings *s);
/* Lines of the preset; 0 for native. */
int prts_res_preset_lines(prts_res_preset p);
/* Parses "authentic", "retro", "crisp", "hd", "native"; returns false if unknown. */
bool prts_res_preset_parse(const char *name, prts_res_preset *out);

typedef struct prts_camera {
    prts_dvec3 eye;
    float yaw, pitch;
} prts_camera;

/* One view of the scene: a flat screen has one, an XR headset has one per eye. */
typedef struct prts_view {
    prts_dvec3 eye;     /* world position of the eye */
    prts_mat4 rotation; /* world -> view rotation (no translation: rendering is eye-relative) */
    float tan_left, tan_right, tan_down, tan_up; /* frustum extents at distance 1 */
    unsigned framebuffer; /* GL framebuffer to draw into; 0 is the window */
    int x, y, w, h;       /* output rectangle in that framebuffer */
} prts_view;

/* Builds the view of a flat-screen camera covering the whole w x h output. */
void prts_view_from_camera(prts_view *v, const prts_camera *cam, float fov_y, int out_w, int out_h);

/* getproc NULL creates the null back end (headless and tests): nothing is drawn. */
prts_gfx *prts_gfx_create(prts_gfx_getproc getproc, prts_vfs *vfs, bool gles);
void prts_gfx_destroy(prts_gfx *g);
bool prts_gfx_is_null(const prts_gfx *g);
/* "<renderer> / <version>" of the GL driver, or "null". */
const char *prts_gfx_info(const prts_gfx *g);
size_t prts_gfx_shader_errors(const prts_gfx *g);
/* Reloads shaders from the VFS (after a mod change or hot reload). */
void prts_gfx_reload_shaders(prts_gfx *g);

/* Internal resolution for an output size and preset (aspect preserved). */
void prts_gfx_internal_size(prts_res_preset p, int out_w, int out_h, int *w, int *h);

/* Draws one view of the scene into the window's default framebuffer. alpha interpolates
 * moving entities between ticks; time drives animated materials. */
void prts_gfx_render(prts_gfx *g, const prts_scene *scene, const prts_world *world, const prts_camera *cam,
                     const prts_render_settings *settings, int out_w, int out_h, double alpha, double time);
/* Draws several views (for example both eyes in XR) of the same frame. Each view renders at
 * the preset's internal resolution and is scaled into its own output rectangle. */
void prts_gfx_render_views(prts_gfx *g, const prts_scene *scene, const prts_world *world, const prts_view *views,
                           int view_count, const prts_render_settings *settings, double alpha, double time);
/* Number of views drawn by the last render call. */
int prts_gfx_last_view_count(const prts_gfx *g);
/* Number of surface textures (in-world displays) drawn in the last frame. */
int prts_gfx_surfaces_drawn(const prts_gfx *g);
/* Cells that were visible in the last view drawn. */
prts_cell_mask prts_gfx_last_visible_cells(const prts_gfx *g);
/* Saves the last rendered frame as PNG. */
bool prts_gfx_screenshot(prts_gfx *g, const char *real_path, int out_w, int out_h);
/* Number of mesh groups and textures currently on the GPU (for tests). */
size_t prts_gfx_mesh_count(const prts_gfx *g);
size_t prts_gfx_texture_count(const prts_gfx *g);

#endif
