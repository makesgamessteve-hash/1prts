/* 1stPRTS engine: immediate-mode UI (microui behind this interface), bitmap fonts, the screen
 * overlay and world surfaces, and ray hit tests onto surfaces. MIT License. */
#ifndef PRTS_UI_H
#define PRTS_UI_H

#include "prts_gfx_ui.h"
#include "prts_math.h"

#define PRTS_UI_SCREEN 0 /* surface 0 is the screen overlay */
#define PRTS_UI_MAX_SURFACES PRTS_GFX_UI_SLOTS
#define PRTS_UI_NAME_MAX 48
#define PRTS_UI_FONT_GLYPHS 96

typedef enum prts_ui_icon {
    PRTS_UI_ICON_WHITE = 0,
    PRTS_UI_ICON_CHECK,
    PRTS_UI_ICON_CLOSE,
    PRTS_UI_ICON_COLLAPSED,
    PRTS_UI_ICON_EXPANDED,
    PRTS_UI_ICON_DISC,
    PRTS_UI_ICON_RING,
    PRTS_UI_ICON_COUNT
} prts_ui_icon;

const char *prts_ui_icon_name(prts_ui_icon icon);
bool prts_ui_icon_parse(const char *name, prts_ui_icon *out);

/* A bitmap font: an atlas of fixed cells for the characters first..first+count-1, with a
 * per-character advance, plus icon rectangles in the same atlas. */
typedef struct prts_ui_font {
    char texture[128];
    int atlas_w, atlas_h;
    int height;      /* glyph height in pixels (with descenders) */
    int line_height; /* distance between lines */
    int cell_w, cell_h, columns, first, count;
    unsigned char advance[PRTS_UI_FONT_GLYPHS];
    int icons[PRTS_UI_ICON_COUNT][4]; /* x, y, w, h */
} prts_ui_font;

/* Fixed-width fallback metrics used until (or if) a font is loaded from data. */
void prts_ui_font_default(prts_ui_font *f);
/* Width of len bytes of UTF-8 text (len < 0: up to the terminator). */
int prts_ui_text_width(const prts_ui_font *f, const char *text, int len);

/* --- surface geometry and hit tests (pure math) ------------------------------------------ */

/* A world surface: a parallelogram from `origin` (the UI's top-left corner) along `right`
 * (to the top-right corner) and `down` (to the bottom-left corner). */
typedef struct prts_ui_quad {
    prts_dvec3 origin;
    prts_vec3 right, down;
} prts_ui_quad;

/* Intersects a ray with a surface (either side). On a hit within max_dist, returns true with
 * the surface coordinates u, v in 0..1 (v = 0 at the top edge) and the distance. */
bool prts_ui_ray_quad(const prts_ui_quad *q, prts_dvec3 origin, prts_vec3 dir, float max_dist, float *u, float *v,
                      float *dist);
/* Direction of the ray through screen point (sx, sy) (fractions of the view, top-left origin)
 * for a camera looking along yaw/pitch with vertical field of view fov_y. */
prts_vec3 prts_ui_screen_ray(float yaw, float pitch, float fov_y, float aspect, float sx, float sy);
/* Integer UI scale for an output height: the screen overlay is laid out in about 360 lines. */
int prts_ui_scale_for(int out_h);

/* --- contexts and frames --------------------------------------------------------------- */

typedef struct prts_ui prts_ui;

/* Keys the UI reacts to (text fields). */
enum {
    PRTS_UI_KEY_SHIFT = 1 << 0,
    PRTS_UI_KEY_CTRL = 1 << 1,
    PRTS_UI_KEY_ALT = 1 << 2,
    PRTS_UI_KEY_BACKSPACE = 1 << 3,
    PRTS_UI_KEY_RETURN = 1 << 4
};

/* Input for one surface and frame. Button and key edges are derived from the previous frame. */
typedef struct prts_ui_input {
    bool has_pointer; /* false: the pointer is not over this surface */
    float x, y;       /* pointer in UI units */
    bool down;        /* primary button (mouse, touch, use, XR trigger) */
    float scroll;     /* wheel steps this frame; positive (wheel up) scrolls towards the top */
    unsigned keys;    /* PRTS_UI_KEY_* held */
    const char *text; /* UTF-8 text typed this frame, or NULL */
} prts_ui_input;

prts_ui *prts_ui_create(void);
void prts_ui_destroy(prts_ui *ui);
void prts_ui_set_font(prts_ui *ui, const prts_ui_font *f);
const prts_ui_font *prts_ui_get_font(const prts_ui *ui);

/* Sets the window size; the screen overlay gets an integer scale and the matching UI size. */
void prts_ui_set_screen(prts_ui *ui, int out_w, int out_h);
/* Adds (or resizes) a world surface; returns its index (>= 1) or -1 if all slots are used
 * or the size is out of range (8..1024 units). */
int prts_ui_surface_add(prts_ui *ui, const char *name, int width, int height);
int prts_ui_surface_find(const prts_ui *ui, const char *name);
int prts_ui_surface_count(const prts_ui *ui);
const char *prts_ui_surface_name(const prts_ui *ui, int s);
void prts_ui_surface_size(const prts_ui *ui, int s, int *w, int *h);
void prts_ui_surface_set_quad(prts_ui *ui, int s, const prts_ui_quad *q);
bool prts_ui_surface_quad(const prts_ui *ui, int s, prts_ui_quad *out);

void prts_ui_feed(prts_ui *ui, int s, const prts_ui_input *in);
/* The pointer of surface s as last fed, before microui sees it: for scripts that draw their
 * own clickable pictures (the war table). pressed: the button went down in that feed. */
typedef struct prts_ui_pointer_state {
    bool present;
    float x, y;
    bool down, pressed;
} prts_ui_pointer_state;
bool prts_ui_pointer(const prts_ui *ui, int s, prts_ui_pointer_state *out);
/* Starts building surface s. Widgets and drawing calls go to it until prts_ui_end. */
void prts_ui_begin(prts_ui *ui, int s);
/* Ends the frame and converts it into the surface's batch. Unbalanced windows and panels
 * left open by a failed script are closed first. */
void prts_ui_end(prts_ui *ui);
int prts_ui_current(const prts_ui *ui); /* surface being built, or -1 */
const prts_ui_batch *prts_ui_batch_of(const prts_ui *ui, int s);
/* True if the pointer was over a window or panel of surface s in the last frame (so a click
 * belongs to the UI, not to the game). */
bool prts_ui_hovered(const prts_ui *ui, int s);

/* --- widgets (between begin and end) ---------------------------------------------------
 * Every call first checks that it is allowed now; if not it returns false and
 * prts_ui_error() says why (scripts turn that into an error instead of a crash). */

const char *prts_ui_error(const prts_ui *ui);

enum {
    PRTS_UI_WIN_TITLE = 1 << 0,  /* title bar */
    PRTS_UI_WIN_FRAME = 1 << 1,  /* background and border */
    PRTS_UI_WIN_SCROLL = 1 << 2, /* scroll bars when the content is larger */
    PRTS_UI_WIN_INPUT = 1 << 3,  /* takes pointer input (HUD windows do not) */
    PRTS_UI_WIN_DEFAULT = PRTS_UI_WIN_TITLE | PRTS_UI_WIN_FRAME | PRTS_UI_WIN_SCROLL | PRTS_UI_WIN_INPUT
};

/* Windows keep the given rectangle every frame (menus are laid out by scripts). */
bool prts_ui_window_begin(prts_ui *ui, const char *title, int x, int y, int w, int h, unsigned flags);
void prts_ui_window_end(prts_ui *ui);
/* Scrollable sub-area filling the next layout cell. */
bool prts_ui_panel_begin(prts_ui *ui, const char *name, bool scroll_to_end);
void prts_ui_panel_end(prts_ui *ui);
/* Next row: n cells with the given widths (0: default, negative: up to that far from the
 * right edge) and height (0: default). n is at most 16. */
bool prts_ui_row(prts_ui *ui, int n, const int *widths, int height);
bool prts_ui_label(prts_ui *ui, const char *text, bool centered);
bool prts_ui_text(prts_ui *ui, const char *text); /* wrapped paragraph */
/* Widgets take an id (NULL: the label) so equal labels in one window stay distinct. */
bool prts_ui_button(prts_ui *ui, const char *label, const char *id, bool *clicked);
bool prts_ui_checkbox(prts_ui *ui, const char *label, const char *id, bool *value, bool *changed);
bool prts_ui_slider(prts_ui *ui, const char *id, float *value, float lo, float hi, float step, bool *changed);
/* Edits buf (size bytes). focus = true keeps the keyboard focus on it. */
bool prts_ui_textbox(prts_ui *ui, const char *id, char *buf, int size, bool focus, bool *changed, bool *submitted);

/* Direct drawing on top of the widgets (HUD, touch overlay, labels). Colors are RGBA. */
enum { PRTS_UI_ALIGN_LEFT = 0, PRTS_UI_ALIGN_CENTER, PRTS_UI_ALIGN_RIGHT };
bool prts_ui_draw_rect(prts_ui *ui, float x, float y, float w, float h, const unsigned char rgba[4]);
bool prts_ui_draw_icon(prts_ui *ui, prts_ui_icon icon, float x, float y, float w, float h,
                       const unsigned char rgba[4]);
/* Draws text with its top-left (or top-center / top-right) at x, y, magnified by scale. */
bool prts_ui_draw_text(prts_ui *ui, const char *text, float x, float y, int align, int scale,
                       const unsigned char rgba[4]);

/* --- what was drawn in the last frame (tests and accessibility) --------------------------- */

size_t prts_ui_text_count(const prts_ui *ui, int s);
const char *prts_ui_text_at(const prts_ui *ui, int s, size_t i);
/* Finds an interactive widget by label or id in the last frame; gives its center. */
bool prts_ui_find_widget(const prts_ui *ui, int s, const char *label, float *x, float *y);

#endif
