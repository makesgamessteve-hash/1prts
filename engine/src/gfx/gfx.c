/* 1stPRTS engine: retro renderer (OpenGL 3.3 core / WebGL 2) and null back end. MIT License. */
#include "prts/prts_gfx.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "gfx/gfx_internal.h"
#include "gfx/image.h"
#include "prts/prts_log.h"

void prts_render_settings_default(prts_render_settings *s)
{
    s->preset = PRTS_RES_RETRO;
    s->dither = true;
    s->snap = true;
    s->affine = false;
    s->fog = true;
    s->crosshair = true;
    s->fov_y = (float)(70.0 * PRTS_PI / 180.0);
    s->vignette = 0.0f;
}

int prts_res_preset_lines(prts_res_preset p)
{
    static const int lines[] = {180, 270, 360, 540, 0};
    return lines[PRTS_CLAMP((int)p, 0, 4)];
}

bool prts_res_preset_parse(const char *name, prts_res_preset *out)
{
    static const char *names[] = {"authentic", "retro", "crisp", "hd", "native"};
    for (int i = 0; i < 5; i++) {
        if (strcmp(name, names[i]) == 0) {
            *out = (prts_res_preset)i;
            return true;
        }
    }
    return false;
}

void prts_gfx_internal_size(prts_res_preset p, int out_w, int out_h, int *w, int *h)
{
    int lines = prts_res_preset_lines(p);
    if (lines == 0 || lines > out_h) {
        *w = PRTS_MAX(1, out_w);
        *h = PRTS_MAX(1, out_h);
        return;
    }
    *h = lines;
    *w = PRTS_MAX(1, (int)lroundf((float)lines * (float)out_w / (float)PRTS_MAX(1, out_h)));
}

bool prts_gfx_is_null(const prts_gfx *g) { return g->null; }
const char *prts_gfx_info(const prts_gfx *g) { return g->info; }
size_t prts_gfx_shader_errors(const prts_gfx *g) { return g->shader_errors; }
size_t prts_gfx_mesh_count(const prts_gfx *g) { return g->meshes.len; }
size_t prts_gfx_texture_count(const prts_gfx *g) { return g->textures.len; }

/* --- shaders ---------------------------------------------------------------- */

static const char *FALLBACK_VS =
    "in vec3 a_pos;\n"
    "uniform mat4 u_view_proj;\nuniform mat4 u_model;\n"
    "void main() { gl_Position = u_view_proj * u_model * vec4(a_pos, 1.0); }\n";
static const char *FALLBACK_FS =
    "out vec4 frag_color;\nvoid main() { frag_color = vec4(1.0, 0.0, 1.0, 1.0); }\n";

static GLuint compile(prts_gfx *g, GLenum type, const char *src, const char *name)
{
    const char *prelude = g->gles ? "#version 300 es\nprecision highp float;\nprecision highp int;\n#line 1\n"
                                  : "#version 330 core\n#line 1\n";
    const char *parts[2] = {prelude, src};
    GLuint s = g->gl.glCreateShader(type);
    g->gl.glShaderSource(s, 2, parts, NULL);
    g->gl.glCompileShader(s);
    GLint ok = 0;
    g->gl.glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        g->gl.glGetShaderInfoLog(s, (GLsizei)sizeof(log) - 1, NULL, log);
        prts_log_error("shader error in %s: %s", name, log);
        g->shader_errors++;
        g->gl.glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint link(prts_gfx *g, const char *vs_src, const char *fs_src, const char *vs_name, const char *fs_name)
{
    GLuint vs = compile(g, GL_VERTEX_SHADER, vs_src, vs_name);
    GLuint fs = compile(g, GL_FRAGMENT_SHADER, fs_src, fs_name);
    GLuint p = 0;
    if (vs && fs) {
        p = g->gl.glCreateProgram();
        g->gl.glAttachShader(p, vs);
        g->gl.glAttachShader(p, fs);
        g->gl.glBindAttribLocation(p, 0, "a_pos");
        g->gl.glBindAttribLocation(p, 1, "a_normal");
        g->gl.glBindAttribLocation(p, 2, "a_uv");
        g->gl.glBindAttribLocation(p, 3, "a_color");
        g->gl.glBindAttribLocation(p, 4, "a_sun");
        g->gl.glBindAttribLocation(p, 5, "a_sky");
        g->gl.glLinkProgram(p);
        GLint ok = 0;
        g->gl.glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024] = {0};
            g->gl.glGetProgramInfoLog(p, (GLsizei)sizeof(log) - 1, NULL, log);
            prts_log_error("shader link error in %s + %s: %s", vs_name, fs_name, log);
            g->shader_errors++;
            g->gl.glDeleteProgram(p);
            p = 0;
        }
    }
    if (vs) g->gl.glDeleteShader(vs);
    if (fs) g->gl.glDeleteShader(fs);
    return p;
}

GLuint prts_gfx_load_program(prts_gfx *g, const char *base)
{
    char vs_path[96], fs_path[96];
    snprintf(vs_path, sizeof(vs_path), "shaders/%s.vert", base);
    snprintf(fs_path, sizeof(fs_path), "shaders/%s.frag", base);
    char *vs = NULL, *fs = NULL;
    prts_vfs_read(g->vfs, vs_path, &vs, NULL);
    prts_vfs_read(g->vfs, fs_path, &fs, NULL);
    GLuint p = 0;
    if (vs && fs) {
        p = link(g, vs, fs, vs_path, fs_path);
    } else {
        prts_log_error("shader %s is missing (%s, %s)", base, vs_path, fs_path);
        g->shader_errors++;
    }
    prts_free(vs);
    prts_free(fs);
    return p ? p : g->prog_fallback;
}

static void delete_program(prts_gfx *g, GLuint *p)
{
    if (*p && *p != g->prog_fallback) g->gl.glDeleteProgram(*p);
    *p = 0;
}

void prts_gfx_reload_shaders(prts_gfx *g)
{
    if (g->null) return;
    delete_program(g, &g->prog_world);
    delete_program(g, &g->prog_sky);
    delete_program(g, &g->prog_lines);
    delete_program(g, &g->prog_post);
    g->prog_world = prts_gfx_load_program(g, "world");
    g->prog_sky = prts_gfx_load_program(g, "sky");
    g->prog_lines = prts_gfx_load_program(g, "lines");
    g->prog_post = prts_gfx_load_program(g, "post");
}

/* --- lifecycle --------------------------------------------------------------- */

prts_gfx *prts_gfx_create(prts_gfx_getproc getproc, prts_vfs *vfs, bool gles)
{
    prts_gfx *g = prts_calloc(1, sizeof(*g));
    g->vfs = vfs;
    g->gles = gles;
    prts_array_init(&g->meshes, sizeof(gpu_mesh));
    prts_array_init(&g->lines, sizeof(gpu_lines));
    prts_array_init(&g->textures, sizeof(texture_entry));
    prts_array_init(&g->surface_targets, sizeof(prts_render_target));
    prts_array_init(&g->sprite_verts, sizeof(prts_vertex));
    if (!getproc) {
        g->null = true;
        strcpy(g->info, "null");
        return g;
    }
    const char *missing = prts_gl_load(&g->gl, getproc);
    if (missing) {
        prts_log_error("OpenGL function %s is not available", missing);
        prts_free(g);
        return NULL;
    }
    snprintf(g->info, sizeof(g->info), "%s / %s", (const char *)g->gl.glGetString(GL_RENDERER),
             (const char *)g->gl.glGetString(GL_VERSION));
    g->prog_fallback = link(g, FALLBACK_VS, FALLBACK_FS, "fallback.vert", "fallback.frag");
    g->gl.glGenVertexArrays(1, &g->empty_vao);
    prts_gfx_reload_shaders(g);
    return g;
}

static void free_scene_gpu(prts_gfx *g)
{
    for (size_t i = 0; i < g->meshes.len; i++) {
        gpu_mesh *m = prts_array_at(&g->meshes, i);
        g->gl.glDeleteVertexArrays(1, &m->vao);
        g->gl.glDeleteBuffers(1, &m->vbo);
        g->gl.glDeleteBuffers(1, &m->ibo);
    }
    for (size_t i = 0; i < g->lines.len; i++) {
        gpu_lines *l = prts_array_at(&g->lines, i);
        g->gl.glDeleteVertexArrays(1, &l->vao);
        g->gl.glDeleteBuffers(1, &l->vbo);
    }
    prts_array_clear(&g->meshes);
    prts_array_clear(&g->lines);
}

void prts_gfx_destroy(prts_gfx *g)
{
    if (!g) return;
    if (!g->null) {
        free_scene_gpu(g);
        for (size_t i = 0; i < g->textures.len; i++) {
            g->gl.glDeleteTextures(1, &((texture_entry *)prts_array_at(&g->textures, i))->tex);
        }
        for (int i = 0; i < PRTS_GFX_MAX_VIEWS; i++) prts_gfx_target_free(g, &g->views[i]);
        for (size_t i = 0; i < g->surface_targets.len; i++) prts_gfx_target_free(g, prts_array_at(&g->surface_targets, i));
        delete_program(g, &g->prog_world);
        delete_program(g, &g->prog_sky);
        delete_program(g, &g->prog_lines);
        delete_program(g, &g->prog_post);
        if (g->prog_fallback) g->gl.glDeleteProgram(g->prog_fallback);
        g->gl.glDeleteVertexArrays(1, &g->empty_vao);
        if (g->sprite_vao) {
            g->gl.glDeleteVertexArrays(1, &g->sprite_vao);
            g->gl.glDeleteBuffers(1, &g->sprite_vbo);
        }
        if (g->ray_vao) {
            g->gl.glDeleteVertexArrays(1, &g->ray_vao);
            g->gl.glDeleteBuffers(1, &g->ray_vbo);
        }
    }
    prts_array_free(&g->meshes);
    prts_array_free(&g->lines);
    prts_array_free(&g->textures);
    prts_array_free(&g->surface_targets);
    prts_array_free(&g->sprite_verts);
    prts_free(g);
}

/* --- textures --------------------------------------------------------------- */

static GLuint upload_texture(prts_gfx *g, const unsigned char *rgba, int w, int h)
{
    GLuint t;
    g->gl.glGenTextures(1, &t);
    g->gl.glBindTexture(GL_TEXTURE_2D, t);
    g->gl.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    g->gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return t;
}

static texture_entry *find_texture(prts_gfx *g, const char *path)
{
    for (size_t i = 0; i < g->textures.len; i++) {
        texture_entry *e = prts_array_at(&g->textures, i);
        if (strcmp(e->path, path) == 0) return e;
    }
    return NULL;
}

/* Decodes an encoded image into a new cache entry, or a magenta checker placeholder. */
static GLuint add_texture(prts_gfx *g, const char *path, const void *data, size_t size)
{
    texture_entry e;
    memset(&e, 0, sizeof(e));
    strncpy(e.path, path, sizeof(e.path) - 1);
    int w = 0, h = 0;
    unsigned char *px = data ? prts_image_decode(data, size, &w, &h) : NULL;
    if (px) {
        e.tex = upload_texture(g, px, w, h);
        prts_image_free(px);
    } else {
        /* Spec: Missing model/asset -> clearly visible placeholder */
        prts_log_error("texture '%s' is missing or unreadable; using placeholder", path);
        unsigned char checker[8 * 8 * 4];
        for (int i = 0; i < 64; i++) {
            bool on = ((i % 8) / 2 + (i / 8) / 2) % 2 == 0;
            checker[i * 4 + 0] = on ? 255 : 20;
            checker[i * 4 + 1] = 0;
            checker[i * 4 + 2] = on ? 255 : 20;
            checker[i * 4 + 3] = 255;
        }
        e.tex = upload_texture(g, checker, 8, 8);
    }
    prts_array_push(&g->textures, &e);
    return e.tex;
}

GLuint prts_gfx_texture(prts_gfx *g, const char *path)
{
    texture_entry *found = find_texture(g, path);
    if (found) return found->tex;
    if (!*path) {
        static const unsigned char white[4] = {255, 255, 255, 255};
        texture_entry e;
        memset(&e, 0, sizeof(e));
        e.tex = upload_texture(g, white, 1, 1);
        prts_array_push(&g->textures, &e);
        return e.tex;
    }
    char *data = NULL;
    size_t size = 0;
    GLuint tex;
    if (prts_vfs_read(g->vfs, path, &data, &size) == PRTS_VFS_OK) tex = add_texture(g, path, data, size);
    else tex = add_texture(g, path, NULL, 0);
    prts_free(data);
    return tex;
}

/* --- scene upload ------------------------------------------------------------- */

/* Attribute layout of prts_vertex for the bound VAO/VBO. */
static void set_vertex_layout(prts_gfx *g)
{
    GLsizei stride = (GLsizei)sizeof(prts_vertex);
    g->gl.glEnableVertexAttribArray(0);
    g->gl.glVertexAttribPointer(0, 3, GL_FLOAT, 0, stride, (const void *)0);
    g->gl.glEnableVertexAttribArray(1);
    g->gl.glVertexAttribPointer(1, 3, GL_FLOAT, 0, stride, (const void *)(3 * sizeof(float)));
    g->gl.glEnableVertexAttribArray(2);
    g->gl.glVertexAttribPointer(2, 2, GL_FLOAT, 0, stride, (const void *)(6 * sizeof(float)));
    g->gl.glEnableVertexAttribArray(3);
    g->gl.glVertexAttribPointer(3, 3, GL_FLOAT, 0, stride, (const void *)(8 * sizeof(float)));
    g->gl.glEnableVertexAttribArray(4);
    g->gl.glVertexAttribPointer(4, 1, GL_FLOAT, 0, stride, (const void *)(11 * sizeof(float)));
    g->gl.glEnableVertexAttribArray(5);
    g->gl.glVertexAttribPointer(5, 1, GL_FLOAT, 0, stride, (const void *)(12 * sizeof(float)));
}

static void upload_scene(prts_gfx *g, const prts_scene *s)
{
    free_scene_gpu(g);
    for (size_t i = 0; i < s->groups.len; i++) {
        const prts_mesh_group *grp = prts_array_at(&s->groups, i);
        if (grp->mesh.indices.len == 0) continue;
        gpu_mesh m = {0, 0, 0, (GLsizei)grp->mesh.indices.len, grp->material, grp->owner, grp->cells};
        g->gl.glGenVertexArrays(1, &m.vao);
        g->gl.glBindVertexArray(m.vao);
        g->gl.glGenBuffers(1, &m.vbo);
        g->gl.glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        g->gl.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(grp->mesh.verts.len * sizeof(prts_vertex)),
                           grp->mesh.verts.data, GL_STATIC_DRAW);
        g->gl.glGenBuffers(1, &m.ibo);
        g->gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
        g->gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(grp->mesh.indices.len * sizeof(uint32_t)),
                           grp->mesh.indices.data, GL_STATIC_DRAW);
        set_vertex_layout(g);
        prts_array_push(&g->meshes, &m);
    }
    for (size_t i = 0; i < s->lines.len; i++) {
        const prts_line_set *ls = prts_array_at(&s->lines, i);
        if (ls->points.len == 0) continue;
        gpu_lines l = {0, 0, (GLsizei)ls->points.len, ls->color, ls->owner, ls->cells};
        g->gl.glGenVertexArrays(1, &l.vao);
        g->gl.glBindVertexArray(l.vao);
        g->gl.glGenBuffers(1, &l.vbo);
        g->gl.glBindBuffer(GL_ARRAY_BUFFER, l.vbo);
        g->gl.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(ls->points.len * sizeof(prts_vec3)), ls->points.data,
                           GL_STATIC_DRAW);
        g->gl.glEnableVertexAttribArray(0);
        g->gl.glVertexAttribPointer(0, 3, GL_FLOAT, 0, (GLsizei)sizeof(prts_vec3), (const void *)0);
        prts_array_push(&g->lines, &l);
    }
    g->gl.glBindVertexArray(0);
    for (size_t i = 0; i < s->images.len; i++) { /* textures embedded in model files */
        const prts_scene_image *im = prts_array_at(&s->images, i);
        if (!find_texture(g, im->name)) add_texture(g, im->name, im->data, im->size);
    }
    for (size_t i = 0; i < s->materials.len; i++) prts_gfx_texture(g, ((prts_material *)prts_array_at(&s->materials, i))->texture);
    for (size_t i = 0; i < s->sprite_defs.len; i++) prts_gfx_texture(g, ((prts_sprite_def *)prts_array_at(&s->sprite_defs, i))->texture);
    g->uploaded_scene = s;
    g->uploaded_revision = s->revision;
}

void prts_gfx_target_free(prts_gfx *g, prts_render_target *t)
{
    if (!t->fbo) return;
    g->gl.glDeleteFramebuffers(1, &t->fbo);
    g->gl.glDeleteTextures(1, &t->color);
    g->gl.glDeleteRenderbuffers(1, &t->depth);
    t->fbo = t->color = t->depth = 0;
    t->w = t->h = 0;
}

void prts_gfx_target_ensure(prts_gfx *g, prts_render_target *t, int w, int h)
{
    if (t->fbo && t->w == w && t->h == h) return;
    prts_gfx_target_free(g, t);
    g->gl.glGenTextures(1, &t->color);
    g->gl.glBindTexture(GL_TEXTURE_2D, t->color);
    g->gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    g->gl.glGenRenderbuffers(1, &t->depth);
    g->gl.glBindRenderbuffer(GL_RENDERBUFFER, t->depth);
    g->gl.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    g->gl.glGenFramebuffers(1, &t->fbo);
    g->gl.glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    g->gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->color, 0);
    g->gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t->depth);
    if (g->gl.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        prts_log_error("render target %dx%d is incomplete", w, h);
    }
    t->w = w;
    t->h = h;
}

/* --- drawing ---------------------------------------------------------------- */

GLint prts_gfx_uniform(prts_gfx *g, GLuint p, const char *name) { return g->gl.glGetUniformLocation(p, name); }

static void set_vec3(prts_gfx *g, GLuint p, const char *name, prts_vec3 v)
{
    g->gl.glUniform3f(prts_gfx_uniform(g, p, name), v.x, v.y, v.z);
}

typedef struct frame_ctx {
    const prts_scene *scene;
    const prts_world *world;
    const prts_view *view;
    const prts_render_settings *rs;
    double alpha;
    int iw, ih;
    prts_cell_mask visible;
    double time;
    int skip_surface; /* surface being drawn (its own display is skipped), or -1 */
} frame_ctx;

static prts_mat4 owner_model(const frame_ctx *f, prts_entity owner)
{
    if (owner == PRTS_ENTITY_NONE) return prts_mat4_translate(prts_dv3_relative(prts_dv3(0, 0, 0), f->view->eye));
    prts_dvec3 p = prts_world_pos_interp(f->world, owner, f->alpha);
    float yaw = prts_world_yaw_interp(f->world, owner, f->alpha);
    return prts_mat4_mul(prts_mat4_translate(prts_dv3_relative(p, f->view->eye)), prts_mat4_rotate_y(yaw));
}

static void draw_sky(prts_gfx *g, const frame_ctx *f)
{
    GLuint p = g->prog_sky;
    g->gl.glUseProgram(p);
    const prts_mat4 *r = &f->view->rotation;
    /* the rows of the world->view rotation are the camera's axes in world space */
    set_vec3(g, p, "u_right", prts_v3(r->m[0], r->m[4], r->m[8]));
    set_vec3(g, p, "u_up", prts_v3(r->m[1], r->m[5], r->m[9]));
    set_vec3(g, p, "u_forward", prts_v3(-r->m[2], -r->m[6], -r->m[10]));
    g->gl.glUniform4f(prts_gfx_uniform(g, p, "u_tan"), f->view->tan_left, f->view->tan_right, f->view->tan_down,
                      f->view->tan_up);
    const prts_scene_env *env = &f->scene->env;
    set_vec3(g, p, "u_horizon", env->sky_horizon);
    set_vec3(g, p, "u_zenith", env->sky_zenith);
    set_vec3(g, p, "u_sun_dir", prts_v3_normalize(env->sun_dir));
    set_vec3(g, p, "u_sun_color", env->sun_color);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_stars"), env->stars);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_moons"), env->moons ? 1.0f : 0.0f);
    g->gl.glBindVertexArray(g->empty_vao);
    g->gl.glDrawArrays(GL_TRIANGLES, 0, 3);
}

static int pick_lights(const frame_ctx *f, prts_light *out)
{
    int n = 0;
    const prts_array *lights = &f->scene->lights;
    float best_d[MAX_LIGHTS];
    prts_vec3 eye = prts_v3((float)f->view->eye.x, (float)f->view->eye.y, (float)f->view->eye.z);
    for (size_t i = 0; i < lights->len; i++) {
        const prts_light *l = prts_array_at(lights, i);
        float d = prts_v3_length(prts_v3_sub(l->pos, eye)) - l->radius;
        int slot = n < MAX_LIGHTS ? n++ : -1;
        if (slot < 0) {
            for (int k = 0; k < MAX_LIGHTS; k++) {
                if (best_d[k] > d && (slot < 0 || best_d[k] > best_d[slot])) slot = k;
            }
            if (slot < 0) continue;
        }
        best_d[slot] = d;
        out[slot] = *l;
    }
    return n;
}

static void draw_meshes(prts_gfx *g, const frame_ctx *f, prts_mat4 view_proj)
{
    GLuint p = g->prog_world;
    g->gl.glUseProgram(p);
    g->gl.glUniformMatrix4fv(prts_gfx_uniform(g, p, "u_view_proj"), 1, 0, view_proj.m);
    const prts_scene_env *env = &f->scene->env;
    set_vec3(g, p, "u_sun_dir", prts_v3_normalize(env->sun_dir));
    set_vec3(g, p, "u_sun_color", env->sun_color);
    set_vec3(g, p, "u_ambient", env->ambient);
    set_vec3(g, p, "u_fog_color", env->fog_color);
    g->gl.glUniform2f(prts_gfx_uniform(g, p, "u_fog_range"), env->fog_near, f->rs->fog ? env->fog_far : 1e9f);
    g->gl.glUniform2f(prts_gfx_uniform(g, p, "u_snap"), f->rs->snap ? (float)f->iw * 0.5f : 0.0f, (float)f->ih * 0.5f);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_affine"), f->rs->affine ? 1.0f : 0.0f);
    prts_light lights[MAX_LIGHTS];
    int n = pick_lights(f, lights);
    float pos[MAX_LIGHTS * 3] = {0}, col[MAX_LIGHTS * 3] = {0}, rad[MAX_LIGHTS] = {0};
    for (int i = 0; i < n; i++) {
        prts_vec3 rel = prts_dv3_relative(prts_dv3_from_v3(lights[i].pos), f->view->eye);
        pos[i * 3] = rel.x;
        pos[i * 3 + 1] = rel.y;
        pos[i * 3 + 2] = rel.z;
        col[i * 3] = lights[i].color.x;
        col[i * 3 + 1] = lights[i].color.y;
        col[i * 3 + 2] = lights[i].color.z;
        rad[i] = lights[i].radius;
    }
    g->gl.glUniform3fv(prts_gfx_uniform(g, p, "u_light_pos"), MAX_LIGHTS, pos);
    g->gl.glUniform3fv(prts_gfx_uniform(g, p, "u_light_color"), MAX_LIGHTS, col);
    g->gl.glUniform1fv(prts_gfx_uniform(g, p, "u_light_radius"), MAX_LIGHTS, rad);
    g->gl.glUniform1i(prts_gfx_uniform(g, p, "u_light_count"), n);
    g->gl.glUniform1i(prts_gfx_uniform(g, p, "u_tex"), 0);
    g->gl.glActiveTexture(GL_TEXTURE0);
    GLint model_loc = prts_gfx_uniform(g, p, "u_model"), emissive_loc = prts_gfx_uniform(g, p, "u_emissive");
    GLint magic_loc = prts_gfx_uniform(g, p, "u_magic"), flip_loc = prts_gfx_uniform(g, p, "u_flip_v");
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_time"), (float)fmod(f->time, 1000.0));
    for (size_t i = 0; i < g->meshes.len; i++) {
        gpu_mesh *m = prts_array_at(&g->meshes, i);
        if (!(m->cells & f->visible)) continue;
        if (m->owner != PRTS_ENTITY_NONE && !prts_world_alive(f->world, m->owner)) continue;
        const prts_material *mat = prts_array_at(&f->scene->materials, (size_t)m->material);
        int surface = mat && mat->surface[0] ? prts_scene_find_surface(f->scene, mat->surface) : -1;
        if (surface >= 0 && surface == f->skip_surface) continue; /* no display inside its own picture */
        prts_render_target *rt = surface >= 0 ? prts_array_at(&g->surface_targets, (size_t)surface) : NULL;
        g->gl.glBindTexture(GL_TEXTURE_2D, rt && rt->fbo ? rt->color : prts_gfx_texture(g, mat ? mat->texture : ""));
        g->gl.glUniform1f(emissive_loc, mat && mat->emissive ? 1.0f : 0.0f);
        g->gl.glUniform1f(magic_loc, mat && mat->magic ? 1.0f : 0.0f);
        g->gl.glUniform1f(flip_loc, rt && rt->fbo ? 1.0f : 0.0f);
        prts_mat4 model = owner_model(f, m->owner);
        g->gl.glUniformMatrix4fv(model_loc, 1, 0, model.m);
        g->gl.glBindVertexArray(m->vao);
        g->gl.glDrawElements(GL_TRIANGLES, m->count, GL_UNSIGNED_INT, NULL);
    }
}

int prts_gfx_sprites_drawn(const prts_gfx *g) { return g->sprites_drawn; }

static void push_sprite_vertex(prts_array *out, prts_vec3 p, prts_vec3 n, float u, float v, float sun)
{
    prts_vertex vx = {p.x, p.y, p.z, n.x, n.y, n.z, u, v, 1.0f, 1.0f, 1.0f, sun, 1.0f}; /* full ambient light */
    prts_array_push(out, &vx);
}

/* Camera-facing (around the vertical axis) quads for every visible sprite. Uses the world
 * program state set up by draw_meshes (same view, lights and fog). */
static void draw_sprites(prts_gfx *g, const frame_ctx *f)
{
    const prts_scene *s = f->scene;
    if (s->sprites.len == 0) return;
    if (!g->sprite_vao) {
        g->gl.glGenVertexArrays(1, &g->sprite_vao);
        g->gl.glGenBuffers(1, &g->sprite_vbo);
        g->gl.glBindVertexArray(g->sprite_vao);
        g->gl.glBindBuffer(GL_ARRAY_BUFFER, g->sprite_vbo);
        set_vertex_layout(g);
    }
    const prts_mat4 *r = &f->view->rotation;
    prts_vec3 right = prts_v3_normalize(prts_v3(r->m[0], 0.0f, r->m[8]));
    prts_vec3 normal = prts_v3(-right.z, 0.0f, right.x); /* towards the viewer */
    GLuint p = g->prog_world;
    g->gl.glUniformMatrix4fv(prts_gfx_uniform(g, p, "u_model"), 1, 0, prts_mat4_identity().m);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_emissive"), 0.0f);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_magic"), 0.0f);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_flip_v"), 0.0f);
    g->gl.glBindVertexArray(g->sprite_vao);
    g->gl.glBindBuffer(GL_ARRAY_BUFFER, g->sprite_vbo);
    int drawn = 0;
    for (size_t d = 0; d < s->sprite_defs.len; d++) {
        const prts_sprite_def *def = prts_array_at(&s->sprite_defs, d);
        prts_array_clear(&g->sprite_verts);
        for (size_t i = 0; i < s->sprites.len; i++) {
            const prts_sprite *sp = prts_array_at(&s->sprites, i);
            if (sp->def != (int)d || !(sp->cells & f->visible)) continue;
            prts_dvec3 base = prts_dv3_from_v3(sp->pos);
            float yaw = sp->yaw;
            if (sp->owner != PRTS_ENTITY_NONE) {
                if (!prts_world_alive(f->world, sp->owner)) continue;
                float oy = prts_world_yaw_interp(f->world, sp->owner, f->alpha);
                base = prts_dv3_add(prts_world_pos_interp(f->world, sp->owner, f->alpha),
                                    prts_dv3_from_v3(prts_v3_rotate_y(sp->pos, oy)));
                yaw += oy;
            }
            prts_vec3 rel = prts_dv3_relative(base, f->view->eye);
            int dir = prts_sprite_direction(yaw, prts_v3(-rel.x, 0.0f, -rel.z), def->directions);
            const prts_sprite_anim *a = &def->anims[PRTS_CLAMP(sp->anim, 0, def->anim_count - 1)];
            int col = prts_sprite_frame(a, f->time - sp->anim_start), row = a->first_row + dir;
            float u0 = (float)col / (float)def->columns, u1 = (float)(col + 1) / (float)def->columns;
            float v0 = (float)row / (float)def->rows, v1 = (float)(row + 1) / (float)def->rows;
            float h = def->height, w = h * (float)def->frame_w / (float)def->frame_h;
            prts_vec3 bl = prts_v3_sub(rel, prts_v3_scale(right, w * 0.5f)), br = prts_v3_add(rel, prts_v3_scale(right, w * 0.5f));
            prts_vec3 tl = prts_v3(bl.x, bl.y + h, bl.z), tr = prts_v3(br.x, br.y + h, br.z);
            float sun = (sp->cells & PRTS_CELL_BIT(0)) ? 1.0f : 0.0f;
            push_sprite_vertex(&g->sprite_verts, bl, normal, u0, v1, sun);
            push_sprite_vertex(&g->sprite_verts, br, normal, u1, v1, sun);
            push_sprite_vertex(&g->sprite_verts, tr, normal, u1, v0, sun);
            push_sprite_vertex(&g->sprite_verts, bl, normal, u0, v1, sun);
            push_sprite_vertex(&g->sprite_verts, tr, normal, u1, v0, sun);
            push_sprite_vertex(&g->sprite_verts, tl, normal, u0, v0, sun);
            drawn++;
        }
        if (g->sprite_verts.len == 0) continue;
        g->gl.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(g->sprite_verts.len * sizeof(prts_vertex)), g->sprite_verts.data,
                           GL_DYNAMIC_DRAW);
        g->gl.glBindTexture(GL_TEXTURE_2D, prts_gfx_texture(g, def->texture));
        g->gl.glDisable(GL_CULL_FACE);
        g->gl.glDrawArrays(GL_TRIANGLES, 0, (GLsizei)g->sprite_verts.len);
        g->gl.glEnable(GL_CULL_FACE);
    }
    g->sprites_drawn = drawn;
}

static void draw_lines(prts_gfx *g, const frame_ctx *f, prts_mat4 view_proj, double time)
{
    if (g->lines.len == 0) return;
    GLuint p = g->prog_lines;
    g->gl.glUseProgram(p);
    g->gl.glUniformMatrix4fv(prts_gfx_uniform(g, p, "u_view_proj"), 1, 0, view_proj.m);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_time"), (float)fmod(time, 1000.0));
    g->gl.glEnable(GL_BLEND);
    g->gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    g->gl.glDepthMask(0);
    for (size_t i = 0; i < g->lines.len; i++) {
        gpu_lines *l = prts_array_at(&g->lines, i);
        if (!(l->cells & f->visible)) continue;
        if (l->owner != PRTS_ENTITY_NONE && !prts_world_alive(f->world, l->owner)) continue;
        prts_mat4 model = owner_model(f, l->owner);
        g->gl.glUniformMatrix4fv(prts_gfx_uniform(g, p, "u_model"), 1, 0, model.m);
        set_vec3(g, p, "u_color", l->color);
        g->gl.glBindVertexArray(l->vao);
        g->gl.glDrawArrays(GL_LINES, 0, l->count);
    }
    g->gl.glDepthMask(1);
    g->gl.glDisable(GL_BLEND);
}

void prts_gfx_set_rays(prts_gfx *g, const prts_gfx_ray *rays, int count)
{
    g->ray_count = PRTS_CLAMP(count, 0, PRTS_GFX_MAX_RAYS);
    if (g->ray_count) memcpy(g->rays, rays, sizeof(*rays) * (size_t)g->ray_count);
}

int prts_gfx_rays_drawn(const prts_gfx *g) { return g->rays_drawn; }

/* The rays as additive lines, eye-relative like the rest of the view. */
static void draw_rays(prts_gfx *g, const frame_ctx *f, prts_mat4 view_proj, double time)
{
    if (f->skip_surface >= 0) return;
    g->rays_drawn = g->ray_count;
    if (g->ray_count == 0) return;
    prts_vec3 pts[PRTS_GFX_MAX_RAYS * 2];
    for (int i = 0; i < g->ray_count; i++) {
        pts[i * 2] = prts_dv3_relative(g->rays[i].from, f->view->eye);
        pts[i * 2 + 1] = prts_dv3_relative(g->rays[i].to, f->view->eye);
    }
    if (!g->ray_vao) {
        g->gl.glGenVertexArrays(1, &g->ray_vao);
        g->gl.glGenBuffers(1, &g->ray_vbo);
        g->gl.glBindVertexArray(g->ray_vao);
        g->gl.glBindBuffer(GL_ARRAY_BUFFER, g->ray_vbo);
        g->gl.glEnableVertexAttribArray(0);
        g->gl.glVertexAttribPointer(0, 3, GL_FLOAT, 0, (GLsizei)sizeof(prts_vec3), (const void *)0);
    }
    g->gl.glBindVertexArray(g->ray_vao);
    g->gl.glBindBuffer(GL_ARRAY_BUFFER, g->ray_vbo);
    g->gl.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(prts_vec3) * 2 * (size_t)g->ray_count), pts, GL_DYNAMIC_DRAW);
    GLuint p = g->prog_lines;
    g->gl.glUseProgram(p);
    prts_mat4 id = prts_mat4_identity();
    g->gl.glUniformMatrix4fv(prts_gfx_uniform(g, p, "u_view_proj"), 1, 0, view_proj.m);
    g->gl.glUniformMatrix4fv(prts_gfx_uniform(g, p, "u_model"), 1, 0, id.m);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_time"), (float)fmod(time, 1000.0));
    g->gl.glEnable(GL_BLEND);
    g->gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    g->gl.glDepthMask(0);
    for (int i = 0; i < g->ray_count; i++) {
        set_vec3(g, p, "u_color", g->rays[i].color);
        g->gl.glDrawArrays(GL_LINES, i * 2, 2);
    }
    g->gl.glDepthMask(1);
    g->gl.glDisable(GL_BLEND);
    g->gl.glBindVertexArray(0);
}

void prts_view_from_camera(prts_view *v, const prts_camera *cam, float fov_y, int out_w, int out_h)
{
    float t = tanf(fov_y * 0.5f);
    float aspect = (float)PRTS_MAX(1, out_w) / (float)PRTS_MAX(1, out_h);
    v->eye = cam->eye;
    v->rotation = prts_mat4_view_yaw_pitch(cam->yaw, cam->pitch);
    v->tan_left = -t * aspect;
    v->tan_right = t * aspect;
    v->tan_down = -t;
    v->tan_up = t;
    v->framebuffer = 0;
    v->x = v->y = 0;
    v->w = out_w;
    v->h = out_h;
}

int prts_gfx_last_view_count(const prts_gfx *g) { return g->last_views; }
prts_cell_mask prts_gfx_last_visible_cells(const prts_gfx *g) { return g->last_visible; }

/* Draws the scene for one view into its low-resolution target. */
static void draw_view_scene(prts_gfx *g, frame_ctx *f, prts_render_target *t, double time)
{
    g->gl.glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    g->gl.glViewport(0, 0, t->w, t->h);
    g->gl.glDisable(GL_DEPTH_TEST);
    g->gl.glDisable(GL_CULL_FACE);
    draw_sky(g, f);
    g->gl.glEnable(GL_DEPTH_TEST);
    g->gl.glDepthFunc(GL_LEQUAL);
    g->gl.glEnable(GL_CULL_FACE);
    g->gl.glCullFace(GL_BACK);
    const prts_view *v = f->view;
    prts_mat4 cull = prts_mat4_mul(prts_mat4_frustum(v->tan_left, v->tan_right, v->tan_down, v->tan_up, NEAR_LAYER_NEAR,
                                                     FAR_LAYER_FAR),
                                   v->rotation);
    f->visible = prts_scene_visible_cells(f->scene, prts_v3((float)v->eye.x, (float)v->eye.y, (float)v->eye.z), cull);
    g->last_visible = f->visible;
    /* Two depth layers: far geometry first, then a fresh depth range for nearby geometry. */
    const float layers[2][2] = {{FAR_LAYER_NEAR, FAR_LAYER_FAR}, {NEAR_LAYER_NEAR, NEAR_LAYER_FAR}};
    prts_mat4 view_proj = v->rotation;
    for (int l = 0; l < 2; l++) {
        g->gl.glClear(GL_DEPTH_BUFFER_BIT);
        prts_mat4 proj = prts_mat4_frustum(v->tan_left, v->tan_right, v->tan_down, v->tan_up, layers[l][0], layers[l][1]);
        view_proj = prts_mat4_mul(proj, v->rotation);
        draw_meshes(g, f, view_proj);
        draw_sprites(g, f);
    }
    draw_lines(g, f, view_proj, time);
    draw_rays(g, f, view_proj, time);
}

/* Scales a view's low-resolution image into its output rectangle. */
static void present_view(prts_gfx *g, const prts_view *v, const prts_render_target *t, const prts_render_settings *rs)
{
    g->gl.glBindFramebuffer(GL_FRAMEBUFFER, v->framebuffer);
    g->gl.glDisable(GL_DEPTH_TEST);
    g->gl.glDisable(GL_CULL_FACE);
    g->gl.glEnable(GL_SCISSOR_TEST);
    g->gl.glScissor(v->x, v->y, v->w, v->h);
    g->gl.glViewport(v->x, v->y, v->w, v->h);
    g->gl.glClearColor(0, 0, 0, 1);
    g->gl.glClear(GL_COLOR_BUFFER_BIT);
    g->gl.glDisable(GL_SCISSOR_TEST);
    /* whole-number scaling keeps pixels even; fall back to a fractional fit when a whole
     * number would leave more than 10% of the output empty */
    float fit = fminf((float)v->w / (float)t->w, (float)v->h / (float)t->h);
    float scale = floorf(fit);
    if (scale < 1.0f || scale / fit < 0.9f) scale = fit;
    int vw = (int)((float)t->w * scale), vh = (int)((float)t->h * scale);
    g->gl.glViewport(v->x + (v->w - vw) / 2, v->y + (v->h - vh) / 2, vw, vh);
    GLuint p = g->prog_post;
    g->gl.glUseProgram(p);
    g->gl.glActiveTexture(GL_TEXTURE0);
    g->gl.glBindTexture(GL_TEXTURE_2D, t->color);
    g->gl.glUniform1i(prts_gfx_uniform(g, p, "u_scene"), 0);
    g->gl.glUniform2f(prts_gfx_uniform(g, p, "u_size"), (float)t->w, (float)t->h);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_dither"), rs->dither ? 1.0f : 0.0f);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_crosshair"), rs->crosshair ? 1.0f : 0.0f);
    /* the comfort vignette centres on the view's optical axis (off-centre for XR eyes) */
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_vignette"), prts_clampf(rs->vignette, 0.0f, 1.0f));
    g->gl.glUniform2f(prts_gfx_uniform(g, p, "u_center"), -v->tan_left / (v->tan_right - v->tan_left),
                      -v->tan_down / (v->tan_up - v->tan_down));
    g->gl.glBindVertexArray(g->empty_vao);
    g->gl.glDrawArrays(GL_TRIANGLES, 0, 3);
    g->gl.glBindVertexArray(0);
}

int prts_gfx_surfaces_drawn(const prts_gfx *g) { return g->surfaces_drawn; }

/* Renders every surface camera into its texture (in-world displays, before the views). */
static void draw_surfaces(prts_gfx *g, const prts_scene *scene, const prts_world *world,
                          const prts_render_settings *rs, double alpha, double time)
{
    while (g->surface_targets.len < scene->surfaces.len) prts_array_push(&g->surface_targets, NULL);
    g->surfaces_drawn = 0;
    for (size_t i = 0; i < scene->surfaces.len; i++) {
        const prts_surface *sf = prts_array_at(&scene->surfaces, i);
        prts_camera cam = {sf->pos, sf->yaw, sf->pitch};
        prts_view v;
        prts_view_from_camera(&v, &cam, sf->fov_y, sf->w, sf->h);
        prts_render_target *rt = prts_array_at(&g->surface_targets, i);
        prts_gfx_target_ensure(g, rt, sf->w, sf->h);
        prts_render_settings srs = *rs;
        srs.fog = sf->fog;
        frame_ctx f = {scene, world, &v, &srs, alpha, sf->w, sf->h, PRTS_CELLS_ALL, time, (int)i};
        draw_view_scene(g, &f, rt, time);
        g->surfaces_drawn++;
    }
}

void prts_gfx_render_views(prts_gfx *g, const prts_scene *scene, const prts_world *world, const prts_view *views,
                           int view_count, const prts_render_settings *rs, double alpha, double time)
{
    view_count = PRTS_CLAMP(view_count, 0, PRTS_GFX_MAX_VIEWS);
    g->last_views = view_count;
    if (g->null) return;
    if (g->uploaded_scene != scene || g->uploaded_revision != scene->revision) upload_scene(g, scene);
    draw_surfaces(g, scene, world, rs, alpha, time);
    for (int i = 0; i < view_count; i++) {
        frame_ctx f = {scene, world, &views[i], rs, alpha, 0, 0, PRTS_CELLS_ALL, time, -1};
        prts_gfx_internal_size(rs->preset, views[i].w, views[i].h, &f.iw, &f.ih);
        prts_gfx_target_ensure(g, &g->views[i], f.iw, f.ih);
        draw_view_scene(g, &f, &g->views[i], time);
        present_view(g, &views[i], &g->views[i], rs);
    }
}

void prts_gfx_render(prts_gfx *g, const prts_scene *scene, const prts_world *world, const prts_camera *cam,
                     const prts_render_settings *rs, int out_w, int out_h, double alpha, double time)
{
    prts_view v;
    prts_view_from_camera(&v, cam, rs->fov_y, out_w, out_h);
    prts_gfx_render_views(g, scene, world, &v, 1, rs, alpha, time);
}

bool prts_gfx_screenshot(prts_gfx *g, const char *real_path, int out_w, int out_h)
{
    if (g->null) return false;
    size_t row = (size_t)out_w * 4;
    unsigned char *px = prts_malloc(row * (size_t)out_h);
    unsigned char *flipped = prts_malloc(row * (size_t)out_h);
    g->gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    g->gl.glPixelStorei(GL_PACK_ALIGNMENT, 1);
    g->gl.glReadPixels(0, 0, out_w, out_h, GL_RGBA, GL_UNSIGNED_BYTE, px);
    for (int y = 0; y < out_h; y++) memcpy(flipped + (size_t)y * row, px + (size_t)(out_h - 1 - y) * row, row);
    for (size_t i = 3; i < row * (size_t)out_h; i += 4) flipped[i] = 255;
    bool ok = prts_image_write_png(real_path, flipped, out_w, out_h);
    prts_free(px);
    prts_free(flipped);
    return ok;
}
