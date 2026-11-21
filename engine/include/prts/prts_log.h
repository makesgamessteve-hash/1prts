/* 1stPRTS engine: logging. MIT License. */
#ifndef PRTS_LOG_H
#define PRTS_LOG_H

#include "prts_base.h"

typedef enum prts_log_level {
    PRTS_LOG_DEBUG = 0,
    PRTS_LOG_INFO,
    PRTS_LOG_WARN,
    PRTS_LOG_ERROR
} prts_log_level;

typedef void (*prts_log_sink)(prts_log_level level, const char *message, void *user);

/* Replaces the log sink. NULL restores the default (stderr). */
void prts_log_set_sink(prts_log_sink sink, void *user);
/* Listeners get every message that passes the level filter, in addition to the sink
 * (the developer console keeps its log view this way). At most 8; returns false when full. */
bool prts_log_add_listener(prts_log_sink fn, void *user);
void prts_log_remove_listener(prts_log_sink fn, void *user);
void prts_log_set_min_level(prts_log_level level);
#if defined(__GNUC__)
__attribute__((format(printf, 2, 3)))
#endif
void prts_logf(prts_log_level level, const char *fmt, ...);
const char *prts_log_level_name(prts_log_level level);
/* Number of messages logged at PRTS_LOG_ERROR since start or the last reset. */
size_t prts_log_error_count(void);
void prts_log_reset_error_count(void);

#define prts_log_debug(...) prts_logf(PRTS_LOG_DEBUG, __VA_ARGS__)
#define prts_log_info(...) prts_logf(PRTS_LOG_INFO, __VA_ARGS__)
#define prts_log_warn(...) prts_logf(PRTS_LOG_WARN, __VA_ARGS__)
#define prts_log_error(...) prts_logf(PRTS_LOG_ERROR, __VA_ARGS__)

#endif
