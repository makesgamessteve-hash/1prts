/* 1stPRTS engine: 2D UI batches (textured, colored quads) and their OpenGL renderer. MIT License. */
#ifndef PRTS_GFX_UI_H
#define PRTS_GFX_UI_H

#include "prts_gfx.h"

/* One corner of a UI quad. Positions are in UI units with the origin at the top-left;
 * texture coordinates address the font atlas, whose white block serves plain rectangles. */
typedef struct prts_ui_vertex {
    float x, y;
    float u, v;
    unsigned char rgba[4];
} prts_ui_vertex;

/* Everything one UI surface draws in a frame: 6 vertices per quad, already clipped on the
 * CPU, so a batch is a single draw call with one texture. */
typedef struct prts_ui_batch {
    prts_array verts;  /* prts_ui_vertex */
    int width, height; /* size of the surface in UI units */
    int scale;         /* output pixels per UI unit (screen overlay) */
    char texture[128]; /* atlas path in the VFS */
} prts_ui_batch;

#define PRTS_GFX_UI_SLOTS 8 /* slot 0 is the screen overlay; world surfaces use 1.. */

typedef struct prts_gfx_ui prts_gfx_ui;

/* The UI renderer of a gfx back end. With the null back end nothing is drawn. */
prts_gfx_ui *prts_gfx_ui_create(prts_gfx *g);
void prts_gfx_ui_destroy(prts_gfx_ui *u);
/* Draws a batch over the whole window, after prts_gfx_render (the screen overlay). */
void prts_gfx_ui_draw_screen(prts_gfx_ui *u, const prts_ui_batch *b, int out_w, int out_h);
/* Draws a batch into the offscreen texture of world surface `slot` (1..), which is created
 * or resized to the batch size. Texel (u, v) shows the UI point (u * width, v * height):
 * v = 0 is the top edge of the UI. */
void prts_gfx_ui_draw_surface(prts_gfx_ui *u, int slot, const prts_ui_batch *b);
/* OpenGL texture name of a world surface (0 if it was never drawn), for the renderer to map
 * onto objects such as the war table or a scrying pool. */
uint32_t prts_gfx_ui_surface_texture(const prts_gfx_ui *u, int slot);
/* Number of batches drawn since creation (for tests). */
size_t prts_gfx_ui_draw_count(const prts_gfx_ui *u);

#endif
