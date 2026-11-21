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
        if (g->fbo) {
            g->gl.glDeleteFramebuffers(1, &g->fbo);
            g->gl.glDeleteTextures(1, &g->color_tex);
            g->gl.glDeleteRenderbuffers(1, &g->depth_rb);
        }
        delete_program(g, &g->prog_world);
        delete_program(g, &g->prog_sky);
        delete_program(g, &g->prog_lines);
        delete_program(g, &g->prog_post);
        if (g->prog_fallback) g->gl.glDeleteProgram(g->prog_fallback);
        g->gl.glDeleteVertexArrays(1, &g->empty_vao);
    }
    prts_array_free(&g->meshes);
    prts_array_free(&g->lines);
    prts_array_free(&g->textures);
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

GLuint prts_gfx_texture(prts_gfx *g, const char *path)
{
    for (size_t i = 0; i < g->textures.len; i++) {
        texture_entry *e = prts_array_at(&g->textures, i);
        if (strcmp(e->path, path) == 0) return e->tex;
    }
    texture_entry e;
    memset(&e, 0, sizeof(e));
    strncpy(e.path, path, sizeof(e.path) - 1);
    if (!*path) {
        static const unsigned char white[4] = {255, 255, 255, 255};
        e.tex = upload_texture(g, white, 1, 1);
    } else {
        char *data = NULL;
        size_t size = 0;
        int w = 0, h = 0;
        unsigned char *px = NULL;
        if (prts_vfs_read(g->vfs, path, &data, &size) == PRTS_VFS_OK) px = prts_image_decode(data, size, &w, &h);
        prts_free(data);
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
    }
    prts_array_push(&g->textures, &e);
    return e.tex;
}

/* --- scene upload ------------------------------------------------------------- */

static void upload_scene(prts_gfx *g, const prts_scene *s)
{
    free_scene_gpu(g);
    for (size_t i = 0; i < s->groups.len; i++) {
        const prts_mesh_group *grp = prts_array_at(&s->groups, i);
        if (grp->mesh.indices.len == 0) continue;
        gpu_mesh m = {0, 0, 0, (GLsizei)grp->mesh.indices.len, grp->material, grp->owner};
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
        prts_array_push(&g->meshes, &m);
    }
    for (size_t i = 0; i < s->lines.len; i++) {
        const prts_line_set *ls = prts_array_at(&s->lines, i);
        if (ls->points.len == 0) continue;
        gpu_lines l = {0, 0, (GLsizei)ls->points.len, ls->color, ls->owner};
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
    for (size_t i = 0; i < s->materials.len; i++) prts_gfx_texture(g, ((prts_material *)prts_array_at(&s->materials, i))->texture);
    g->uploaded_scene = s;
    g->uploaded_revision = s->revision;
}

static void ensure_target(prts_gfx *g, int w, int h)
{
    if (g->fbo && g->fbo_w == w && g->fbo_h == h) return;
    if (g->fbo) {
        g->gl.glDeleteFramebuffers(1, &g->fbo);
        g->gl.glDeleteTextures(1, &g->color_tex);
        g->gl.glDeleteRenderbuffers(1, &g->depth_rb);
    }
    g->gl.glGenTextures(1, &g->color_tex);
    g->gl.glBindTexture(GL_TEXTURE_2D, g->color_tex);
    g->gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    g->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    g->gl.glGenRenderbuffers(1, &g->depth_rb);
    g->gl.glBindRenderbuffer(GL_RENDERBUFFER, g->depth_rb);
    g->gl.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    g->gl.glGenFramebuffers(1, &g->fbo);
    g->gl.glBindFramebuffer(GL_FRAMEBUFFER, g->fbo);
    g->gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g->color_tex, 0);
    g->gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g->depth_rb);
    if (g->gl.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        prts_log_error("render target %dx%d is incomplete", w, h);
    }
    g->fbo_w = w;
    g->fbo_h = h;
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
    const prts_camera *cam;
    const prts_render_settings *rs;
    double alpha;
    int iw, ih;
} frame_ctx;

static prts_mat4 owner_model(const frame_ctx *f, prts_entity owner)
{
    if (owner == PRTS_ENTITY_NONE) return prts_mat4_translate(prts_dv3_relative(prts_dv3(0, 0, 0), f->cam->eye));
    prts_dvec3 p = prts_world_pos_interp(f->world, owner, f->alpha);
    float yaw = prts_world_yaw_interp(f->world, owner, f->alpha);
    return prts_mat4_mul(prts_mat4_translate(prts_dv3_relative(p, f->cam->eye)), prts_mat4_rotate_y(yaw));
}

static void draw_sky(prts_gfx *g, const frame_ctx *f)
{
    GLuint p = g->prog_sky;
    g->gl.glUseProgram(p);
    float aspect = (float)f->iw / (float)f->ih;
    prts_vec3 fwd = prts_view_forward(f->cam->yaw, f->cam->pitch);
    prts_vec3 right = prts_v3_normalize(prts_v3_cross(fwd, prts_v3(0, 1, 0)));
    prts_vec3 up = prts_v3_cross(right, fwd);
    float t = tanf(f->rs->fov_y * 0.5f);
    set_vec3(g, p, "u_forward", fwd);
    set_vec3(g, p, "u_right", prts_v3_scale(right, t * aspect));
    set_vec3(g, p, "u_up", prts_v3_scale(up, t));
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
    prts_vec3 eye = prts_v3((float)f->cam->eye.x, (float)f->cam->eye.y, (float)f->cam->eye.z);
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
        prts_vec3 rel = prts_dv3_relative(prts_dv3_from_v3(lights[i].pos), f->cam->eye);
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
    for (size_t i = 0; i < g->meshes.len; i++) {
        gpu_mesh *m = prts_array_at(&g->meshes, i);
        if (m->owner != PRTS_ENTITY_NONE && !prts_world_alive(f->world, m->owner)) continue;
        const prts_material *mat = prts_array_at(&f->scene->materials, (size_t)m->material);
        g->gl.glBindTexture(GL_TEXTURE_2D, prts_gfx_texture(g, mat ? mat->texture : ""));
        g->gl.glUniform1f(emissive_loc, mat && mat->emissive ? 1.0f : 0.0f);
        prts_mat4 model = owner_model(f, m->owner);
        g->gl.glUniformMatrix4fv(model_loc, 1, 0, model.m);
        g->gl.glBindVertexArray(m->vao);
        g->gl.glDrawElements(GL_TRIANGLES, m->count, GL_UNSIGNED_INT, NULL);
    }
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

void prts_gfx_render(prts_gfx *g, const prts_scene *scene, const prts_world *world, const prts_camera *cam,
                     const prts_render_settings *rs, int out_w, int out_h, double alpha, double time)
{
    if (g->null) return;
    if (g->uploaded_scene != scene || g->uploaded_revision != scene->revision) upload_scene(g, scene);
    frame_ctx f = {scene, world, cam, rs, alpha, 0, 0};
    prts_gfx_internal_size(rs->preset, out_w, out_h, &f.iw, &f.ih);
    ensure_target(g, f.iw, f.ih);

    g->gl.glBindFramebuffer(GL_FRAMEBUFFER, g->fbo);
    g->gl.glViewport(0, 0, f.iw, f.ih);
    g->gl.glDisable(GL_DEPTH_TEST);
    g->gl.glDisable(GL_CULL_FACE);
    draw_sky(g, &f);

    g->gl.glEnable(GL_DEPTH_TEST);
    g->gl.glDepthFunc(GL_LEQUAL);
    g->gl.glEnable(GL_CULL_FACE);
    g->gl.glCullFace(GL_BACK);
    float aspect = (float)f.iw / (float)f.ih;
    prts_mat4 view = prts_mat4_view_yaw_pitch(cam->yaw, cam->pitch);
    /* Two depth layers: far geometry first, then a fresh depth range for nearby geometry. */
    const float layers[2][2] = {{FAR_LAYER_NEAR, FAR_LAYER_FAR}, {NEAR_LAYER_NEAR, NEAR_LAYER_FAR}};
    prts_mat4 view_proj = view;
    for (int l = 0; l < 2; l++) {
        g->gl.glClear(GL_DEPTH_BUFFER_BIT);
        view_proj = prts_mat4_mul(prts_mat4_perspective(rs->fov_y, aspect, layers[l][0], layers[l][1]), view);
        draw_meshes(g, &f, view_proj);
    }
    draw_lines(g, &f, view_proj, time);

    /* upscale to the window with whole-number scaling where possible */
    g->gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    g->gl.glDisable(GL_DEPTH_TEST);
    g->gl.glDisable(GL_CULL_FACE);
    g->gl.glViewport(0, 0, out_w, out_h);
    g->gl.glClearColor(0, 0, 0, 1);
    g->gl.glClear(GL_COLOR_BUFFER_BIT);
    /* whole-number scaling keeps pixels even; fall back to a fractional fit when a whole
     * number would leave more than 10% of the window empty */
    float fit = fminf((float)out_w / (float)f.iw, (float)out_h / (float)f.ih);
    float scale = floorf(fit);
    if (scale < 1.0f || scale / fit < 0.9f) scale = fit;
    int vw = (int)((float)f.iw * scale), vh = (int)((float)f.ih * scale);
    g->gl.glViewport((out_w - vw) / 2, (out_h - vh) / 2, vw, vh);
    GLuint p = g->prog_post;
    g->gl.glUseProgram(p);
    g->gl.glActiveTexture(GL_TEXTURE0);
    g->gl.glBindTexture(GL_TEXTURE_2D, g->color_tex);
    g->gl.glUniform1i(prts_gfx_uniform(g, p, "u_scene"), 0);
    g->gl.glUniform2f(prts_gfx_uniform(g, p, "u_size"), (float)f.iw, (float)f.ih);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_dither"), rs->dither ? 1.0f : 0.0f);
    g->gl.glUniform1f(prts_gfx_uniform(g, p, "u_crosshair"), rs->crosshair ? 1.0f : 0.0f);
    g->gl.glBindVertexArray(g->empty_vao);
    g->gl.glDrawArrays(GL_TRIANGLES, 0, 3);
    g->gl.glBindVertexArray(0);
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
