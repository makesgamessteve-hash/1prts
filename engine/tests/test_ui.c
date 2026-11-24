/* Unit tests: UI module (widgets, batches, limits, surfaces, hit tests), console, touch
 * controls, input schemes and blocking, log listeners. MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_console.h"
#include "prts/prts_input.h"
#include "prts/prts_log.h"
#include "prts/prts_touch.h"
#include "prts/prts_ui.h"
#include "prts/prts_vfs.h"
#include "utest.h"

static prts_ui_input pointer(float x, float y, bool down)
{
    prts_ui_input in;
    memset(&in, 0, sizeof(in));
    in.has_pointer = true;
    in.x = x;
    in.y = y;
    in.down = down;
    return in;
}

static bool has_text(const prts_ui *ui, int s, const char *text)
{
    for (size_t i = 0; i < prts_ui_text_count(ui, s); i++) {
        if (strcmp(prts_ui_text_at(ui, s, i), text) == 0) return true;
    }
    return false;
}

UTEST(ui, font_metrics_and_icons)
{
    prts_ui_font f;
    prts_ui_font_default(&f);
    ASSERT_EQ(12, prts_ui_text_width(&f, "ab", -1));
    ASSERT_EQ(6, prts_ui_text_width(&f, "ab", 1));
    ASSERT_EQ(12, prts_ui_text_width(&f, "a\xc3\xa9", -1)); /* a two-byte character counts once */
    ASSERT_EQ(6, prts_ui_text_width(&f, "\x01", -1));        /* not in the font: '?' */
    /* Latin-1 letters have their own glyphs after the 95 ASCII ones, in a block at y = 128 */
    ASSERT_EQ(2, f.range_count);
    int src[4];
    ASSERT_EQ(95 + 0xe4 - 160, prts_ui_font_glyph(&f, 0xe4, src)); /* a with diaeresis */
    ASSERT_EQ(4 * 8, src[0]); /* column 4 of row 4 */
    ASSERT_EQ(128 + 4 * 12, src[1]);
    ASSERT_EQ(6, src[2]);
    ASSERT_EQ(9, src[3]);
    ASSERT_EQ('A' - 32, prts_ui_font_glyph(&f, 'A', NULL));
    ASSERT_EQ(-1, prts_ui_font_glyph(&f, 0x100, NULL));
    ASSERT_EQ(-1, prts_ui_font_glyph(&f, 31, NULL));
    /* "Gruesse" with u-umlaut and sharp s: the sum of the advances of its five characters */
    f.advance[prts_ui_font_glyph(&f, 0xfc, NULL)] = 7;
    f.advance[prts_ui_font_glyph(&f, 0xdf, NULL)] = 5;
    ASSERT_EQ(6 + 6 + 7 + 5 + 6, prts_ui_text_width(&f, "Gr\xc3\xbc\xc3\x9f" "e", -1));
    ASSERT_EQ(6 + 6 + 7, prts_ui_text_width(&f, "Gr\xc3\xbc\xc3\x9f" "e", 4));
    /* only part of a character within len: one '?' */
    ASSERT_EQ(6 + 6 + 6, prts_ui_text_width(&f, "Gr\xc3\xbc", 3));
    /* characters beyond the font (here 3 and 4 bytes long) are one '?' each */
    ASSERT_EQ(6, prts_ui_text_width(&f, "\xe2\x82\xac", -1));     /* euro sign */
    ASSERT_EQ(6, prts_ui_text_width(&f, "\xf0\x9f\x98\x80", -1)); /* an emoji */
    /* malformed text: a '?' for each stray byte and for each cut-off sequence */
    ASSERT_EQ(12, prts_ui_text_width(&f, "\xbc\xbc", -1));
    ASSERT_EQ(12, prts_ui_text_width(&f, "\xc3" "a", -1));
    ASSERT_EQ(12, prts_ui_text_width(&f, "\xe2\x82" "a", -1));
    ASSERT_EQ(6, prts_ui_text_width(&f, "\xff", -1));
    ASSERT_EQ(12, prts_ui_text_width(&f, "\xc0\x80", -1)); /* overlong NUL */
    f.range_count = 1; /* ASCII only, as an older font */
    ASSERT_EQ(6, prts_ui_text_width(&f, "\xc3\xa4", -1));
    f.ranges[0].count = 10; /* a font without '?' */
    ASSERT_EQ(0, prts_ui_text_width(&f, "~", -1));
    prts_ui_icon icon;
    ASSERT_TRUE(prts_ui_icon_parse("disc", &icon));
    ASSERT_EQ(PRTS_UI_ICON_DISC, icon);
    ASSERT_FALSE(prts_ui_icon_parse("star", &icon));
    ASSERT_STREQ("ring", prts_ui_icon_name(PRTS_UI_ICON_RING));
    ASSERT_STREQ("ring", prts_ui_icon_name((prts_ui_icon)99));
    ASSERT_EQ(1, prts_ui_scale_for(100));
    ASSERT_EQ(2, prts_ui_scale_for(720));
    ASSERT_EQ(3, prts_ui_scale_for(1080));
}

UTEST(ui, utf8_decoding)
{
    int cp;
    ASSERT_EQ(0, prts_utf8_decode("", -1, &cp));
    ASSERT_EQ(-1, cp);
    ASSERT_EQ(0, prts_utf8_decode("a", 0, &cp));
    ASSERT_EQ(1, prts_utf8_decode("a", -1, &cp));
    ASSERT_EQ('a', cp);
    ASSERT_EQ(2, prts_utf8_decode("\xc3\xa9", -1, &cp));
    ASSERT_EQ(0xe9, cp);
    ASSERT_EQ(3, prts_utf8_decode("\xe2\x82\xac", -1, &cp));
    ASSERT_EQ(0x20ac, cp);
    ASSERT_EQ(4, prts_utf8_decode("\xf0\x9f\x98\x80", -1, &cp));
    ASSERT_EQ(0x1f600, cp);
    ASSERT_EQ(4, prts_utf8_decode("\xf4\x8f\xbf\xbf", 4, &cp));
    ASSERT_EQ(0x10ffff, cp);
    /* cut off by the length: the bytes that fit, as one malformed character */
    ASSERT_EQ(1, prts_utf8_decode("\xc3\xa9", 1, &cp));
    ASSERT_EQ(-1, cp);
    ASSERT_EQ(3, prts_utf8_decode("\xf0\x9f\x98\x80", 3, &cp));
    ASSERT_EQ(-1, cp);
    /* overlong forms, surrogates, code points beyond U+10FFFF and bytes UTF-8 never uses */
    ASSERT_EQ(1, prts_utf8_decode("\xe0\x80\x80", -1, &cp));
    ASSERT_EQ(-1, cp);
    ASSERT_EQ(1, prts_utf8_decode("\xf0\x80\x80\x80", -1, &cp));
    ASSERT_EQ(-1, cp);
    ASSERT_EQ(1, prts_utf8_decode("\xed\xa0\x80", -1, &cp));
    ASSERT_EQ(-1, cp);
    ASSERT_EQ(3, prts_utf8_decode("\xed\x9f\xbf", -1, &cp));
    ASSERT_EQ(0xd7ff, cp);
    ASSERT_EQ(1, prts_utf8_decode("\xf4\x90\x80\x80", -1, &cp));
    ASSERT_EQ(-1, cp);
    ASSERT_EQ(1, prts_utf8_decode("\xf5\x80", -1, &cp));
    ASSERT_EQ(-1, cp);
    ASSERT_EQ(1, prts_utf8_decode("\xc1\xbf", -1, &cp));
    ASSERT_EQ(-1, cp);
    ASSERT_EQ(2, prts_utf8_decode("\xe2\x82" "a", -1, &cp));
    ASSERT_EQ(-1, cp);
}

UTEST(ui, font_ranges_have_limits)
{
    prts_ui_font f;
    prts_ui_font_default(&f);
    ASSERT_FALSE(prts_ui_font_add_range(&f, 300, 0, 0, 0, NULL));  /* no characters */
    ASSERT_FALSE(prts_ui_font_add_range(&f, -1, 4, 0, 0, NULL));   /* not a code point */
    ASSERT_FALSE(prts_ui_font_add_range(&f, 300, 66, 0, 0, NULL)); /* 95 + 96 + 66 > 256 */
    const unsigned char adv[2] = {3, 4};
    ASSERT_TRUE(prts_ui_font_add_range(&f, 0x2018, 2, 0, 200, adv));
    ASSERT_EQ(95 + 96, prts_ui_font_glyph(&f, 0x2018, NULL));
    ASSERT_EQ(3 + 4, prts_ui_text_width(&f, "\xe2\x80\x98\xe2\x80\x99", -1)); /* quotes */
    while (f.range_count < PRTS_UI_FONT_RANGES) ASSERT_TRUE(prts_ui_font_add_range(&f, 0x3000, 1, 0, 0, NULL));
    ASSERT_FALSE(prts_ui_font_add_range(&f, 0x4000, 1, 0, 0, NULL)); /* no range left */
}

UTEST(ui, latin1_text_is_drawn_from_its_block)
{
    prts_ui *ui = prts_ui_create();
    const unsigned char c[4] = {255, 255, 255, 255};
    prts_ui_begin(ui, PRTS_UI_SCREEN);
    ASSERT_TRUE(prts_ui_draw_text(ui, "\xc3\xa9\xff", 0, 0, PRTS_UI_ALIGN_LEFT, 1, c));
    prts_ui_end(ui);
    const prts_ui_batch *b = prts_ui_batch_of(ui, PRTS_UI_SCREEN);
    ASSERT_EQ((size_t)12, b->verts.len); /* the e with acute and one '?' */
    const prts_ui_vertex *v = prts_array_at(&b->verts, 0);
    ASSERT_NEAR((9 * 8) / 128.0f, v->u, 1e-6f); /* U+00E9: column 9 of row 4 of the block */
    ASSERT_NEAR((128 + 4 * 12) / 256.0f, v->v, 1e-6f);
    v = prts_array_at(&b->verts, 6);
    ASSERT_NEAR((('?' - 32) % 16 * 8) / 128.0f, v->u, 1e-6f);
    ASSERT_NEAR((('?' - 32) / 16 * 12) / 256.0f, v->v, 1e-6f);
    /* a full text field drops a character that did not fit whole */
    char buf[4] = "ab";
    bool changed = false, submitted;
    int full = -1;
    for (int frame = 0; frame < 2; frame++) {
        prts_ui_input in = pointer(1, 1, false);
        in.text = frame == 1 ? "\xc3\xa9" : NULL;
        prts_ui_feed(ui, PRTS_UI_SCREEN, &in);
        prts_ui_begin(ui, PRTS_UI_SCREEN);
        ASSERT_TRUE(prts_ui_window_begin(ui, "Field", 0, 0, 200, 100, PRTS_UI_WIN_DEFAULT));
        ASSERT_TRUE(prts_ui_row(ui, 1, &full, 0));
        ASSERT_TRUE(prts_ui_textbox(ui, "field", buf, (int)sizeof(buf), true, &changed, &submitted));
        prts_ui_window_end(ui);
        prts_ui_end(ui);
    }
    ASSERT_TRUE(changed);
    ASSERT_STREQ("ab", buf);
    /* a whole character is kept */
    char wide[8] = "ab";
    for (int frame = 0; frame < 2; frame++) {
        prts_ui_input in = pointer(1, 1, false);
        in.text = frame == 1 ? "\xc3\xa9" : NULL;
        prts_ui_feed(ui, PRTS_UI_SCREEN, &in);
        prts_ui_begin(ui, PRTS_UI_SCREEN);
        ASSERT_TRUE(prts_ui_window_begin(ui, "Field", 0, 0, 200, 100, PRTS_UI_WIN_DEFAULT));
        ASSERT_TRUE(prts_ui_row(ui, 1, &full, 0));
        ASSERT_TRUE(prts_ui_textbox(ui, "field", wide, (int)sizeof(wide), true, &changed, &submitted));
        prts_ui_window_end(ui);
        prts_ui_end(ui);
    }
    ASSERT_STREQ("ab\xc3\xa9", wide);
    prts_ui_destroy(ui);
}

UTEST(ui, widgets_build_a_clipped_batch_and_react_to_input)
{
    prts_ui *ui = prts_ui_create();
    prts_ui_set_screen(ui, 1280, 720);
    int w, h;
    prts_ui_surface_size(ui, PRTS_UI_SCREEN, &w, &h);
    ASSERT_EQ(640, w);
    ASSERT_EQ(360, h);
    ASSERT_STREQ("screen", prts_ui_surface_name(ui, PRTS_UI_SCREEN));
    bool clicked = false, check = false, changed = false, submitted = false;
    float value = 2.0f;
    char buf[16] = "ab";
    int clicks = 0, toggles = 0, submits = 0;
    float bx = 0, by = 0, cx = 0, cy = 0, sx = 0, sy = 0, tx = 0, ty = 0;
    /* hovering takes two frames (microui finds the hovered window a frame late), then a press */
    for (int frame = 0; frame < 19; frame++) {
        prts_ui_input in = pointer(-50, -50, false);
        if (frame >= 2 && frame <= 5) in = pointer(bx, by, frame == 4); /* click the button */
        if (frame >= 6 && frame <= 8) in = pointer(cx, cy, frame == 8); /* the checkbox */
        if (frame >= 9 && frame <= 11) in = pointer(sx + 40, sy, frame == 11); /* the slider */
        if (frame >= 12 && frame <= 14) in = pointer(tx, ty, frame == 14); /* the text box */
        if (frame == 15) {
            in = pointer(tx, ty, false);
            in.text = "cdefghijklmnopqrstuvwxyz0123456789abcdefghijklm"; /* more than microui takes */
            in.keys = PRTS_UI_KEY_SHIFT | PRTS_UI_KEY_CTRL | PRTS_UI_KEY_ALT;
        }
        if (frame == 16) in.keys = PRTS_UI_KEY_BACKSPACE;
        if (frame == 17) in.keys = PRTS_UI_KEY_RETURN;
        in.scroll = frame == 1 ? 1.0f : 0.0f;
        prts_ui_feed(ui, PRTS_UI_SCREEN, &in);
        prts_ui_begin(ui, PRTS_UI_SCREEN);
        prts_ui_begin(ui, PRTS_UI_SCREEN); /* ignored while building */
        ASSERT_EQ(PRTS_UI_SCREEN, prts_ui_current(ui));
        ASSERT_TRUE(prts_ui_window_begin(ui, "Test window", 20, 20, 300, 220, PRTS_UI_WIN_DEFAULT));
        int widths[2] = {100, -1};
        ASSERT_TRUE(prts_ui_row(ui, 2, widths, 0));
        ASSERT_TRUE(prts_ui_label(ui, "Label", false));
        ASSERT_TRUE(prts_ui_label(ui, "Centered", true));
        int full = -1;
        ASSERT_TRUE(prts_ui_row(ui, 1, &full, 0));
        ASSERT_TRUE(prts_ui_text(ui, "A paragraph that is long enough to wrap onto a second line in this window."));
        ASSERT_TRUE(prts_ui_button(ui, "Press me", NULL, &clicked));
        clicks += clicked;
        ASSERT_TRUE(prts_ui_checkbox(ui, "Check", "check", &check, &changed));
        toggles += changed;
        ASSERT_TRUE(prts_ui_slider(ui, "slider", &value, 0.0f, 10.0f, 1.0f, &changed));
        ASSERT_TRUE(prts_ui_slider(ui, "fine", &value, 0.0f, 10.0f, 0.0f, &changed));
        bool focus = false;
        ASSERT_TRUE(prts_ui_textbox(ui, "text", buf, (int)sizeof(buf), focus, &changed, &submitted));
        submits += submitted;
        ASSERT_TRUE(prts_ui_row(ui, 1, &full, 30));
        ASSERT_TRUE(prts_ui_panel_begin(ui, "panel", true));
        for (int i = 0; i < 6; i++) prts_ui_label(ui, "line in a panel", false);
        prts_ui_panel_end(ui);
        prts_ui_window_end(ui);
        const unsigned char red[4] = {255, 0, 0, 255};
        ASSERT_TRUE(prts_ui_draw_rect(ui, 0, 0, 10, 10, red));
        ASSERT_TRUE(prts_ui_draw_icon(ui, PRTS_UI_ICON_RING, 0, 0, 16, 16, red));
        ASSERT_TRUE(prts_ui_draw_text(ui, "HUD", 320, 300, PRTS_UI_ALIGN_CENTER, 2, red));
        ASSERT_TRUE(prts_ui_draw_text(ui, "right", 640, 300, PRTS_UI_ALIGN_RIGHT, 1, red));
        prts_ui_end(ui);
        ASSERT_EQ(-1, prts_ui_current(ui));
        ASSERT_TRUE(prts_ui_find_widget(ui, PRTS_UI_SCREEN, "Press me", &bx, &by));
        ASSERT_TRUE(prts_ui_find_widget(ui, PRTS_UI_SCREEN, "check", &cx, &cy)); /* by id */
        ASSERT_TRUE(prts_ui_find_widget(ui, PRTS_UI_SCREEN, "slider", &sx, &sy));
        ASSERT_TRUE(prts_ui_find_widget(ui, PRTS_UI_SCREEN, "text", &tx, &ty));
    }
    ASSERT_EQ(1, clicks);
    ASSERT_EQ(1, toggles);
    ASSERT_TRUE(check);
    ASSERT_TRUE(value > 5.0f);
    ASSERT_EQ(1, submits);
    ASSERT_EQ(14u, (unsigned)strlen(buf)); /* "ab" + the 13 typed bytes that fit, minus one backspace */
    ASSERT_FALSE(prts_ui_find_widget(ui, PRTS_UI_SCREEN, "nothing", &bx, &by));
    ASSERT_TRUE(has_text(ui, PRTS_UI_SCREEN, "Test window"));
    ASSERT_TRUE(has_text(ui, PRTS_UI_SCREEN, "HUD"));
    ASSERT_TRUE(has_text(ui, PRTS_UI_SCREEN, "line in a panel"));
    ASSERT_TRUE(prts_ui_text_at(ui, PRTS_UI_SCREEN, 9999) == NULL);
    const prts_ui_batch *b = prts_ui_batch_of(ui, PRTS_UI_SCREEN);
    ASSERT_TRUE(b->verts.len > 100);
    ASSERT_EQ(0u, (unsigned)(b->verts.len % 6));
    ASSERT_EQ(2, b->scale);
    ASSERT_STREQ("fonts/ui.png", b->texture);
    /* every vertex of a window widget lies inside the window (CPU clipping) */
    for (size_t i = 0; i < b->verts.len; i++) {
        const prts_ui_vertex *v = prts_array_at(&b->verts, i);
        ASSERT_TRUE(v->x >= 0.0f && v->x <= 640.0f && v->y >= 0.0f && v->y <= 360.0f);
        ASSERT_TRUE(v->u >= 0.0f && v->u <= 1.0f && v->v >= 0.0f && v->v <= 1.0f);
    }
    prts_ui_feed(ui, PRTS_UI_SCREEN, &(prts_ui_input){.has_pointer = true, .x = 50, .y = 50});
    prts_ui_begin(ui, PRTS_UI_SCREEN);
    prts_ui_window_begin(ui, "Test window", 20, 20, 300, 220, PRTS_UI_WIN_DEFAULT);
    prts_ui_window_end(ui);
    prts_ui_end(ui);
    ASSERT_TRUE(prts_ui_hovered(ui, PRTS_UI_SCREEN));
    prts_ui_destroy(ui);
    prts_ui_destroy(NULL);
}

UTEST(ui, misuse_is_refused_not_fatal)
{
    prts_ui *ui = prts_ui_create();
    const unsigned char c[4] = {1, 2, 3, 4};
    bool b = false, ch = false, sub = false;
    float v = 0;
    char buf[8] = "";
    int one = 0;
    /* outside a frame */
    ASSERT_FALSE(prts_ui_label(ui, "x", false));
    ASSERT_TRUE(strstr(prts_ui_error(ui), "surface") != NULL);
    ASSERT_FALSE(prts_ui_draw_rect(ui, 0, 0, 1, 1, c));
    ASSERT_FALSE(prts_ui_window_begin(ui, "w", 0, 0, 10, 10, 0));
    prts_ui_window_end(ui);
    prts_ui_panel_end(ui);
    prts_ui_end(ui);
    prts_ui_begin(ui, 5); /* no such surface */
    ASSERT_EQ(-1, prts_ui_current(ui));
    prts_ui_begin(ui, PRTS_UI_SCREEN);
    /* outside a window */
    ASSERT_FALSE(prts_ui_button(ui, "x", NULL, &b));
    ASSERT_TRUE(strstr(prts_ui_error(ui), "window") != NULL);
    ASSERT_FALSE(prts_ui_panel_begin(ui, "p", false));
    ASSERT_FALSE(prts_ui_row(ui, 1, &one, 0));
    ASSERT_FALSE(prts_ui_window_begin(ui, "", 0, 0, 10, 10, 0));
    char huge[1200];
    memset(huge, 'x', sizeof(huge) - 1);
    huge[sizeof(huge) - 1] = '\0';
    ASSERT_FALSE(prts_ui_window_begin(ui, huge, 0, 0, 10, 10, 0));
    ASSERT_FALSE(prts_ui_draw_text(ui, huge, 0, 0, 0, 1, c));
    ASSERT_TRUE(prts_ui_window_begin(ui, "Limits", 0, 0, 300, 100000, PRTS_UI_WIN_DEFAULT));
    ASSERT_FALSE(prts_ui_row(ui, 0, &one, 0));
    ASSERT_FALSE(prts_ui_row(ui, 17, NULL, 0));
    ASSERT_FALSE(prts_ui_label(ui, huge, false));
    ASSERT_FALSE(prts_ui_button(ui, huge, NULL, &b));
    ASSERT_FALSE(prts_ui_button(ui, "ok", huge, &b));
    ASSERT_FALSE(prts_ui_checkbox(ui, huge, NULL, &b, &ch));
    ASSERT_FALSE(prts_ui_checkbox(ui, "ok", huge, &b, &ch));
    ASSERT_FALSE(prts_ui_slider(ui, huge, &v, 0, 1, 0, &ch));
    ASSERT_FALSE(prts_ui_slider(ui, "s", &v, 1, 1, 0, &ch));
    ASSERT_FALSE(prts_ui_slider(ui, "s", &v, 0, 1, -1, &ch));
    ASSERT_FALSE(prts_ui_textbox(ui, huge, buf, 8, false, &ch, &sub));
    ASSERT_FALSE(prts_ui_panel_begin(ui, huge, false));
    char para[9000];
    memset(para, 'y', sizeof(para) - 1);
    para[sizeof(para) - 1] = '\0';
    ASSERT_FALSE(prts_ui_text(ui, para));
    /* nesting is limited; the frame end closes what was left open */
    int opened = 0;
    while (prts_ui_panel_begin(ui, "nested", false)) opened++;
    ASSERT_EQ(11, opened);
    ASSERT_TRUE(strstr(prts_ui_error(ui), "too many") != NULL);
    prts_ui_window_end(ui); /* not a window on top: ignored */
    prts_ui_end(ui);
    /* a frame with more UI than fits is refused before microui would abort */
    prts_ui_begin(ui, PRTS_UI_SCREEN);
    ASSERT_TRUE(prts_ui_window_begin(ui, "Big", 0, 0, 600, 1000000, PRTS_UI_WIN_DEFAULT));
    bool refused = false;
    char line[200];
    memset(line, 'z', sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    for (int i = 0; i < 5000 && !refused; i++) refused = !prts_ui_label(ui, line, false);
    ASSERT_TRUE(refused);
    ASSERT_TRUE(strstr(prts_ui_error(ui), "too much") != NULL);
    prts_ui_end(ui);
    /* too many windows */
    prts_ui_begin(ui, PRTS_UI_SCREEN);
    int windows = 0;
    char title[16];
    for (int i = 0; i < 40; i++) {
        snprintf(title, sizeof(title), "w%d", i);
        if (!prts_ui_window_begin(ui, title, i, i, 20, 20, 0)) break;
        prts_ui_window_end(ui);
        windows++;
    }
    ASSERT_EQ(24, windows);
    prts_ui_end(ui);
    prts_ui_destroy(ui);
}

UTEST(ui, world_surfaces)
{
    prts_ui *ui = prts_ui_create();
    ASSERT_EQ(1, prts_ui_surface_count(ui));
    ASSERT_EQ(-1, prts_ui_surface_add(ui, "screen", 64, 64));
    ASSERT_EQ(-1, prts_ui_surface_add(ui, "tiny", 4, 64));
    ASSERT_EQ(-1, prts_ui_surface_add(ui, "huge", 64, 4096));
    int s = prts_ui_surface_add(ui, "war_table", 256, 128);
    ASSERT_EQ(1, s);
    ASSERT_EQ(s, prts_ui_surface_add(ui, "war_table", 128, 128));
    int w, h;
    prts_ui_surface_size(ui, s, &w, &h);
    ASSERT_EQ(128, w);
    prts_ui_surface_size(ui, 7, &w, &h);
    ASSERT_EQ(0, w);
    ASSERT_TRUE(prts_ui_surface_name(ui, 7) == NULL);
    char name[16];
    for (int i = 0; i < 7; i++) {
        snprintf(name, sizeof(name), "pool%d", i);
        int got = prts_ui_surface_add(ui, name, 32, 32);
        ASSERT_EQ(i < 6 ? i + 2 : -1, got);
    }
    ASSERT_EQ(PRTS_UI_MAX_SURFACES, prts_ui_surface_count(ui));
    ASSERT_EQ(3, prts_ui_surface_find(ui, "pool1"));
    ASSERT_EQ(-1, prts_ui_surface_find(ui, "nope"));
    prts_ui_quad q = {prts_dv3(0, 2, 0), prts_v3(2, 0, 0), prts_v3(0, -1, 0)}, out;
    ASSERT_FALSE(prts_ui_surface_quad(ui, s, &out));
    prts_ui_surface_set_quad(ui, s, &q);
    prts_ui_surface_set_quad(ui, PRTS_UI_SCREEN, &q); /* the screen has no world placement */
    ASSERT_TRUE(prts_ui_surface_quad(ui, s, &out));
    ASSERT_FALSE(prts_ui_surface_quad(ui, PRTS_UI_SCREEN, &out));
    ASSERT_EQ(2.0f, out.right.x);
    /* a surface frame */
    prts_ui_feed(ui, s, &(prts_ui_input){.has_pointer = true, .x = 10, .y = 10, .down = true});
    prts_ui_feed(ui, 99, &(prts_ui_input){0});
    prts_ui_begin(ui, s);
    const unsigned char c[4] = {9, 9, 9, 9};
    ASSERT_TRUE(prts_ui_draw_text(ui, "on the table", 0, 0, PRTS_UI_ALIGN_LEFT, 1, c));
    prts_ui_end(ui);
    ASSERT_EQ(128, prts_ui_batch_of(ui, s)->width);
    ASSERT_TRUE(prts_ui_batch_of(ui, 99) == NULL);
    ASSERT_EQ((size_t)1, prts_ui_text_count(ui, s));
    ASSERT_EQ((size_t)0, prts_ui_text_count(ui, 99));
    ASSERT_FALSE(prts_ui_hovered(ui, 99));
    ASSERT_FALSE(prts_ui_find_widget(ui, 99, "x", &out.right.x, &out.right.y));
    /* custom font metrics are used for text */
    prts_ui_font f = *prts_ui_get_font(ui);
    f.advance['A' - f.ranges[0].first] = 9;
    prts_ui_set_font(ui, &f);
    ASSERT_EQ(9, prts_ui_text_width(prts_ui_get_font(ui), "A", -1));
    prts_ui_destroy(ui);
}

UTEST(ui, ray_hit_tests_on_surfaces)
{
    /* a 2 m x 1 m table top, horizontal at y = 1, top-left corner at (-1, 1, -0.5) */
    prts_ui_quad table = {prts_dv3(-1, 1, -0.5), prts_v3(2, 0, 0), prts_v3(0, 0, 1)};
    float u, v, d;
    ASSERT_TRUE(prts_ui_ray_quad(&table, prts_dv3(0, 2, 0), prts_v3(0, -1, 0), 5.0f, &u, &v, &d));
    ASSERT_NEAR(0.5f, u, 1e-5f);
    ASSERT_NEAR(0.5f, v, 1e-5f);
    ASSERT_NEAR(1.0f, d, 1e-5f);
    ASSERT_TRUE(prts_ui_ray_quad(&table, prts_dv3(-0.5, 2, 0.25), prts_v3(0, -1, 0), 5.0f, &u, &v, &d));
    ASSERT_NEAR(0.25f, u, 1e-5f);
    ASSERT_NEAR(0.75f, v, 1e-5f);
    ASSERT_TRUE(prts_ui_ray_quad(&table, prts_dv3(0, 0, 0), prts_v3(0, 1, 0), 5.0f, &u, &v, &d)); /* from below */
    ASSERT_FALSE(prts_ui_ray_quad(&table, prts_dv3(3, 2, 0), prts_v3(0, -1, 0), 5.0f, &u, &v, &d)); /* beside */
    ASSERT_FALSE(prts_ui_ray_quad(&table, prts_dv3(0, 2, 0), prts_v3(0, 1, 0), 5.0f, &u, &v, &d));  /* away */
    ASSERT_FALSE(prts_ui_ray_quad(&table, prts_dv3(0, 9, 0), prts_v3(0, -1, 0), 5.0f, &u, &v, &d)); /* too far */
    ASSERT_FALSE(prts_ui_ray_quad(&table, prts_dv3(0, 2, 0), prts_v3(1, 0, 0), 5.0f, &u, &v, &d));  /* parallel */
    /* a slanted, non-rectangular surface, 64 km from the origin (camera-relative precision) */
    prts_ui_quad mirror = {prts_dv3(64000, 2, 64000), prts_v3(1, 0, 0), prts_v3(0.5, -1, 0)};
    ASSERT_TRUE(prts_ui_ray_quad(&mirror, prts_dv3(64000.75, 1.5, 64001), prts_v3(0, 0, -1), 5.0f, &u, &v, &d));
    ASSERT_NEAR(0.5f, u, 1e-3f);
    ASSERT_NEAR(0.5f, v, 1e-3f);
    ASSERT_NEAR(1.0f, d, 1e-3f);
    /* screen rays: the centre looks forward, the corners spread by the field of view */
    float fov = (float)(PRTS_PI / 2.0);
    prts_vec3 c = prts_ui_screen_ray(0.0f, 0.0f, fov, 1.0f, 0.5f, 0.5f);
    ASSERT_NEAR(-1.0f, c.z, 1e-5f);
    prts_vec3 tl = prts_ui_screen_ray(0.0f, 0.0f, fov, 1.0f, 0.0f, 0.0f);
    ASSERT_NEAR(tl.y, -tl.x, 1e-5f);
    ASSERT_TRUE(tl.x < 0.0f && tl.y > 0.0f);
    ASSERT_NEAR(1.0f, prts_v3_length(tl), 1e-5f);
}

static int g_runs;
static char g_last[64];
static void exec_line(const char *line, void *user)
{
    g_runs += user != NULL;
    snprintf(g_last, sizeof(g_last), "%s", line);
}

UTEST(ui, console_log_view_and_command_line)
{
    prts_console *c = prts_console_create(4, exec_line, &g_runs);
    ASSERT_FALSE(prts_console_is_open(c));
    prts_console_print(c, PRTS_LOG_INFO, "one");
    prts_console_print(c, PRTS_LOG_WARN, "two\nthree");
    prts_console_log_listener(PRTS_LOG_ERROR, "four", c);
    ASSERT_EQ((size_t)4, prts_console_line_count(c));
    prts_console_print(c, PRTS_LOG_DEBUG, "five"); /* the oldest line is dropped */
    ASSERT_STREQ("[warn] two", prts_console_line(c, 0));
    ASSERT_STREQ("  three", prts_console_line(c, 1));
    ASSERT_STREQ("[error] four", prts_console_line(c, 2));
    ASSERT_STREQ("five", prts_console_line(c, 3));
    ASSERT_TRUE(prts_console_line(c, 4) == NULL);
    prts_console_submit(c, "   ");
    ASSERT_EQ(0, g_runs);
    prts_console_submit(c, "  print(1)");
    ASSERT_EQ(1, g_runs);
    ASSERT_STREQ("print(1)", g_last);
    ASSERT_STREQ("print(1)", prts_console_last_command(c));
    ASSERT_STREQ("> print(1)", prts_console_line(c, 3));
    prts_console_clear(c);
    ASSERT_EQ((size_t)0, prts_console_line_count(c));
    ASSERT_STREQ("", prts_console_line(c, 0) ? "x" : "");
    /* drawn: typing and Return run the line */
    prts_ui *ui = prts_ui_create();
    prts_console_draw(c, ui, 640, 360); /* closed: nothing */
    prts_console_set_open(c, true);
    const char *typed[] = {"", "observer", "", ""};
    for (int frame = 0; frame < 4; frame++) {
        prts_ui_input in = pointer(-5, -5, false);
        in.text = typed[frame];
        in.keys = frame == 2 ? PRTS_UI_KEY_RETURN : 0;
        prts_ui_feed(ui, PRTS_UI_SCREEN, &in);
        prts_ui_begin(ui, PRTS_UI_SCREEN);
        prts_console_draw(c, ui, 640, 360);
        prts_ui_end(ui);
    }
    ASSERT_EQ(2, g_runs);
    ASSERT_STREQ("observer", g_last);
    ASSERT_TRUE(has_text(ui, PRTS_UI_SCREEN, "> observer"));
    /* the Run button too */
    float x = 0, y = 0;
    ASSERT_TRUE(prts_ui_find_widget(ui, PRTS_UI_SCREEN, "console.run", &x, &y));
    for (int frame = 0; frame < 4; frame++) {
        prts_ui_input in = pointer(x, y, frame == 2);
        in.text = frame == 0 ? "help" : NULL;
        prts_ui_feed(ui, PRTS_UI_SCREEN, &in);
        prts_ui_begin(ui, PRTS_UI_SCREEN);
        prts_console_draw(c, ui, 640, 360);
        prts_ui_end(ui);
    }
    ASSERT_STREQ("help", g_last);
    /* a window that cannot open draws nothing */
    prts_console_draw(c, ui, 640, 360);
    prts_ui_destroy(ui);
    prts_console_destroy(c);
    prts_console_destroy(NULL);
    prts_console *quiet = prts_console_create(0, NULL, NULL);
    prts_console_submit(quiet, "nothing runs");
    ASSERT_EQ((size_t)1, prts_console_line_count(quiet));
    prts_console_destroy(quiet);
}

static void finger(prts_input_state *s, int slot, float x, float y, bool down)
{
    s->touches[slot].id = slot + 100;
    s->touches[slot].x = x;
    s->touches[slot].y = y;
    s->touches[slot].down = down;
}

UTEST(touch, move_and_look_at_the_same_time)
{
    /* Spec: Touch movement and look */
    prts_touch *t = prts_touch_create();
    prts_touch_layout l;
    prts_touch_layout_default(&l);
    l.button_count = 2;
    snprintf(l.buttons[0].id, sizeof(l.buttons[0].id), "%s", "use");
    l.buttons[0].x = 0.9f;
    l.buttons[0].y = 0.5f;
    l.buttons[0].radius = 0.1f;
    snprintf(l.buttons[1].id, sizeof(l.buttons[1].id), "%s", "jump");
    l.buttons[1].x = 0.9f;
    l.buttons[1].y = 0.9f;
    l.buttons[1].radius = 0.05f;
    prts_touch_set_layout(t, &l);
    ASSERT_EQ(0, prts_touch_find_button(t, "use"));
    ASSERT_EQ(-1, prts_touch_find_button(t, "nope"));
    prts_touch_set_visible(t, 1, true);
    prts_touch_set_visible(t, 7, true); /* no such button */
    prts_input_state s;
    memset(&s, 0, sizeof(s));
    float aspect = 16.0f / 9.0f;
    finger(&s, 0, 0.2f, 0.6f, true); /* left: stick */
    finger(&s, 1, 0.7f, 0.5f, true); /* right: look */
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    ASSERT_EQ(0.0f, s.touch_axes[0]);
    float ox, oy, kx, ky;
    ASSERT_TRUE(prts_touch_stick(t, &ox, &oy, &kx, &ky));
    ASSERT_EQ(0.2f, ox);
    finger(&s, 0, 0.2f, 0.54f, true); /* half way up */
    finger(&s, 1, 0.75f, 0.5f, true); /* drag right */
    finger(&s, 2, 0.3f, 0.5f, true);  /* a second left finger does not take the stick */
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    ASSERT_NEAR(-0.5f, s.touch_axes[1], 1e-4f);
    ASSERT_NEAR(0.0f, s.touch_axes[0], 1e-4f);
    ASSERT_NEAR(0.05f * aspect * 1000.0f, s.touch_axes[2], 1e-2f);
    ASSERT_NEAR(0.0f, s.touch_axes[3], 1e-4f);
    /* an input mapper with the default touch bindings sees moving and looking together */
    prts_input *in = prts_input_create(NULL);
    prts_input_define(in, "move_forward");
    prts_input_define(in, "look_x");
    ASSERT_TRUE(prts_input_bind(in, "move_forward", PRTS_BIND_TOUCH_AXIS, NULL, 1, -1.0f));
    ASSERT_TRUE(prts_input_bind(in, "look_x", PRTS_BIND_TOUCH_AXIS, NULL, 2, 1.0f));
    ASSERT_FALSE(prts_input_bind(in, "look_x", PRTS_BIND_TOUCH_AXIS, NULL, 4, 1.0f));
    prts_input_update(in, &s);
    ASSERT_NEAR(0.5f, prts_input_value(in, "move_forward"), 1e-4f);
    ASSERT_TRUE(prts_input_value(in, "look_x") > 50.0f);
    /* past the radius the stick is clamped to full deflection */
    finger(&s, 0, 0.2f + 0.5f / aspect, 0.6f, true);
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    ASSERT_NEAR(1.0f, s.touch_axes[0], 1e-4f);
    ASSERT_EQ(0.0f, s.touch_axes[2]); /* look only reports movement */
    /* lifting: a dragged finger is no tap */
    finger(&s, 1, 0.75f, 0.5f, false);
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    ASSERT_FALSE(s.touch_buttons[0]);
    /* a short, still press on the right is a tap (touch button 0) for one update */
    finger(&s, 3, 0.6f, 0.3f, true);
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    finger(&s, 3, 0.6f, 0.3f, false);
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    ASSERT_TRUE(s.touch_buttons[0]);
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    ASSERT_FALSE(s.touch_buttons[0]);
    /* buttons: a hidden one is ignored (the finger looks instead), a visible one is held */
    finger(&s, 4, 0.9f, 0.5f, true);
    finger(&s, 5, 0.9f, 0.9f, true);
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    ASSERT_FALSE(s.touch_buttons[1]);
    ASSERT_TRUE(s.touch_buttons[2]);
    ASSERT_TRUE(prts_touch_button_held(t, 1));
    ASSERT_FALSE(prts_touch_button_held(t, 0));
    /* everything up: no stick */
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) s.touches[i].down = false;
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    ASSERT_FALSE(prts_touch_stick(t, &ox, &oy, &kx, &ky));
    ASSERT_EQ(2, prts_touch_get_layout(t)->button_count);
    /* layouts are clamped */
    l.button_count = 99;
    l.stick_radius = 0.0f;
    prts_touch_set_layout(t, &l);
    ASSERT_EQ(PRTS_TOUCH_MAX_BUTTONS, prts_touch_get_layout(t)->button_count);
    ASSERT_TRUE(prts_touch_get_layout(t)->stick_radius > 0.0f);
    /* more fingers than slots are ignored */
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) finger(&s, i, 0.7f, 0.1f * (float)i, true);
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    for (int i = 0; i < PRTS_INPUT_MAX_TOUCHES; i++) s.touches[i].id += 50; /* all new ids */
    prts_touch_update(t, &s, 1.0f / 30.0f, aspect);
    prts_input_destroy(in);
    prts_touch_destroy(t);
}

static int fake_keys(const char *name) { return strcmp(name, "E") == 0 ? 8 : 0; }

UTEST(input, schemes_blocking_and_binding_queries)
{
    prts_input_scheme sc;
    ASSERT_TRUE(prts_input_scheme_parse("touch", &sc));
    ASSERT_EQ(PRTS_SCHEME_TOUCH, sc);
    ASSERT_TRUE(prts_input_scheme_parse("xr", &sc));
    ASSERT_FALSE(prts_input_scheme_parse("wheel", &sc));
    ASSERT_EQ(PRTS_SCHEME_KBM, prts_binding_scheme(PRTS_BIND_MOUSE_AXIS));
    ASSERT_EQ(PRTS_SCHEME_GAMEPAD, prts_binding_scheme(PRTS_BIND_PAD_AXIS));
    ASSERT_EQ(PRTS_SCHEME_TOUCH, prts_binding_scheme(PRTS_BIND_TOUCH_BUTTON));
    prts_input *in = prts_input_create(fake_keys);
    prts_input_define(in, "use");
    prts_input_define(in, "menu");
    ASSERT_TRUE(prts_input_bind(in, "use", PRTS_BIND_KEY, "E", 0, 1.0f));
    ASSERT_TRUE(prts_input_bind(in, "use", PRTS_BIND_PAD_BUTTON, NULL, 2, 1.0f));
    ASSERT_TRUE(prts_input_bind(in, "use", PRTS_BIND_TOUCH_BUTTON, NULL, 0, 1.0f));
    ASSERT_TRUE(prts_input_bind(in, "menu", PRTS_BIND_KEY, "E", 0, 1.0f));
    ASSERT_EQ((size_t)3, prts_input_binding_count(in, "use"));
    ASSERT_EQ((size_t)0, prts_input_binding_count(in, "nothing"));
    prts_binding_kind k;
    int code;
    float scale;
    ASSERT_TRUE(prts_input_binding(in, "use", 1, &k, &code, &scale));
    ASSERT_EQ(PRTS_BIND_PAD_BUTTON, k);
    ASSERT_EQ(2, code);
    ASSERT_FALSE(prts_input_binding(in, "use", 3, &k, &code, &scale));
    ASSERT_FALSE(prts_input_binding(in, "nothing", 0, &k, &code, &scale));
    prts_input_clear_scheme(in, "use", PRTS_SCHEME_GAMEPAD);
    prts_input_clear_scheme(in, "nothing", PRTS_SCHEME_GAMEPAD);
    ASSERT_EQ((size_t)2, prts_input_binding_count(in, "use"));
    ASSERT_TRUE(prts_input_binding(in, "use", 1, &k, &code, &scale));
    ASSERT_EQ(PRTS_BIND_TOUCH_BUTTON, k);
    /* a tap drives the action like a key */
    prts_input_state s;
    memset(&s, 0, sizeof(s));
    s.touch_buttons[0] = true;
    prts_input_update(in, &s);
    ASSERT_TRUE(prts_input_went_down(in, "use"));
    ASSERT_FALSE(prts_input_went_down(in, "nothing"));
    prts_input_update(in, &s);
    ASSERT_FALSE(prts_input_went_down(in, "use"));
    /* blocked by a menu: only "always" actions work; test overrides still apply */
    memset(&s, 0, sizeof(s));
    s.keys[8] = true;
    prts_input_set_always(in, "menu", true);
    prts_input_set_always(in, "nothing", true);
    prts_input_set_blocked(in, true);
    ASSERT_TRUE(prts_input_blocked(in));
    prts_input_update(in, &s);
    ASSERT_FALSE(prts_input_down(in, "use"));
    ASSERT_TRUE(prts_input_down(in, "menu"));
    prts_input_override(in, "use", 1.0f, 2);
    prts_input_update(in, &s);
    ASSERT_TRUE(prts_input_down(in, "use"));
    prts_input_set_blocked(in, false);
    ASSERT_FALSE(prts_input_blocked(in));
    prts_input_destroy(in);
}

static int g_heard;
static void listener(prts_log_level level, const char *message, void *user)
{
    PRTS_UNUSED(level);
    PRTS_UNUSED(message);
    g_heard += user == &g_heard;
}

UTEST(log, listeners_hear_every_message)
{
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    ASSERT_TRUE(prts_log_add_listener(listener, &g_heard));
    prts_log_info("filtered out");
    prts_log_set_min_level(PRTS_LOG_DEBUG);
    prts_log_set_sink(listener, NULL);
    prts_log_debug("heard");
    prts_log_set_sink(NULL, NULL);
    ASSERT_EQ(1, g_heard);
    int added = 1;
    while (prts_log_add_listener(listener, NULL)) added++;
    ASSERT_EQ(8, added);
    prts_log_remove_listener(listener, NULL);
    prts_log_remove_listener(listener, &g_heard);
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_log_debug("not heard");
    ASSERT_EQ(1, g_heard);
}

UTEST(vfs, read_from_one_mount)
{
    prts_vfs *vfs = prts_vfs_create();
    ASSERT_TRUE(prts_vfs_mount(vfs, PRTS_TEST_DATA "/base", "base"));
    char *data = NULL;
    size_t size = 0;
    ASSERT_EQ(PRTS_VFS_OK, prts_vfs_read_mount(vfs, 0, "CREDITS.md", &data, &size));
    ASSERT_TRUE(size > 100 && strstr(data, "Credits") != NULL);
    prts_free(data);
    ASSERT_EQ(PRTS_VFS_NOT_FOUND, prts_vfs_read_mount(vfs, 0, "nothing.md", &data, NULL));
    ASSERT_EQ(PRTS_VFS_NOT_FOUND, prts_vfs_read_mount(vfs, 3, "CREDITS.md", &data, NULL));
    ASSERT_EQ(PRTS_VFS_INVALID_PATH, prts_vfs_read_mount(vfs, 0, "../x", &data, NULL));
    ASSERT_EQ(PRTS_VFS_NOT_FOUND, prts_vfs_read_mount(vfs, 0, "scripts", &data, NULL)); /* a folder */
    prts_vfs_destroy(vfs);
}
