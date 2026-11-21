/* 1stPRTS engine: arena allocator and insertion-ordered string hash map. MIT License. */
#include "prts/prts_arena.h"

#include <string.h>

#include "prts/prts_rng.h"

/* --- arena ---------------------------------------------------------------------------- */

struct prts_arena_block {
    prts_arena_block *next; /* older block */
    size_t size;
    size_t used;
    /* followed by the block's memory, starting at an aligned offset */
};

#define BLOCK_HEADER (((sizeof(prts_arena_block) + PRTS_ARENA_ALIGN - 1u) / PRTS_ARENA_ALIGN) * PRTS_ARENA_ALIGN)

static unsigned char *block_mem(prts_arena_block *b) { return (unsigned char *)b + BLOCK_HEADER; }

static size_t align_up(size_t n) { return (n + PRTS_ARENA_ALIGN - 1u) & ~(size_t)(PRTS_ARENA_ALIGN - 1u); }

void prts_arena_init(prts_arena *a, size_t block_size)
{
    memset(a, 0, sizeof(*a));
    a->block_size = block_size ? align_up(block_size) : 65536u;
}

static prts_arena_block *new_block(prts_arena *a, size_t min_size)
{
    size_t size = min_size > a->block_size ? align_up(min_size) : a->block_size;
    prts_arena_block *b = prts_malloc(BLOCK_HEADER + size);
    b->size = size;
    b->used = 0;
    b->next = a->head;
    a->head = b;
    a->blocks++;
    return b;
}

void *prts_arena_alloc(prts_arena *a, size_t size)
{
    size_t need = align_up(size ? size : 1u);
    prts_arena_block *b = a->head;
    if (!b || b->size - b->used < need) {
        if (need > a->block_size && b && b->size - b->used >= PRTS_ARENA_ALIGN) {
            /* a big request gets its own block behind the current one, so the free space
             * left in the current block is still used by later small requests */
            prts_arena_block *big = prts_malloc(BLOCK_HEADER + need);
            big->size = big->used = need;
            big->next = b->next;
            b->next = big;
            a->blocks++;
            a->used += need;
            memset(block_mem(big), 0, need);
            return block_mem(big);
        }
        b = new_block(a, need);
    }
    void *p = block_mem(b) + b->used;
    b->used += need;
    a->used += need;
    memset(p, 0, need);
    return p;
}

char *prts_arena_strdup(prts_arena *a, const char *s)
{
    size_t n = strlen(s);
    char *out = prts_arena_alloc(a, n + 1u);
    memcpy(out, s, n);
    return out;
}

void prts_arena_reset(prts_arena *a)
{
    /* keep the newest block if it has the standard size; free the rest */
    prts_arena_block *keep = a->head && a->head->size == a->block_size ? a->head : NULL;
    prts_arena_block *b = keep ? keep->next : a->head;
    while (b) {
        prts_arena_block *next = b->next;
        prts_free(b);
        b = next;
    }
    a->head = keep;
    a->blocks = keep ? 1u : 0u;
    if (keep) {
        keep->next = NULL;
        keep->used = 0;
    }
    a->used = 0;
}

void prts_arena_free(prts_arena *a)
{
    size_t block_size = a->block_size;
    prts_arena_reset(a);
    prts_free(a->head);
    prts_arena_init(a, block_size);
}

/* --- string hash map ------------------------------------------------------------------- */

#define PRTS_MAP_TOMB 0xffffffffu

void prts_map_init(prts_map *m) { memset(m, 0, sizeof(*m)); }

void prts_map_clear(prts_map *m)
{
    for (size_t i = 0; i < m->len; i++) prts_free(m->entries[i].key);
    m->len = 0;
    m->count = 0;
    m->tombs = 0;
    if (m->slots) memset(m->slots, 0, m->slot_count * sizeof(uint32_t));
}

void prts_map_free(prts_map *m)
{
    prts_map_clear(m);
    prts_free(m->entries);
    prts_free(m->slots);
    prts_map_init(m);
}

static size_t find_slot(const prts_map *m, const char *key, uint64_t hash)
{
    size_t mask = m->slot_count - 1u;
    size_t i = (size_t)hash & mask;
    for (;;) {
        uint32_t s = m->slots[i];
        if (s == 0) return i;
        if (s != PRTS_MAP_TOMB) {
            const prts_map_entry *e = &m->entries[s - 1u];
            if (e->hash == hash && strcmp(e->key, key) == 0) return i;
        }
        i = (i + 1u) & mask;
    }
}

/* Rebuilds the slot table (and compacts the entry list) for at least `want` live keys. */
static void rebuild(prts_map *m, size_t want)
{
    size_t n = 0;
    for (size_t i = 0; i < m->len; i++) {
        if (m->entries[i].key) m->entries[n++] = m->entries[i];
    }
    m->len = n;
    size_t slots = 16;
    while (slots < want * 2u) slots *= 2u;
    prts_free(m->slots);
    m->slots = prts_calloc(slots, sizeof(uint32_t));
    m->slot_count = slots;
    m->tombs = 0;
    size_t mask = slots - 1u;
    for (size_t e = 0; e < n; e++) {
        size_t i = (size_t)m->entries[e].hash & mask;
        while (m->slots[i]) i = (i + 1u) & mask;
        m->slots[i] = (uint32_t)(e + 1u);
    }
}

bool prts_map_set(prts_map *m, const char *key, void *value)
{
    uint64_t hash = prts_hash_str(key);
    if (m->slot_count) {
        uint32_t s = m->slots[find_slot(m, key, hash)];
        if (s) {
            m->entries[s - 1u].value = value;
            return false;
        }
    }
    /* keep the table at most half full, counting removed slots */
    if ((m->count + m->tombs + 1u) * 2u > m->slot_count) rebuild(m, m->count + 1u);
    if (m->len == m->cap) {
        m->cap = m->cap ? m->cap * 2u : 8u;
        m->entries = prts_realloc(m->entries, m->cap * sizeof(prts_map_entry));
    }
    prts_map_entry *e = &m->entries[m->len];
    e->key = prts_strdup(key);
    e->value = value;
    e->hash = hash;
    m->slots[find_slot(m, key, hash)] = (uint32_t)(m->len + 1u);
    m->len++;
    m->count++;
    return true;
}

static const prts_map_entry *lookup(const prts_map *m, const char *key)
{
    if (!m->count) return NULL;
    uint32_t s = m->slots[find_slot(m, key, prts_hash_str(key))];
    return s ? &m->entries[s - 1u] : NULL;
}

void *prts_map_get(const prts_map *m, const char *key)
{
    const prts_map_entry *e = lookup(m, key);
    return e ? e->value : NULL;
}

bool prts_map_has(const prts_map *m, const char *key) { return lookup(m, key) != NULL; }

bool prts_map_remove(prts_map *m, const char *key)
{
    if (!m->count) return false;
    size_t slot = find_slot(m, key, prts_hash_str(key));
    uint32_t s = m->slots[slot];
    if (!s) return false;
    prts_map_entry *e = &m->entries[s - 1u];
    prts_free(e->key);
    e->key = NULL;
    e->value = NULL;
    m->slots[slot] = PRTS_MAP_TOMB;
    m->count--;
    m->tombs++;
    return true;
}

size_t prts_map_count(const prts_map *m) { return m->count; }

bool prts_map_next(const prts_map *m, size_t *it, const char **key, void **value)
{
    while (*it < m->len) {
        const prts_map_entry *e = &m->entries[(*it)++];
        if (!e->key) continue;
        if (key) *key = e->key;
        if (value) *value = e->value;
        return true;
    }
    return false;
}
