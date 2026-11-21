/* 1stPRTS engine: fixed-size object pool. MIT License. */
#include <string.h>

#include "prts/prts_arena.h"

#define POOL_DEFAULT_ITEMS 64u

static size_t round_up(size_t n) { return (n + PRTS_ARENA_ALIGN - 1u) & ~(size_t)(PRTS_ARENA_ALIGN - 1u); }

void prts_pool_init(prts_pool *p, size_t item_size, size_t per_block)
{
    memset(p, 0, sizeof(*p));
    /* every item must be able to hold the free-list link */
    p->item_size = round_up(item_size < sizeof(void *) ? sizeof(void *) : item_size);
    p->per_block = per_block ? per_block : POOL_DEFAULT_ITEMS;
}

static void grow(prts_pool *p)
{
    /* the block header (the link to the next block) takes one aligned slot */
    size_t header = round_up(sizeof(void *));
    unsigned char *block = prts_malloc(header + p->item_size * p->per_block);
    *(void **)block = p->blocks;
    p->blocks = block;
    /* push the items so that the first item of the block is handed out first */
    for (size_t i = p->per_block; i-- > 0;) {
        void *item = block + header + i * p->item_size;
        *(void **)item = p->free_list;
        p->free_list = item;
    }
    p->capacity += p->per_block;
}

void *prts_pool_alloc(prts_pool *p)
{
    if (!p->free_list) grow(p);
    void *item = p->free_list;
    p->free_list = *(void **)item;
    memset(item, 0, p->item_size);
    p->live++;
    return item;
}

void prts_pool_release(prts_pool *p, void *item)
{
    if (!item) return;
    *(void **)item = p->free_list;
    p->free_list = item;
    p->live--;
}

void prts_pool_free(prts_pool *p)
{
    while (p->blocks) {
        void *next = *(void **)p->blocks;
        prts_free(p->blocks);
        p->blocks = next;
    }
    p->free_list = NULL;
    p->live = p->capacity = 0;
}
