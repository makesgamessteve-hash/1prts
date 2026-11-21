/* 1stPRTS engine: arena allocator and insertion-ordered string hash map. MIT License. */
#ifndef PRTS_ARENA_H
#define PRTS_ARENA_H

#include "prts_base.h"

/* --- arena --------------------------------------------------------------------------
 * Bump allocator over a list of blocks. Allocations are 16-byte aligned and zeroed. A
 * request larger than the block size gets a block of its own. Memory is returned all at
 * once with reset (keeps one block for reuse) or free. */

#define PRTS_ARENA_ALIGN 16u

typedef struct prts_arena_block prts_arena_block;

typedef struct prts_arena {
    prts_arena_block *head; /* newest block */
    size_t block_size;
    size_t used;   /* bytes handed out since the last reset */
    size_t blocks; /* number of blocks held */
} prts_arena;

/* block_size 0 picks a default (64 KiB). */
void prts_arena_init(prts_arena *a, size_t block_size);
void *prts_arena_alloc(prts_arena *a, size_t size);
char *prts_arena_strdup(prts_arena *a, const char *s);
void prts_arena_reset(prts_arena *a);
void prts_arena_free(prts_arena *a);

/* --- pool --------------------------------------------------------------------------
 * Fixed-size objects from blocks of `per_block` items, recycled through a free list. Freed
 * items are reused newest first, so allocation order is deterministic. Items are 16-byte
 * aligned and zeroed when handed out. */

typedef struct prts_pool {
    size_t item_size; /* rounded up to PRTS_ARENA_ALIGN */
    size_t per_block;
    void *blocks;     /* linked list of blocks (first word links to the next) */
    void *free_list;  /* linked list of free items */
    size_t live;      /* items handed out and not yet freed */
    size_t capacity;  /* items in all blocks */
} prts_pool;

/* per_block 0 picks a default (64 items). */
void prts_pool_init(prts_pool *p, size_t item_size, size_t per_block);
void *prts_pool_alloc(prts_pool *p);
/* Returns an item to the pool (NULL is ignored). */
void prts_pool_release(prts_pool *p, void *item);
void prts_pool_free(prts_pool *p);

/* --- string hash map ----------------------------------------------------------------
 * Maps NUL-terminated string keys (copied) to pointer values. Iteration follows insertion
 * order, never hash order, so it is deterministic. Setting an existing key keeps its
 * position; removing and re-adding a key moves it to the end. */

typedef struct prts_map_entry {
    char *key; /* NULL: removed */
    void *value;
    uint64_t hash;
} prts_map_entry;

typedef struct prts_map {
    prts_map_entry *entries; /* insertion order, with holes for removed keys */
    size_t len, cap;
    uint32_t *slots; /* open addressing: 0 empty, PRTS_MAP_TOMB removed, else entry index + 1 */
    size_t slot_count;
    size_t count; /* live keys */
    size_t tombs;
} prts_map;

void prts_map_init(prts_map *m);
void prts_map_free(prts_map *m);
void prts_map_clear(prts_map *m);
/* Returns true if the key was new. */
bool prts_map_set(prts_map *m, const char *key, void *value);
void *prts_map_get(const prts_map *m, const char *key);
bool prts_map_has(const prts_map *m, const char *key);
/* Returns true if the key was present. */
bool prts_map_remove(prts_map *m, const char *key);
size_t prts_map_count(const prts_map *m);
/* Iteration: size_t it = 0; while (prts_map_next(m, &it, &key, &value)) { ... }.
 * The map must not be changed while iterating. key or value may be NULL. */
bool prts_map_next(const prts_map *m, size_t *it, const char **key, void **value);

#endif
