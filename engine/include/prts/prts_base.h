/* 1stPRTS engine: basic types, memory helpers and macros. MIT License. */
#ifndef PRTS_BASE_H
#define PRTS_BASE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PRTS_ENGINE_VERSION "1.0.0"
/* Major version of the Lua scripting API (prts.api_version). */
#define PRTS_API_VERSION 1

#define PRTS_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define PRTS_UNUSED(x) ((void)(x))
#define PRTS_MIN(a, b) ((a) < (b) ? (a) : (b))
#define PRTS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define PRTS_CLAMP(v, lo, hi) PRTS_MIN(PRTS_MAX((v), (lo)), (hi))

/* Allocation helpers. They never return NULL: running out of memory aborts. */
void *prts_malloc(size_t size);
void *prts_calloc(size_t count, size_t size);
void *prts_realloc(void *ptr, size_t size);
void prts_free(void *ptr);
char *prts_strdup(const char *s);
char *prts_strndup(const char *s, size_t n);

/* Growable array of fixed-size elements. */
typedef struct prts_array {
    unsigned char *data;
    size_t len;
    size_t cap;
    size_t elem;
} prts_array;

void prts_array_init(prts_array *a, size_t elem_size);
/* Appends a copy of elem (or a zeroed element if elem is NULL); returns it. */
void *prts_array_push(prts_array *a, const void *elem);
void *prts_array_at(const prts_array *a, size_t index);
void prts_array_clear(prts_array *a);
void prts_array_free(prts_array *a);

/* Growable, always NUL-terminated string. */
typedef struct prts_str {
    char *data;
    size_t len;
    size_t cap;
} prts_str;

void prts_str_append(prts_str *s, const char *text);
void prts_str_append_n(prts_str *s, const char *text, size_t n);
void prts_str_appendf(prts_str *s, const char *fmt, ...);
const char *prts_str_cstr(const prts_str *s);
void prts_str_free(prts_str *s);

bool prts_str_starts_with(const char *s, const char *prefix);
bool prts_str_ends_with(const char *s, const char *suffix);
/* Joins two file-system path parts with exactly one '/'. Caller frees. */
char *prts_path_join(const char *a, const char *b);

#endif
