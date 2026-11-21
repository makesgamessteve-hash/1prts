/* 1stPRTS engine: little-endian binary writer and reader (saves, replays). MIT License. */
#ifndef PRTS_SERIAL_H
#define PRTS_SERIAL_H

#include "prts_base.h"

/* Growable byte buffer. Numbers are written little-endian on every platform, floats as their
 * IEEE-754 bit patterns, so a value reads back bit for bit. */
typedef struct prts_writer {
    unsigned char *data;
    size_t len, cap;
} prts_writer;

void prts_w_bytes(prts_writer *w, const void *data, size_t size);
void prts_w_u8(prts_writer *w, uint8_t v);
void prts_w_u32(prts_writer *w, uint32_t v);
void prts_w_u64(prts_writer *w, uint64_t v);
void prts_w_f32(prts_writer *w, float v);
void prts_w_f64(prts_writer *w, double v);
/* u32 length followed by the bytes (no terminator). NULL writes an empty string. */
void prts_w_str(prts_writer *w, const char *s);
void prts_writer_free(prts_writer *w);

/* Reader over a byte range. Reading past the end sets `failed` and returns zeros, so a
 * caller can read a whole record and check once. */
typedef struct prts_reader {
    const unsigned char *data;
    size_t len, pos;
    bool failed;
} prts_reader;

void prts_reader_init(prts_reader *r, const void *data, size_t size);
bool prts_r_bytes(prts_reader *r, void *out, size_t size);
uint8_t prts_r_u8(prts_reader *r);
uint32_t prts_r_u32(prts_reader *r);
uint64_t prts_r_u64(prts_reader *r);
float prts_r_f32(prts_reader *r);
double prts_r_f64(prts_reader *r);
/* Returns a NUL-terminated copy the caller frees, or NULL on failure. Strings longer than
 * max_len fail. */
char *prts_r_str(prts_reader *r, size_t max_len);

#endif
