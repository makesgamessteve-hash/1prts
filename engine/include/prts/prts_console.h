/* 1stPRTS engine: the developer console (log view and command line). MIT License. */
#ifndef PRTS_CONSOLE_H
#define PRTS_CONSOLE_H

#include "prts_log.h"
#include "prts_ui.h"

#define PRTS_CONSOLE_INPUT_MAX 256

typedef struct prts_console prts_console;
/* Runs one submitted command line. */
typedef void (*prts_console_exec_fn)(const char *line, void *user);

/* Keeps the last max_lines lines of output (older lines are dropped). */
prts_console *prts_console_create(size_t max_lines, prts_console_exec_fn exec, void *user);
void prts_console_destroy(prts_console *c);

/* Adds output; text with newlines becomes several lines. Warnings and errors are prefixed. */
void prts_console_print(prts_console *c, prts_log_level level, const char *text);
/* A prts_log listener that prints every log message into the console given as user. */
void prts_console_log_listener(prts_log_level level, const char *message, void *user);
void prts_console_clear(prts_console *c);
size_t prts_console_line_count(const prts_console *c);
const char *prts_console_line(const prts_console *c, size_t i); /* 0 is the oldest */

void prts_console_set_open(prts_console *c, bool open);
bool prts_console_is_open(const prts_console *c);
/* Echoes the line, runs it through the exec callback and remembers it. */
void prts_console_submit(prts_console *c, const char *line);
const char *prts_console_last_command(const prts_console *c);

/* Draws the open console over the top half of the surface being built. */
void prts_console_draw(prts_console *c, prts_ui *ui, int width, int height);

#endif
