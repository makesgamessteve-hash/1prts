/* 1stPRTS engine: PCG32 random numbers and FNV-1a hashing. MIT License. */
#include "prts/prts_rng.h"

#include <string.h>

void prts_rng_seed(prts_rng *r, uint64_t seed, uint64_t stream)
{
    r->state = 0u;
    r->inc = (stream << 1u) | 1u;
    (void)prts_rng_next(r);
    r->state += seed;
    (void)prts_rng_next(r);
}

uint32_t prts_rng_next(prts_rng *r)
{
    uint64_t old = r->state;
    r->state = old * 6364136223846793005ULL + r->inc;
    uint32_t xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
    uint32_t rot = (uint32_t)(old >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31u));
}

double prts_rng_next_double(prts_rng *r) { return prts_rng_next(r) / 4294967296.0; }

int64_t prts_rng_range(prts_rng *r, int64_t lo, int64_t hi)
{
    if (hi <= lo) return lo;
    uint64_t span = (uint64_t)(hi - lo) + 1u;
    return lo + (int64_t)(((uint64_t)prts_rng_next(r) * span) >> 32u);
}

void prts_hasher_init(prts_hasher *h) { h->h = 14695981039346656037ULL; }

void prts_hasher_bytes(prts_hasher *h, const void *data, size_t size)
{
    const unsigned char *p = data;
    for (size_t i = 0; i < size; i++) {
        h->h ^= p[i];
        h->h *= 1099511628211ULL;
    }
}

void prts_hasher_u64(prts_hasher *h, uint64_t v) { prts_hasher_bytes(h, &v, sizeof(v)); }

void prts_hasher_f64(prts_hasher *h, double v)
{
    uint64_t bits;
    if (v == 0.0) v = 0.0; /* -0.0 and 0.0 hash the same */
    memcpy(&bits, &v, sizeof(bits));
    prts_hasher_u64(h, bits);
}

uint64_t prts_hash_bytes(const void *data, size_t size)
{
    prts_hasher h;
    prts_hasher_init(&h);
    prts_hasher_bytes(&h, data, size);
    return h.h;
}

uint64_t prts_hash_str(const char *s) { return prts_hash_bytes(s, strlen(s)); }
