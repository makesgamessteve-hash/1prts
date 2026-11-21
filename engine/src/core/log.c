/* 1stPRTS engine: logging. MIT License. */
#include "prts/prts_log.h"

#include <stdarg.h>
#include <stdio.h>

static prts_log_sink g_sink;
static void *g_sink_user;
static prts_log_level g_min_level = PRTS_LOG_INFO;
static size_t g_error_count;
#define MAX_LISTENERS 8
static struct {
    prts_log_sink fn;
    void *user;
} g_listeners[MAX_LISTENERS];

void prts_log_set_sink(prts_log_sink sink, void *user)
{
    g_sink = sink;
    g_sink_user = user;
}

void prts_log_set_min_level(prts_log_level level) { g_min_level = level; }

bool prts_log_add_listener(prts_log_sink fn, void *user)
{
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (!g_listeners[i].fn) {
            g_listeners[i].fn = fn;
            g_listeners[i].user = user;
            return true;
        }
    }
    return false;
}

void prts_log_remove_listener(prts_log_sink fn, void *user)
{
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (g_listeners[i].fn == fn && g_listeners[i].user == user) g_listeners[i].fn = NULL;
    }
}

const char *prts_log_level_name(prts_log_level level)
{
    switch (level) {
    case PRTS_LOG_DEBUG: return "debug";
    case PRTS_LOG_INFO: return "info";
    case PRTS_LOG_WARN: return "warn";
    default: return "error";
    }
}

void prts_logf(prts_log_level level, const char *fmt, ...)
{
    if (level == PRTS_LOG_ERROR) g_error_count++;
    if (level < g_min_level) return;
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (g_sink) g_sink(level, buf, g_sink_user);
    else fprintf(stderr, "[%s] %s\n", prts_log_level_name(level), buf);
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (g_listeners[i].fn) g_listeners[i].fn(level, buf, g_listeners[i].user);
    }
}

size_t prts_log_error_count(void) { return g_error_count; }
void prts_log_reset_error_count(void) { g_error_count = 0; }
