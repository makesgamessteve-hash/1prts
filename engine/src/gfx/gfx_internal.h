/* 1stPRTS engine: renderer internals shared by the files in gfx/. MIT License. */
#ifndef PRTS_GFX_INTERNAL_H
#define PRTS_GFX_INTERNAL_H

#include "gfx/gl_api.h"
#include "prts/prts_gfx.h"

#define MAX_LIGHTS 8
#define FAR_LAYER_NEAR 25.0f
#define FAR_LAYER_FAR 6000.0f
#define NEAR_LAYER_NEAR 0.05f
#define NEAR_LAYER_FAR 30.0f

typedef struct gpu_mesh {
    GLuint vao, vbo, ibo;
    GLsizei count;
    int material;
    prts_entity owner;
} gpu_mesh;

typedef struct gpu_lines {
    GLuint vao, vbo;
    GLsizei count;
    prts_vec3 color;
    prts_entity owner;
} gpu_lines;

typedef struct texture_entry {
    char path[PRTS_SCENE_PATH_MAX];
    GLuint tex;
} texture_entry;

struct prts_gfx {
    bool null;
    bool gles;
    prts_gl gl;
    prts_vfs *vfs;
    char info[256];
    size_t shader_errors;
    GLuint prog_world, prog_sky, prog_lines, prog_post, prog_fallback;
    prts_array meshes;   /* gpu_mesh */
    prts_array lines;    /* gpu_lines */
    prts_array textures; /* texture_entry */
    const prts_scene *uploaded_scene;
    uint32_t uploaded_revision;
    GLuint fbo, color_tex, depth_rb;
    int fbo_w, fbo_h;
    GLuint empty_vao;
};

/* Texture from the VFS (cached); "" is a 1x1 white texture; missing files get a placeholder. */
GLuint prts_gfx_texture(prts_gfx *g, const char *path);
/* Loads shaders/<base>.vert + .frag from the VFS (fallback program on failure). */
GLuint prts_gfx_load_program(prts_gfx *g, const char *base);
GLint prts_gfx_uniform(prts_gfx *g, GLuint program, const char *name);

#endif
