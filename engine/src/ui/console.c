/* 1stPRTS engine: the developer console (log view and command line). MIT License. */
#include "prts/prts_console.h"

#include <string.h>

struct prts_console {
    char **lines; /* ring buffer */
    size_t max, count, head;
    prts_console_exec_fn exec;
    void *user;
    bool open;
    int scroll; /* frames left to keep the log scrolled to the end (new output) */
    char input[PRTS_CONSOLE_INPUT_MAX];
    char *last_command;
};

prts_console *prts_console_create(size_t max_lines, prts_console_exec_fn exec, void *user)
{
    prts_console *c = prts_calloc(1, sizeof(*c));
    c->max = max_lines > 0 ? max_lines : 1;
    c->lines = prts_calloc(c->max, sizeof(char *));
    c->exec = exec;
    c->user = user;
    return c;
}

void prts_console_clear(prts_console *c)
{
    for (size_t i = 0; i < c->max; i++) {
        prts_free(c->lines[i]);
        c->lines[i] = NULL;
    }
    c->count = c->head = 0;
}

void prts_console_destroy(prts_console *c)
{
    if (!c) return;
    prts_console_clear(c);
    prts_free(c->lines);
    prts_free(c->last_command);
    prts_free(c);
}

static void push_line(prts_console *c, const char *prefix, const char *text, size_t len)
{
    size_t slot = (c->head + c->count) % c->max;
    if (c->count == c->max) {
        slot = c->head;
        c->head = (c->head + 1) % c->max;
    } else {
        c->count++;
    }
    prts_free(c->lines[slot]);
    prts_str s = {0};
    prts_str_append(&s, prefix);
    prts_str_append_n(&s, text, PRTS_MIN(len, (size_t)1000)); /* the UI shows at most 1 KiB per line */
    c->lines[slot] = s.data ? s.data : prts_strdup("");
    c->scroll = 2; /* the panel learns its new height a frame later */
}

void prts_console_print(prts_console *c, prts_log_level level, const char *text)
{
    const char *prefix = level == PRTS_LOG_ERROR ? "[error] " : level == PRTS_LOG_WARN ? "[warn] " : "";
    for (;;) {
        const char *nl = strchr(text, '\n');
        size_t len = nl ? (size_t)(nl - text) : strlen(text);
        push_line(c, prefix, text, len);
        if (!nl) break;
        text = nl + 1;
        prefix = "  ";
    }
}

void prts_console_log_listener(prts_log_level level, const char *message, void *user)
{
    prts_console_print(user, level, message);
}

size_t prts_console_line_count(const prts_console *c) { return c->count; }

const char *prts_console_line(const prts_console *c, size_t i)
{
    return i < c->count ? c->lines[(c->head + i) % c->max] : NULL;
}

void prts_console_set_open(prts_console *c, bool open)
{
    c->open = open;
    c->scroll = 2;
}

bool prts_console_is_open(const prts_console *c) { return c->open; }

void prts_console_submit(prts_console *c, const char *line)
{
    while (*line == ' ') line++;
    if (!*line) return;
    char *copy = prts_strdup(line);
    prts_free(c->last_command);
    c->last_command = copy;
    prts_str echo = {0};
    prts_str_appendf(&echo, "> %s", copy);
    prts_console_print(c, PRTS_LOG_INFO, echo.data);
    prts_str_free(&echo);
    if (c->exec) c->exec(copy, c->user);
}

const char *prts_console_last_command(const prts_console *c) { return c->last_command ? c->last_command : ""; }

void prts_console_draw(prts_console *c, prts_ui *ui, int width, int height)
{
    if (!c->open) return;
    int h = PRTS_MAX(height / 2, 80);
    if (!prts_ui_window_begin(ui, "Developer console", 4, 4, PRTS_MAX(width - 8, 80), h,
                              PRTS_UI_WIN_TITLE | PRTS_UI_WIN_FRAME | PRTS_UI_WIN_INPUT)) {
        return;
    }
    int full = -1;
    prts_ui_row(ui, 1, &full, -26);
    prts_ui_panel_begin(ui, "console.log", c->scroll > 0);
    c->scroll = PRTS_MAX(c->scroll - 1, 0);
    prts_ui_row(ui, 1, &full, 0);
    for (size_t i = 0; i < c->count; i++) prts_ui_label(ui, prts_console_line(c, i), false);
    prts_ui_panel_end(ui);
    int widths[2] = {-52, -1};
    prts_ui_row(ui, 2, widths, 0);
    bool changed = false, submitted = false, run = false;
    prts_ui_textbox(ui, "console.input", c->input, (int)sizeof(c->input), true, &changed, &submitted);
    prts_ui_button(ui, "Run", "console.run", &run);
    prts_ui_window_end(ui);
    if (submitted || run) {
        char line[PRTS_CONSOLE_INPUT_MAX];
        memcpy(line, c->input, sizeof(line));
        c->input[0] = '\0';
        prts_console_submit(c, line);
    }
}
