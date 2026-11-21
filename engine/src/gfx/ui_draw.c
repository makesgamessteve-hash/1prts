/* 1stPRTS engine: draws UI batches over the window or into world-surface textures. MIT License. */
#include "prts/prts_gfx_ui.h"

#include "gfx/gfx_internal.h"
#include "prts/prts_log.h"

#define UI_STREAM_DRAW 0x88E0 /* GL_STREAM_DRAW */

typedef struct ui_target {
    GLuint fbo, tex;
    int w, h;
} ui_target;

struct prts_gfx_ui {
    prts_gfx *g;
    GLuint prog, vao, vbo;
    ui_target targets[PRTS_GFX_UI_SLOTS];
    size_t draws;
};

prts_gfx_ui *prts_gfx_ui_create(prts_gfx *g)
{
    prts_gfx_ui *u = prts_calloc(1, sizeof(*u));
    u->g = g;
    if (g->null) return u;
    u->prog = prts_gfx_load_program(g, "ui");
    g->gl.glGenVertexArrays(1, &u->vao);
    g->gl.glBindVertexArray(u->vao);
    g->gl.glGenBuffers(1, &u->vbo);
    g->gl.glBindBuffer(GL_ARRAY_BUFFER, u->vbo);
    GLsizei stride = (GLsizei)sizeof(prts_ui_vertex);
    g->gl.glEnableVertexAttribArray(0);
    g->gl.glVertexAttribPointer(0, 2, GL_FLOAT, 0, stride, (const void *)0);
    g->gl.glEnableVertexAttribArray(2);
    g->gl.glVertexAttribPointer(2, 2, GL_FLOAT, 0, stride, (const void *)(2 * sizeof(float)));
    g->gl.glEnableVertexAttribArray(3);
    g->gl.glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, 1, stride, (const void *)(4 * sizeof(float)));
    g->gl.glBindVertexArray(0);
    return u;
}

void prts_gfx_ui_destroy(prts_gfx_ui *u)
{
    if (!u) return;
    if (!u->g->null) {
        prts_gl *gl = &u->g->gl;
        for (int i = 0; i < PRTS_GFX_UI_SLOTS; i++) {
            if (!u->targets[i].fbo) continue;
            gl->glDeleteFramebuffers(1, &u->targets[i].fbo);
            gl->glDeleteTextures(1, &u->targets[i].tex);
        }
        gl->glDeleteBuffers(1, &u->vbo);
        gl->glDeleteVertexArrays(1, &u->vao);
        if (u->prog != u->g->prog_fallback) gl->glDeleteProgram(u->prog);
    }
    prts_free(u);
}

size_t prts_gfx_ui_draw_count(const prts_gfx_ui *u) { return u->draws; }

/* Draws the batch into the bound framebuffer, which is size_w x size_h UI units big. */
static void draw_batch(prts_gfx_ui *u, const prts_ui_batch *b, float size_w, float size_h, float flip)
{
    prts_gl *gl = &u->g->gl;
    gl->glDisable(GL_DEPTH_TEST);
    gl->glDisable(GL_CULL_FACE);
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->glUseProgram(u->prog);
    gl->glUniform2f(prts_gfx_uniform(u->g, u->prog, "u_size"), size_w, size_h);
    gl->glUniform1f(prts_gfx_uniform(u->g, u->prog, "u_flip"), flip);
    gl->glUniform1i(prts_gfx_uniform(u->g, u->prog, "u_tex"), 0);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, prts_gfx_texture(u->g, b->texture));
    gl->glBindVertexArray(u->vao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, u->vbo);
    gl->glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(b->verts.len * sizeof(prts_ui_vertex)), b->verts.data,
                     UI_STREAM_DRAW);
    gl->glDrawArrays(GL_TRIANGLES, 0, (GLsizei)b->verts.len);
    gl->glBindVertexArray(0);
    gl->glDisable(GL_BLEND);
    u->draws++;
}

void prts_gfx_ui_draw_screen(prts_gfx_ui *u, const prts_ui_batch *b, int out_w, int out_h)
{
    if (u->g->null || b->verts.len == 0) return;
    prts_gl *gl = &u->g->gl;
    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl->glViewport(0, 0, out_w, out_h);
    float scale = (float)PRTS_MAX(1, b->scale);
    draw_batch(u, b, (float)out_w / scale, (float)out_h / scale, 1.0f);
}

static void ensure_target(prts_gfx_ui *u, ui_target *t, int w, int h)
{
    if (t->fbo && t->w == w && t->h == h) return;
    prts_gl *gl = &u->g->gl;
    if (!t->fbo) {
        gl->glGenFramebuffers(1, &t->fbo);
        gl->glGenTextures(1, &t->tex);
    }
    gl->glBindTexture(GL_TEXTURE_2D, t->tex);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->tex, 0);
    if (gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        prts_log_error("UI surface target %dx%d is incomplete", w, h);
    }
    t->w = w;
    t->h = h;
}

void prts_gfx_ui_draw_surface(prts_gfx_ui *u, int slot, const prts_ui_batch *b)
{
    if (u->g->null || slot < 1 || slot >= PRTS_GFX_UI_SLOTS || b->width < 1 || b->height < 1) return;
    prts_gl *gl = &u->g->gl;
    ui_target *t = &u->targets[slot];
    ensure_target(u, t, b->width, b->height);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    gl->glViewport(0, 0, t->w, t->h);
    gl->glClearColor(0, 0, 0, 0);
    gl->glClear(GL_COLOR_BUFFER_BIT);
    if (b->verts.len > 0) draw_batch(u, b, (float)t->w, (float)t->h, -1.0f);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t prts_gfx_ui_surface_texture(const prts_gfx_ui *u, int slot)
{
    if (slot < 1 || slot >= PRTS_GFX_UI_SLOTS) return 0;
    return u->targets[slot].tex;
}
