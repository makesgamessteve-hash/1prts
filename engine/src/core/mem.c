/* 1stPRTS engine: memory, arrays and strings. MIT License. */
#include "prts/prts_base.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *prts_check(void *p)
{
    if (!p) {
        fputs("1stprts: out of memory\n", stderr);
        abort();
    }
    return p;
}

void *prts_malloc(size_t size) { return prts_check(malloc(size ? size : 1)); }
void *prts_calloc(size_t count, size_t size) { return prts_check(calloc(count ? count : 1, size ? size : 1)); }
void *prts_realloc(void *ptr, size_t size) { return prts_check(realloc(ptr, size ? size : 1)); }
void prts_free(void *ptr) { free(ptr); }

char *prts_strndup(const char *s, size_t n)
{
    size_t len = strlen(s);
    if (len > n) len = n;
    char *out = prts_malloc(len + 1);
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}

char *prts_strdup(const char *s) { return prts_strndup(s, strlen(s)); }

void prts_array_init(prts_array *a, size_t elem_size)
{
    memset(a, 0, sizeof(*a));
    a->elem = elem_size;
}

void *prts_array_push(prts_array *a, const void *elem)
{
    if (a->len == a->cap) {
        a->cap = a->cap ? a->cap * 2 : 8;
        a->data = prts_realloc(a->data, a->cap * a->elem);
    }
    unsigned char *slot = a->data + a->len * a->elem;
    if (elem) memcpy(slot, elem, a->elem);
    else memset(slot, 0, a->elem);
    a->len++;
    return slot;
}

void *prts_array_at(const prts_array *a, size_t index)
{
    return index < a->len ? a->data + index * a->elem : NULL;
}

void prts_array_clear(prts_array *a) { a->len = 0; }

void prts_array_free(prts_array *a)
{
    prts_free(a->data);
    size_t elem = a->elem;
    prts_array_init(a, elem);
}

static void prts_str_reserve(prts_str *s, size_t extra)
{
    size_t need = s->len + extra + 1;
    if (need <= s->cap) return;
    size_t cap = s->cap ? s->cap : 32;
    while (cap < need) cap *= 2;
    s->data = prts_realloc(s->data, cap);
    s->cap = cap;
}

void prts_str_append_n(prts_str *s, const char *text, size_t n)
{
    prts_str_reserve(s, n);
    memcpy(s->data + s->len, text, n);
    s->len += n;
    s->data[s->len] = '\0';
}

void prts_str_append(prts_str *s, const char *text) { prts_str_append_n(s, text, strlen(text)); }

void prts_str_appendf(prts_str *s, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n > 0) {
        prts_str_reserve(s, (size_t)n);
        vsnprintf(s->data + s->len, (size_t)n + 1, fmt, ap2);
        s->len += (size_t)n;
    }
    va_end(ap2);
}

const char *prts_str_cstr(const prts_str *s) { return s->data ? s->data : ""; }

void prts_str_free(prts_str *s)
{
    prts_free(s->data);
    memset(s, 0, sizeof(*s));
}

bool prts_str_starts_with(const char *s, const char *prefix)
{
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

bool prts_str_ends_with(const char *s, const char *suffix)
{
    size_t ls = strlen(s), lx = strlen(suffix);
    return lx <= ls && memcmp(s + ls - lx, suffix, lx) == 0;
}

char *prts_path_join(const char *a, const char *b)
{
    prts_str out = {0};
    prts_str_append(&out, a);
    while (out.len > 1 && (out.data[out.len - 1] == '/' || out.data[out.len - 1] == '\\')) out.data[--out.len] = '\0';
    while (*b == '/' || *b == '\\') b++;
    if (out.len > 0 && *b) prts_str_append(&out, "/");
    prts_str_append(&out, b);
    return out.data ? out.data : prts_strdup("");
}
