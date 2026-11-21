/* 1stPRTS engine: immediate-mode UI over microui, bitmap font text and UI batches. MIT License. */
#include "prts/prts_ui.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "microui.h"

/* Limits that keep scripts from tripping microui's internal assertions (which abort). */
#define MAX_DEPTH 12          /* open windows + panels */
#define MAX_WINDOWS 24        /* root windows per frame (microui: 32) */
#define MAX_CONTAINERS 40     /* windows + panels per frame (microui pool: 48) */
#define MAX_TEXT 1024         /* bytes per label or button */
#define MAX_PARAGRAPH 8192    /* bytes per wrapped text */
#define COMMAND_RESERVE (64 * 1024)

typedef struct widget_rec {
    char *label;
    char *id;
    mu_Rect rect;
} widget_rec;

typedef struct surface {
    bool used;
    char name[PRTS_UI_NAME_MAX];
    int w, h, scale;
    bool has_quad;
    prts_ui_quad quad;
    mu_Context *mu;
    prts_ui_batch batch;
    prts_array top;     /* prts_ui_vertex: direct drawing, on top of the widgets */
    prts_array texts;   /* char*: every text drawn in the last frame */
    prts_array widgets; /* widget_rec */
    bool raw_down;   /* the button as the device reports it */
    bool pending;    /* a press waiting for the pointer to settle */
    bool delivered;  /* microui sees the button down */
    int settle;      /* frames the pointer has stayed near the same spot */
    float last_x, last_y;
    unsigned prev_keys;
    bool hovered;
} surface;

struct prts_ui {
    prts_ui_font font;
    surface surfaces[PRTS_UI_MAX_SURFACES];
    int current;
    int windows, containers;
    int stack[MAX_DEPTH]; /* 1: window, 2: panel */
    int depth;
    const char *error;
};

static const char *ICON_NAMES[PRTS_UI_ICON_COUNT] = {"white", "check", "close", "collapsed", "expanded", "disc", "ring"};

const char *prts_ui_icon_name(prts_ui_icon icon) { return ICON_NAMES[PRTS_CLAMP((int)icon, 0, PRTS_UI_ICON_COUNT - 1)]; }

bool prts_ui_icon_parse(const char *name, prts_ui_icon *out)
{
    for (int i = 0; i < PRTS_UI_ICON_COUNT; i++) {
        if (strcmp(name, ICON_NAMES[i]) == 0) {
            *out = (prts_ui_icon)i;
            return true;
        }
    }
    return false;
}

/* --- font ----------------------------------------------------------------------------- */

void prts_ui_font_default(prts_ui_font *f)
{
    memset(f, 0, sizeof(*f));
    snprintf(f->texture, sizeof(f->texture), "%s", "fonts/ui.png");
    f->atlas_w = f->atlas_h = 128;
    f->height = 9;
    f->line_height = 11;
    f->cell_w = 8;
    f->cell_h = 12;
    f->columns = 16;
    f->first = 32;
    f->count = 95;
    for (int i = 0; i < f->count; i++) f->advance[i] = 6;
    for (int i = 0; i < PRTS_UI_ICON_COUNT; i++) {
        f->icons[i][0] = 40;
        f->icons[i][1] = 80;
        f->icons[i][2] = f->icons[i][3] = 4;
    }
}

/* Glyph index for byte c (UTF-8 continuation bytes give -1, unknown characters '?'). */
static int glyph_of(const prts_ui_font *f, unsigned char c)
{
    if ((c & 0xc0) == 0x80) return -1;
    int g = (int)c - f->first;
    if (g < 0 || g >= f->count) g = '?' - f->first;
    return g >= 0 && g < f->count ? g : -1;
}

int prts_ui_text_width(const prts_ui_font *f, const char *text, int len)
{
    int w = 0;
    for (int i = 0; len < 0 ? text[i] != '\0' : i < len; i++) {
        int g = glyph_of(f, (unsigned char)text[i]);
        if (g >= 0) w += f->advance[g];
    }
    return w;
}

static int mu_text_width(mu_Font font, const char *str, int len) { return prts_ui_text_width(font, str, len); }
static int mu_text_height(mu_Font font) { return ((const prts_ui_font *)font)->height; }

/* --- lifecycle and surfaces ---------------------------------------------------------- */

static void set_style(mu_Context *m, prts_ui_font *font)
{
    static const unsigned char colors[MU_COLOR_MAX][4] = {
        {236, 224, 196, 255}, /* text */
        {18, 12, 8, 255},     /* border */
        {44, 32, 24, 238},    /* window */
        {92, 58, 30, 255},    /* title */
        {252, 226, 160, 255}, /* title text */
        {30, 22, 16, 160},    /* panel */
        {84, 60, 38, 255},    /* button */
        {116, 82, 48, 255},   /* button hover */
        {150, 106, 56, 255},  /* button focus */
        {28, 20, 14, 255},    /* base */
        {40, 30, 22, 255},    /* base hover */
        {54, 40, 28, 255},    /* base focus */
        {36, 26, 20, 255},    /* scroll base */
        {120, 88, 52, 255},   /* scroll thumb */
    };
    m->text_width = mu_text_width;
    m->text_height = mu_text_height;
    m->style->font = font;
    m->style->size = mu_vec2(68, 10);
    m->style->padding = 4;
    m->style->spacing = 3;
    m->style->indent = 12;
    m->style->title_height = 15;
    m->style->scrollbar_size = 8;
    m->style->thumb_size = 8;
    for (int i = 0; i < MU_COLOR_MAX; i++) {
        m->style->colors[i] = mu_color(colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
    }
}

static void surface_init(prts_ui *ui, surface *s, const char *name, int w, int h)
{
    s->used = true;
    snprintf(s->name, sizeof(s->name), "%s", name);
    s->w = w;
    s->h = h;
    s->scale = 1;
    s->mu = prts_malloc(sizeof(mu_Context));
    mu_init(s->mu);
    set_style(s->mu, &ui->font);
    prts_array_init(&s->batch.verts, sizeof(prts_ui_vertex));
    prts_array_init(&s->top, sizeof(prts_ui_vertex));
    prts_array_init(&s->texts, sizeof(char *));
    prts_array_init(&s->widgets, sizeof(widget_rec));
}

static void clear_records(surface *s)
{
    for (size_t i = 0; i < s->texts.len; i++) prts_free(*(char **)prts_array_at(&s->texts, i));
    for (size_t i = 0; i < s->widgets.len; i++) {
        widget_rec *w = prts_array_at(&s->widgets, i);
        prts_free(w->label);
        prts_free(w->id);
    }
    prts_array_clear(&s->texts);
    prts_array_clear(&s->widgets);
}

prts_ui *prts_ui_create(void)
{
    prts_ui *ui = prts_calloc(1, sizeof(*ui));
    prts_ui_font_default(&ui->font);
    ui->current = -1;
    surface_init(ui, &ui->surfaces[PRTS_UI_SCREEN], "screen", 640, 360);
    prts_ui_set_screen(ui, 1280, 720);
    return ui;
}

void prts_ui_destroy(prts_ui *ui)
{
    if (!ui) return;
    for (int i = 0; i < PRTS_UI_MAX_SURFACES; i++) {
        surface *s = &ui->surfaces[i];
        if (!s->used) continue;
        clear_records(s);
        prts_array_free(&s->texts);
        prts_array_free(&s->widgets);
        prts_array_free(&s->top);
        prts_array_free(&s->batch.verts);
        prts_free(s->mu);
    }
    prts_free(ui);
}

void prts_ui_set_font(prts_ui *ui, const prts_ui_font *f) { ui->font = *f; }
const prts_ui_font *prts_ui_get_font(const prts_ui *ui) { return &ui->font; }

int prts_ui_scale_for(int out_h) { return PRTS_MAX(1, (int)lroundf((float)out_h / 360.0f)); }

void prts_ui_set_screen(prts_ui *ui, int out_w, int out_h)
{
    surface *s = &ui->surfaces[PRTS_UI_SCREEN];
    s->scale = prts_ui_scale_for(out_h);
    s->w = PRTS_MAX(1, out_w / s->scale);
    s->h = PRTS_MAX(1, out_h / s->scale);
}

static surface *get_surface(const prts_ui *ui, int s)
{
    if (s < 0 || s >= PRTS_UI_MAX_SURFACES || !ui->surfaces[s].used) return NULL;
    return (surface *)&ui->surfaces[s];
}

int prts_ui_surface_find(const prts_ui *ui, const char *name)
{
    for (int i = 0; i < PRTS_UI_MAX_SURFACES; i++) {
        if (ui->surfaces[i].used && strcmp(ui->surfaces[i].name, name) == 0) return i;
    }
    return -1;
}

int prts_ui_surface_add(prts_ui *ui, const char *name, int width, int height)
{
    if (width < 8 || height < 8 || width > 1024 || height > 1024) return -1;
    int s = prts_ui_surface_find(ui, name);
    if (s == PRTS_UI_SCREEN) return -1;
    if (s < 0) {
        for (int i = 1; i < PRTS_UI_MAX_SURFACES && s < 0; i++) {
            if (!ui->surfaces[i].used) s = i;
        }
        if (s < 0) return -1;
        surface_init(ui, &ui->surfaces[s], name, width, height);
    }
    ui->surfaces[s].w = width;
    ui->surfaces[s].h = height;
    return s;
}

int prts_ui_surface_count(const prts_ui *ui)
{
    int n = 0;
    for (int i = 0; i < PRTS_UI_MAX_SURFACES; i++) n += ui->surfaces[i].used ? 1 : 0;
    return n;
}

const char *prts_ui_surface_name(const prts_ui *ui, int s)
{
    surface *sf = get_surface(ui, s);
    return sf ? sf->name : NULL;
}

void prts_ui_surface_size(const prts_ui *ui, int s, int *w, int *h)
{
    surface *sf = get_surface(ui, s);
    *w = sf ? sf->w : 0;
    *h = sf ? sf->h : 0;
}

void prts_ui_surface_set_quad(prts_ui *ui, int s, const prts_ui_quad *q)
{
    surface *sf = get_surface(ui, s);
    if (!sf || s == PRTS_UI_SCREEN) return;
    sf->quad = *q;
    sf->has_quad = true;
}

bool prts_ui_surface_quad(const prts_ui *ui, int s, prts_ui_quad *out)
{
    surface *sf = get_surface(ui, s);
    if (!sf || !sf->has_quad) return false;
    *out = sf->quad;
    return true;
}

/* --- input and frames ------------------------------------------------------------------ */

void prts_ui_feed(prts_ui *ui, int s, const prts_ui_input *in)
{
    surface *sf = get_surface(ui, s);
    if (!sf) return;
    mu_Context *m = sf->mu;
    /* microui learns which window is under the pointer a frame late, so a widget can only
     * take a press after the pointer has stayed two frames on it. A press that comes sooner
     * (a pointer that appears and presses at once, like a quick tap that may even be over
     * before then) is held back until the pointer has settled, at its last position. */
    bool present = in->has_pointer;
    float px = in->x, py = in->y;
    if (!present && (sf->pending || sf->delivered)) {
        present = true;
        px = sf->last_x;
        py = sf->last_y;
    }
    bool jumped = fabsf(px - sf->last_x) + fabsf(py - sf->last_y) > 16.0f;
    sf->settle = present && !jumped ? PRTS_MIN(sf->settle + 1, 2) : 0;
    sf->last_x = px;
    sf->last_y = py;
    bool raw = in->has_pointer && in->down;
    if (raw && !sf->raw_down) sf->pending = true;
    sf->raw_down = raw;
    int x = present ? (int)floorf(px) : -100000, y = present ? (int)floorf(py) : -100000;
    mu_input_mousemove(m, x, y);
    if (sf->pending && sf->settle >= 2) {
        mu_input_mousedown(m, x, y, MU_MOUSE_LEFT);
        sf->pending = false;
        sf->delivered = true;
    } else if (sf->delivered && !raw) {
        mu_input_mouseup(m, x, y, MU_MOUSE_LEFT);
        sf->delivered = false;
    }
    if (in->scroll != 0.0f) mu_input_scroll(m, 0, (int)(-in->scroll * 24.0f));
    static const int keys[][2] = {{PRTS_UI_KEY_SHIFT, MU_KEY_SHIFT},         {PRTS_UI_KEY_CTRL, MU_KEY_CTRL},
                                  {PRTS_UI_KEY_ALT, MU_KEY_ALT},             {PRTS_UI_KEY_BACKSPACE, MU_KEY_BACKSPACE},
                                  {PRTS_UI_KEY_RETURN, MU_KEY_RETURN}};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(keys); i++) {
        bool now = (in->keys & (unsigned)keys[i][0]) != 0, before = (sf->prev_keys & (unsigned)keys[i][0]) != 0;
        if (now && !before) mu_input_keydown(m, keys[i][1]);
        if (!now && before) mu_input_keyup(m, keys[i][1]);
    }
    sf->prev_keys = in->keys;
    if (in->text && *in->text) {
        /* microui keeps 31 bytes of text per frame; cut at a character boundary */
        char buf[32];
        size_t n = strlen(m->input_text), room = sizeof(buf) - 1 - n, len = strlen(in->text);
        if (len > room) {
            len = room;
            while (len > 0 && ((unsigned char)in->text[len] & 0xc0) == 0x80) len--;
        }
        memcpy(buf, in->text, len);
        buf[len] = '\0';
        mu_input_text(m, buf);
    }
}

void prts_ui_begin(prts_ui *ui, int s)
{
    surface *sf = get_surface(ui, s);
    if (!sf || ui->current >= 0) return;
    ui->current = s;
    ui->windows = ui->containers = ui->depth = 0;
    ui->error = NULL;
    clear_records(sf);
    prts_array_clear(&sf->top);
    mu_begin(sf->mu);
}

int prts_ui_current(const prts_ui *ui) { return ui->current; }

static void push_quad(prts_array *out, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
                      const unsigned char rgba[4], const mu_Rect *clip)
{
    if (clip) {
        float cx0 = (float)clip->x, cy0 = (float)clip->y;
        float cx1 = cx0 + (float)clip->w, cy1 = cy0 + (float)clip->h;
        float nx0 = fmaxf(x0, cx0), ny0 = fmaxf(y0, cy0), nx1 = fminf(x1, cx1), ny1 = fminf(y1, cy1);
        if (nx0 >= nx1 || ny0 >= ny1) return;
        float du = (u1 - u0) / (x1 - x0), dv = (v1 - v0) / (y1 - y0);
        u0 += (nx0 - x0) * du;
        u1 -= (x1 - nx1) * du;
        v0 += (ny0 - y0) * dv;
        v1 -= (y1 - ny1) * dv;
        x0 = nx0;
        y0 = ny0;
        x1 = nx1;
        y1 = ny1;
    }
    if (x0 >= x1 || y0 >= y1) return;
    const float c[6][4] = {{x0, y0, u0, v0}, {x1, y0, u1, v0}, {x1, y1, u1, v1},
                           {x0, y0, u0, v0}, {x1, y1, u1, v1}, {x0, y1, u0, v1}};
    for (int i = 0; i < 6; i++) {
        prts_ui_vertex *v = prts_array_push(out, NULL);
        v->x = c[i][0];
        v->y = c[i][1];
        v->u = c[i][2];
        v->v = c[i][3];
        memcpy(v->rgba, rgba, 4);
    }
}

static void push_atlas(const prts_ui_font *f, prts_array *out, float x, float y, float w, float h, const int src[4],
                       const unsigned char rgba[4], const mu_Rect *clip)
{
    float aw = (float)f->atlas_w, ah = (float)f->atlas_h;
    push_quad(out, x, y, x + w, y + h, (float)src[0] / aw, (float)src[1] / ah, (float)(src[0] + src[2]) / aw,
              (float)(src[1] + src[3]) / ah, rgba, clip);
}

static void push_text(const prts_ui_font *f, prts_array *out, const char *text, float x, float y, int scale,
                      const unsigned char rgba[4], const mu_Rect *clip)
{
    for (const char *p = text; *p; p++) {
        int g = glyph_of(f, (unsigned char)*p);
        if (g < 0) continue;
        int adv = f->advance[g];
        if (*p != ' ') {
            int src[4] = {(g % f->columns) * f->cell_w, (g / f->columns) * f->cell_h, adv, f->height};
            push_atlas(f, out, x, y, (float)(adv * scale), (float)(f->height * scale), src, rgba, clip);
        }
        x += (float)(adv * scale);
    }
}

static void record_text(surface *sf, const char *text)
{
    char *copy = prts_strdup(text);
    prts_array_push(&sf->texts, &copy);
}

static void close_open_containers(prts_ui *ui)
{
    while (ui->depth > 0) {
        if (ui->stack[ui->depth - 1] == 1) prts_ui_window_end(ui);
        else prts_ui_panel_end(ui);
    }
}

void prts_ui_end(prts_ui *ui)
{
    surface *sf = get_surface(ui, ui->current);
    if (!sf) return;
    close_open_containers(ui);
    mu_Context *m = sf->mu;
    mu_end(m);
    prts_ui_batch *b = &sf->batch;
    prts_array_clear(&b->verts);
    b->width = sf->w;
    b->height = sf->h;
    b->scale = sf->scale;
    snprintf(b->texture, sizeof(b->texture), "%s", ui->font.texture);
    const int *white = ui->font.icons[PRTS_UI_ICON_WHITE];
    int white_src[4] = {white[0] + 1, white[1] + 1, 1, 1}; /* inner texel: no bleeding */
    mu_Rect clip = mu_rect(0, 0, 0x1000000, 0x1000000);
    mu_Command *at = NULL;
    while (mu_next_command(m, &at)) {
        /* microui packs commands without padding (text commands have any length), so they
         * are read through an aligned copy; a text's characters follow its fixed part */
        mu_Command cmd;
        memcpy(&cmd.base, at, sizeof(cmd.base));
        memcpy(&cmd, at, PRTS_MIN((size_t)cmd.base.size, sizeof(cmd)));
        if (cmd.type == MU_COMMAND_CLIP) {
            clip = cmd.clip.rect;
        } else if (cmd.type == MU_COMMAND_RECT) {
            mu_Rect r = cmd.rect.rect;
            const unsigned char c[4] = {cmd.rect.color.r, cmd.rect.color.g, cmd.rect.color.b, cmd.rect.color.a};
            push_atlas(&ui->font, &b->verts, (float)r.x, (float)r.y, (float)r.w, (float)r.h, white_src, c, &clip);
        } else if (cmd.type == MU_COMMAND_TEXT) {
            const unsigned char c[4] = {cmd.text.color.r, cmd.text.color.g, cmd.text.color.b, cmd.text.color.a};
            const char *str = (const char *)at + offsetof(mu_TextCommand, str);
            push_text(&ui->font, &b->verts, str, (float)cmd.text.pos.x, (float)cmd.text.pos.y, 1, c, &clip);
        } else {
            static const prts_ui_icon map[MU_ICON_MAX] = {PRTS_UI_ICON_WHITE, PRTS_UI_ICON_CLOSE, PRTS_UI_ICON_CHECK,
                                                          PRTS_UI_ICON_COLLAPSED, PRTS_UI_ICON_EXPANDED};
            const int *src = ui->font.icons[map[PRTS_CLAMP(cmd.icon.id, 0, MU_ICON_MAX - 1)]];
            mu_Rect r = cmd.icon.rect;
            const unsigned char c[4] = {cmd.icon.color.r, cmd.icon.color.g, cmd.icon.color.b, cmd.icon.color.a};
            push_atlas(&ui->font, &b->verts, (float)(r.x + (r.w - src[2]) / 2), (float)(r.y + (r.h - src[3]) / 2),
                       (float)src[2], (float)src[3], src, c, &clip);
        }
    }
    for (size_t i = 0; i < sf->top.len; i++) prts_array_push(&b->verts, prts_array_at(&sf->top, i));
    sf->hovered = m->next_hover_root != NULL;
    ui->current = -1;
}

const prts_ui_batch *prts_ui_batch_of(const prts_ui *ui, int s)
{
    surface *sf = get_surface(ui, s);
    return sf ? &sf->batch : NULL;
}

bool prts_ui_hovered(const prts_ui *ui, int s)
{
    surface *sf = get_surface(ui, s);
    return sf && sf->hovered;
}

/* --- widgets ------------------------------------------------------------------------- */

const char *prts_ui_error(const prts_ui *ui) { return ui->error; }

static mu_Context *ctx_for(prts_ui *ui, bool need_container)
{
    surface *sf = get_surface(ui, ui->current);
    if (!sf) {
        ui->error = "the UI can only be drawn while a surface is being built (in a 'ui' event handler)";
        return NULL;
    }
    if (need_container && ui->depth == 0) {
        ui->error = "widgets must be placed inside a window";
        return NULL;
    }
    if (sf->mu->command_list.idx > MU_COMMANDLIST_SIZE - COMMAND_RESERVE) {
        ui->error = "too much UI in one frame";
        return NULL;
    }
    return sf->mu;
}

static bool text_ok(prts_ui *ui, const char *text, size_t max)
{
    if (strlen(text) <= max) return true;
    ui->error = "text is too long for one UI element";
    return false;
}

/* Texts are recorded where scripts place them (also when scrolled out of view): a list on
 * the screen "lists" all of its entries. */
static void record(prts_ui *ui, const char *text) { record_text(get_surface(ui, ui->current), text); }

static void record_widget(prts_ui *ui, const char *label, const char *id, mu_Rect r)
{
    surface *sf = get_surface(ui, ui->current);
    widget_rec w = {prts_strdup(label ? label : ""), prts_strdup(id ? id : ""), r};
    prts_array_push(&sf->widgets, &w);
}

bool prts_ui_window_begin(prts_ui *ui, const char *title, int x, int y, int w, int h, unsigned flags)
{
    mu_Context *m = ctx_for(ui, false);
    if (!m) return false;
    if (ui->depth >= MAX_DEPTH || ui->windows >= MAX_WINDOWS || ui->containers >= MAX_CONTAINERS) {
        ui->error = "too many windows or panels in one frame";
        return false;
    }
    if (!*title || !text_ok(ui, title, MAX_TEXT)) {
        if (!*title) ui->error = "a window needs a title";
        return false;
    }
    int opt = MU_OPT_NORESIZE | MU_OPT_NOCLOSE;
    if (!(flags & PRTS_UI_WIN_TITLE)) opt |= MU_OPT_NOTITLE;
    if (!(flags & PRTS_UI_WIN_FRAME)) opt |= MU_OPT_NOFRAME;
    if (!(flags & PRTS_UI_WIN_SCROLL)) opt |= MU_OPT_NOSCROLL;
    if (!(flags & PRTS_UI_WIN_INPUT)) opt |= MU_OPT_NOINTERACT;
    mu_Container *cnt = mu_get_container(m, title);
    cnt->rect = mu_rect(x, y, PRTS_MAX(w, 16), PRTS_MAX(h, 16));
    cnt->open = 1;
    mu_begin_window_ex(m, title, cnt->rect, opt);
    if (flags & PRTS_UI_WIN_TITLE) record(ui, title);
    ui->windows++;
    ui->containers++;
    ui->stack[ui->depth++] = 1;
    return true;
}

void prts_ui_window_end(prts_ui *ui)
{
    surface *sf = get_surface(ui, ui->current);
    if (!sf || ui->depth == 0 || ui->stack[ui->depth - 1] != 1) return;
    mu_end_window(sf->mu);
    ui->depth--;
}

bool prts_ui_panel_begin(prts_ui *ui, const char *name, bool scroll_to_end)
{
    mu_Context *m = ctx_for(ui, true);
    if (!m) return false;
    if (ui->depth >= MAX_DEPTH || ui->containers >= MAX_CONTAINERS) {
        ui->error = "too many windows or panels in one frame";
        return false;
    }
    if (!text_ok(ui, name, MAX_TEXT)) return false;
    mu_Container *cnt = mu_get_container(m, name); /* the same id mu_begin_panel_ex uses */
    if (scroll_to_end) cnt->scroll.y = 0x1000000;  /* clamped to the content when laid out */
    mu_begin_panel_ex(m, name, 0);
    ui->containers++;
    ui->stack[ui->depth++] = 2;
    return true;
}

void prts_ui_panel_end(prts_ui *ui)
{
    surface *sf = get_surface(ui, ui->current);
    if (!sf || ui->depth == 0 || ui->stack[ui->depth - 1] != 2) return;
    mu_end_panel(sf->mu);
    ui->depth--;
}

bool prts_ui_row(prts_ui *ui, int n, const int *widths, int height)
{
    mu_Context *m = ctx_for(ui, true);
    if (!m) return false;
    if (n < 1 || n > MU_MAX_WIDTHS) {
        ui->error = "a row has 1 to 16 cells";
        return false;
    }
    mu_layout_row(m, n, widths, height);
    return true;
}

bool prts_ui_label(prts_ui *ui, const char *text, bool centered)
{
    mu_Context *m = ctx_for(ui, true);
    if (!m || !text_ok(ui, text, MAX_TEXT)) return false;
    mu_draw_control_text(m, text, mu_layout_next(m), MU_COLOR_TEXT, centered ? MU_OPT_ALIGNCENTER : 0);
    record(ui, text);
    return true;
}

bool prts_ui_text(prts_ui *ui, const char *text)
{
    mu_Context *m = ctx_for(ui, true);
    if (!m || !text_ok(ui, text, MAX_PARAGRAPH)) return false;
    mu_text(m, text);
    record(ui, text);
    return true;
}

static void push_widget_id(mu_Context *m, const char *label, const char *id)
{
    const char *key = id ? id : label;
    mu_push_id(m, key, (int)strlen(key));
}

bool prts_ui_button(prts_ui *ui, const char *label, const char *id, bool *clicked)
{
    mu_Context *m = ctx_for(ui, true);
    if (!m || !text_ok(ui, label, MAX_TEXT) || (id && !text_ok(ui, id, MAX_TEXT))) return false;
    push_widget_id(m, label, id);
    *clicked = (mu_button_ex(m, label, 0, MU_OPT_ALIGNCENTER) & MU_RES_SUBMIT) != 0;
    mu_pop_id(m);
    record(ui, label);
    record_widget(ui, label, id, m->last_rect);
    return true;
}

bool prts_ui_checkbox(prts_ui *ui, const char *label, const char *id, bool *value, bool *changed)
{
    mu_Context *m = ctx_for(ui, true);
    if (!m || !text_ok(ui, label, MAX_TEXT) || (id && !text_ok(ui, id, MAX_TEXT))) return false;
    /* like mu_checkbox, but with an id from the label or id: microui derives it from the
     * address of the state, which is a different stack slot from call to call */
    const char *key = id ? id : label;
    mu_Id mid = mu_get_id(m, key, (int)strlen(key));
    mu_Rect r = mu_layout_next(m);
    mu_Rect box = mu_rect(r.x, r.y, r.h, r.h);
    mu_update_control(m, mid, r, 0);
    *changed = m->mouse_pressed == MU_MOUSE_LEFT && m->focus == mid;
    if (*changed) *value = !*value;
    mu_draw_control_frame(m, mid, box, MU_COLOR_BASE, 0);
    if (*value) mu_draw_icon(m, MU_ICON_CHECK, box, m->style->colors[MU_COLOR_TEXT]);
    mu_draw_control_text(m, label, mu_rect(r.x + box.w, r.y, r.w - box.w, r.h), MU_COLOR_TEXT, 0);
    record(ui, label);
    record_widget(ui, label, id, r);
    return true;
}

bool prts_ui_slider(prts_ui *ui, const char *id, float *value, float lo, float hi, float step, bool *changed)
{
    mu_Context *m = ctx_for(ui, true);
    if (!m || !text_ok(ui, id, MAX_TEXT)) return false;
    if (!(hi > lo) || step < 0.0f) {
        ui->error = "a slider needs low < high and a step >= 0";
        return false;
    }
    /* like mu_slider_ex (without its shift-click number entry), with a stable id */
    mu_Id mid = mu_get_id(m, id, (int)strlen(id));
    mu_Rect base = mu_layout_next(m);
    mu_update_control(m, mid, base, MU_OPT_ALIGNCENTER);
    float v = *value;
    if (m->focus == mid && (m->mouse_down | m->mouse_pressed) == MU_MOUSE_LEFT) {
        v = lo + (float)(m->mouse_pos.x - base.x) * (hi - lo) / (float)PRTS_MAX(base.w, 1);
        if (step > 0.0f) v = lo + floorf((v - lo) / step + 0.5f) * step;
    }
    v = prts_clampf(v, lo, hi);
    *changed = v != *value;
    *value = v;
    mu_draw_control_frame(m, mid, base, MU_COLOR_BASE, MU_OPT_ALIGNCENTER);
    int thumb = m->style->thumb_size;
    int x = (int)((v - lo) * (float)(base.w - thumb) / (hi - lo));
    mu_draw_control_frame(m, mid, mu_rect(base.x + x, base.y, thumb, base.h), MU_COLOR_BUTTON, MU_OPT_ALIGNCENTER);
    char text[32];
    snprintf(text, sizeof(text), step >= 1.0f ? "%.0f" : "%.2f", (double)v);
    mu_draw_control_text(m, text, base, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);
    record_widget(ui, id, NULL, base);
    return true;
}

bool prts_ui_textbox(prts_ui *ui, const char *id, char *buf, int size, bool focus, bool *changed, bool *submitted)
{
    mu_Context *m = ctx_for(ui, true);
    if (!m || !text_ok(ui, id, MAX_TEXT)) return false;
    mu_Id mid = mu_get_id(m, id, (int)strlen(id));
    mu_Rect r = mu_layout_next(m);
    if (focus) mu_set_focus(m, mid);
    int res = mu_textbox_raw(m, buf, size, mid, r, 0);
    *changed = (res & MU_RES_CHANGE) != 0;
    *submitted = (res & MU_RES_SUBMIT) != 0;
    record_widget(ui, id, NULL, r);
    return true;
}

/* --- direct drawing ------------------------------------------------------------------- */

static surface *draw_surface(prts_ui *ui)
{
    if (ctx_for(ui, false)) return get_surface(ui, ui->current);
    return NULL;
}

bool prts_ui_draw_rect(prts_ui *ui, float x, float y, float w, float h, const unsigned char rgba[4])
{
    return prts_ui_draw_icon(ui, PRTS_UI_ICON_WHITE, x, y, w, h, rgba);
}

bool prts_ui_draw_icon(prts_ui *ui, prts_ui_icon icon, float x, float y, float w, float h, const unsigned char rgba[4])
{
    surface *sf = draw_surface(ui);
    if (!sf) return false;
    const int *src = ui->font.icons[PRTS_CLAMP((int)icon, 0, PRTS_UI_ICON_COUNT - 1)];
    int inner[4] = {src[0] + 1, src[1] + 1, 1, 1};
    push_atlas(&ui->font, &sf->top, x, y, w, h, icon == PRTS_UI_ICON_WHITE ? inner : src, rgba, NULL);
    return true;
}

bool prts_ui_draw_text(prts_ui *ui, const char *text, float x, float y, int align, int scale,
                       const unsigned char rgba[4])
{
    surface *sf = draw_surface(ui);
    if (!sf || !text_ok(ui, text, MAX_TEXT)) return false;
    scale = PRTS_CLAMP(scale, 1, 8);
    float w = (float)(prts_ui_text_width(&ui->font, text, -1) * scale);
    if (align == PRTS_UI_ALIGN_CENTER) x -= floorf(w * 0.5f);
    else if (align == PRTS_UI_ALIGN_RIGHT) x -= w;
    push_text(&ui->font, &sf->top, text, x, y, scale, rgba, NULL);
    record_text(sf, text);
    return true;
}

/* --- records ----------------------------------------------------------------------------- */

size_t prts_ui_text_count(const prts_ui *ui, int s)
{
    surface *sf = get_surface(ui, s);
    return sf ? sf->texts.len : 0;
}

const char *prts_ui_text_at(const prts_ui *ui, int s, size_t i)
{
    surface *sf = get_surface(ui, s);
    char **t = sf ? prts_array_at(&sf->texts, i) : NULL;
    return t ? *t : NULL;
}

bool prts_ui_find_widget(const prts_ui *ui, int s, const char *label, float *x, float *y)
{
    surface *sf = get_surface(ui, s);
    for (size_t pass = 0; sf && pass < 2; pass++) {
        for (size_t i = 0; i < sf->widgets.len; i++) {
            widget_rec *w = prts_array_at(&sf->widgets, i);
            if (strcmp(pass == 0 ? w->label : w->id, label) == 0) {
                *x = (float)w->rect.x + (float)w->rect.w * 0.5f;
                *y = (float)w->rect.y + (float)w->rect.h * 0.5f;
                return true;
            }
        }
    }
    return false;
}
