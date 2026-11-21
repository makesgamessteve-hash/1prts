/* 1stPRTS engine: little-endian binary writer and reader. MIT License. */
#include "prts/prts_serial.h"

#include <string.h>

void prts_w_bytes(prts_writer *w, const void *data, size_t size)
{
    if (w->len + size > w->cap) {
        size_t cap = w->cap ? w->cap : 256u;
        while (cap < w->len + size) cap *= 2u;
        w->data = prts_realloc(w->data, cap);
        w->cap = cap;
    }
    if (size) memcpy(w->data + w->len, data, size);
    w->len += size;
}

void prts_w_u8(prts_writer *w, uint8_t v) { prts_w_bytes(w, &v, 1); }

void prts_w_u32(prts_writer *w, uint32_t v)
{
    unsigned char b[4];
    for (int i = 0; i < 4; i++) b[i] = (unsigned char)(v >> (8 * i));
    prts_w_bytes(w, b, 4);
}

void prts_w_u64(prts_writer *w, uint64_t v)
{
    unsigned char b[8];
    for (int i = 0; i < 8; i++) b[i] = (unsigned char)(v >> (8 * i));
    prts_w_bytes(w, b, 8);
}

void prts_w_f32(prts_writer *w, float v)
{
    uint32_t bits;
    memcpy(&bits, &v, sizeof(bits));
    prts_w_u32(w, bits);
}

void prts_w_f64(prts_writer *w, double v)
{
    uint64_t bits;
    memcpy(&bits, &v, sizeof(bits));
    prts_w_u64(w, bits);
}

void prts_w_str(prts_writer *w, const char *s)
{
    size_t n = s ? strlen(s) : 0u;
    prts_w_u32(w, (uint32_t)n);
    prts_w_bytes(w, s, n);
}

void prts_writer_free(prts_writer *w)
{
    prts_free(w->data);
    memset(w, 0, sizeof(*w));
}

void prts_reader_init(prts_reader *r, const void *data, size_t size)
{
    r->data = data;
    r->len = size;
    r->pos = 0;
    r->failed = false;
}

bool prts_r_bytes(prts_reader *r, void *out, size_t size)
{
    if (r->failed || size > r->len - r->pos) {
        r->failed = true;
        memset(out, 0, size);
        return false;
    }
    memcpy(out, r->data + r->pos, size);
    r->pos += size;
    return true;
}

uint8_t prts_r_u8(prts_reader *r)
{
    uint8_t v;
    prts_r_bytes(r, &v, 1);
    return v;
}

uint32_t prts_r_u32(prts_reader *r)
{
    unsigned char b[4];
    prts_r_bytes(r, b, 4);
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= (uint32_t)b[i] << (8 * i);
    return v;
}

uint64_t prts_r_u64(prts_reader *r)
{
    unsigned char b[8];
    prts_r_bytes(r, b, 8);
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)b[i] << (8 * i);
    return v;
}

float prts_r_f32(prts_reader *r)
{
    uint32_t bits = prts_r_u32(r);
    float v;
    memcpy(&v, &bits, sizeof(v));
    return v;
}

double prts_r_f64(prts_reader *r)
{
    uint64_t bits = prts_r_u64(r);
    double v;
    memcpy(&v, &bits, sizeof(v));
    return v;
}

char *prts_r_str(prts_reader *r, size_t max_len)
{
    uint32_t n = prts_r_u32(r);
    if (r->failed || n > max_len || n > r->len - r->pos) {
        r->failed = true;
        return NULL;
    }
    char *s = prts_malloc((size_t)n + 1u);
    prts_r_bytes(r, s, n);
    s[n] = '\0';
    return s;
}
