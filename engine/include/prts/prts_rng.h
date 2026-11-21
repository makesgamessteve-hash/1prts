/* 1stPRTS engine: deterministic random numbers (PCG32) and hashing. MIT License. */
#ifndef PRTS_RNG_H
#define PRTS_RNG_H

#include "prts_base.h"

typedef struct prts_rng {
    uint64_t state;
    uint64_t inc;
} prts_rng;

void prts_rng_seed(prts_rng *r, uint64_t seed, uint64_t stream);
uint32_t prts_rng_next(prts_rng *r);
/* Uniform in [0, 1). */
double prts_rng_next_double(prts_rng *r);
/* Uniform integer in [lo, hi] (inclusive). Returns lo if hi < lo. */
int64_t prts_rng_range(prts_rng *r, int64_t lo, int64_t hi);

/* 64-bit FNV-1a hashing, usable incrementally. */
typedef struct prts_hasher { uint64_t h; } prts_hasher;

void prts_hasher_init(prts_hasher *h);
void prts_hasher_bytes(prts_hasher *h, const void *data, size_t size);
void prts_hasher_u64(prts_hasher *h, uint64_t v);
void prts_hasher_f64(prts_hasher *h, double v);
uint64_t prts_hash_bytes(const void *data, size_t size);
uint64_t prts_hash_str(const char *s);

#endif
