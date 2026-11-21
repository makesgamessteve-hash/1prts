/* Unit tests: arena allocator, insertion-ordered hash map, binary writer/reader. MIT License. */
#include <stdio.h>
#include <string.h>

#include "prts/prts_arena.h"
#include "prts/prts_serial.h"
#include "utest.h"

UTEST(arena, alloc_alignment_growth_reset_free)
{
    prts_arena a;
    prts_arena_init(&a, 64);
    ASSERT_EQ((size_t)64, a.block_size);
    unsigned char *p1 = prts_arena_alloc(&a, 3);
    unsigned char *p2 = prts_arena_alloc(&a, 0);
    ASSERT_EQ((size_t)0, (size_t)((uintptr_t)p1 % PRTS_ARENA_ALIGN));
    ASSERT_EQ((size_t)0, (size_t)((uintptr_t)p2 % PRTS_ARENA_ALIGN));
    ASSERT_TRUE(p2 >= p1 + PRTS_ARENA_ALIGN);
    ASSERT_EQ((size_t)1, a.blocks);
    /* the block is full after four 16-byte pieces; the next one starts a new block */
    prts_arena_alloc(&a, 16);
    prts_arena_alloc(&a, 16);
    ASSERT_EQ((size_t)1, a.blocks);
    unsigned char *p5 = prts_arena_alloc(&a, 1);
    ASSERT_EQ((size_t)2, a.blocks);
    ASSERT_EQ(0, p5[0]); /* zeroed */
    /* a large request gets its own block behind the current one, which keeps serving */
    unsigned char *big = prts_arena_alloc(&a, 1000);
    memset(big, 7, 1000);
    ASSERT_EQ((size_t)3, a.blocks);
    unsigned char *p6 = prts_arena_alloc(&a, 1);
    ASSERT_EQ((size_t)3, a.blocks);
    ASSERT_EQ(p5 + PRTS_ARENA_ALIGN, p6);
    char *s = prts_arena_strdup(&a, "war table");
    ASSERT_STREQ("war table", s);
    ASSERT_EQ((size_t)(5 * 16 + 1008 + 16 + 16), a.used);
    prts_arena_reset(&a);
    ASSERT_EQ((size_t)1, a.blocks);
    ASSERT_EQ((size_t)0, a.used);
    unsigned char *again = prts_arena_alloc(&a, 8);
    ASSERT_TRUE(again != NULL);
    ASSERT_EQ((size_t)1, a.blocks);
    /* a large request into an empty or full arena becomes the head block, which reset frees */
    prts_arena_free(&a);
    ASSERT_TRUE(a.head == NULL);
    ASSERT_EQ((size_t)64, a.block_size);
    prts_arena_alloc(&a, 500);
    ASSERT_EQ((size_t)1, a.blocks);
    prts_arena_alloc(&a, 500);
    ASSERT_EQ((size_t)2, a.blocks);
    prts_arena_reset(&a);
    ASSERT_EQ((size_t)0, a.blocks);
    ASSERT_TRUE(a.head == NULL);
    prts_arena_free(&a);
    prts_arena_init(&a, 0);
    ASSERT_EQ((size_t)65536, a.block_size);
    prts_arena_free(&a);
}

UTEST(pool, alloc_reuse_alignment_and_growth)
{
    prts_pool p;
    prts_pool_init(&p, 3, 4); /* tiny items still hold the free-list link */
    ASSERT_EQ((size_t)PRTS_ARENA_ALIGN, p.item_size);
    void *items[9];
    for (int i = 0; i < 9; i++) {
        items[i] = prts_pool_alloc(&p);
        ASSERT_EQ((uintptr_t)0, (uintptr_t)items[i] % PRTS_ARENA_ALIGN);
        memset(items[i], 0xab, 3);
    }
    ASSERT_EQ((size_t)9, p.live);
    ASSERT_EQ((size_t)12, p.capacity); /* three blocks of four */
    /* consecutive items of a block are handed out in address order */
    ASSERT_EQ((uintptr_t)items[0] + p.item_size, (uintptr_t)items[1]);
    /* the newest freed item is reused first, and comes back zeroed */
    prts_pool_release(&p, items[2]);
    prts_pool_release(&p, items[5]);
    prts_pool_release(&p, NULL);
    ASSERT_EQ((size_t)7, p.live);
    unsigned char *again = prts_pool_alloc(&p);
    ASSERT_EQ((void *)items[5], (void *)again);
    ASSERT_EQ(0, again[0]);
    ASSERT_EQ((void *)items[2], prts_pool_alloc(&p));
    prts_pool_free(&p);
    ASSERT_EQ((size_t)0, p.capacity);
    prts_pool_init(&p, 100, 0);
    ASSERT_EQ((size_t)112, p.item_size);
    ASSERT_TRUE(prts_pool_alloc(&p) != NULL);
    ASSERT_EQ((size_t)64, p.capacity);
    prts_pool_free(&p);
}

UTEST(map, set_get_update_remove)
{
    prts_map m;
    prts_map_init(&m);
    ASSERT_TRUE(prts_map_get(&m, "x") == NULL);
    ASSERT_FALSE(prts_map_remove(&m, "x"));
    int a = 1, b = 2, c = 3;
    ASSERT_TRUE(prts_map_set(&m, "keep", &a));
    ASSERT_TRUE(prts_map_set(&m, "lift", &b));
    ASSERT_FALSE(prts_map_set(&m, "keep", &c)); /* update keeps the position */
    ASSERT_EQ((size_t)2, prts_map_count(&m));
    ASSERT_TRUE(prts_map_get(&m, "keep") == &c);
    ASSERT_TRUE(prts_map_has(&m, "lift"));
    ASSERT_FALSE(prts_map_has(&m, "door"));
    ASSERT_TRUE(prts_map_remove(&m, "keep"));
    ASSERT_FALSE(prts_map_remove(&m, "keep"));
    ASSERT_FALSE(prts_map_has(&m, "keep"));
    ASSERT_EQ((size_t)1, prts_map_count(&m));
    prts_map_clear(&m);
    ASSERT_EQ((size_t)0, prts_map_count(&m));
    ASSERT_FALSE(prts_map_has(&m, "lift"));
    ASSERT_TRUE(prts_map_set(&m, "lift", NULL));
    ASSERT_TRUE(prts_map_has(&m, "lift"));
    prts_map_free(&m);
    prts_map_free(&m);
}

UTEST(map, iteration_follows_insertion_order)
{
    /* deterministic: the order depends only on the sequence of inserts and removals */
    prts_map m;
    prts_map_init(&m);
    char key[32];
    static int values[1000];
    for (int i = 0; i < 1000; i++) {
        values[i] = i;
        snprintf(key, sizeof(key), "k%d", (i * 7919) % 1000);
        prts_map_set(&m, key, &values[i]);
    }
    ASSERT_EQ((size_t)1000, prts_map_count(&m));
    /* remove every third key, then re-add one: it moves to the end */
    for (int i = 0; i < 1000; i += 3) {
        snprintf(key, sizeof(key), "k%d", (i * 7919) % 1000);
        ASSERT_TRUE(prts_map_remove(&m, key));
    }
    snprintf(key, sizeof(key), "k%d", 0);
    prts_map_set(&m, key, &values[0]);
    size_t it = 0, n = 0;
    const char *k;
    void *v;
    int last = -1;
    while (prts_map_next(&m, &it, &k, &v)) {
        int i = *(int *)v;
        if (n + 1 < prts_map_count(&m)) {
            ASSERT_TRUE(i > last);
            ASSERT_NE(0, i % 3);
            last = i;
        } else {
            ASSERT_STREQ("k0", k);
        }
        snprintf(key, sizeof(key), "k%d", (i * 7919) % 1000);
        ASSERT_STREQ(key, k);
        n++;
    }
    ASSERT_EQ(prts_map_count(&m), n);
    it = 0;
    ASSERT_TRUE(prts_map_next(&m, &it, NULL, NULL));
    /* many removals and inserts in a row reuse the table without growing without bound */
    for (int round = 0; round < 50; round++) {
        prts_map_set(&m, "churn", &values[1]);
        prts_map_remove(&m, "churn");
    }
    ASSERT_FALSE(prts_map_has(&m, "churn"));
    ASSERT_TRUE(m.slot_count <= 4096);
    prts_map_free(&m);
}

UTEST(serial, round_trip_and_truncation)
{
    prts_writer w = {0};
    prts_w_u8(&w, 0xab);
    prts_w_u32(&w, 0x01020304u);
    prts_w_u64(&w, 0x1122334455667788ull);
    prts_w_f32(&w, -1.5f);
    prts_w_f64(&w, 0.1);
    prts_w_str(&w, "keep");
    prts_w_str(&w, NULL);
    for (int i = 0; i < 100; i++) prts_w_u64(&w, (uint64_t)i);
    ASSERT_EQ((size_t)(1 + 4 + 8 + 4 + 8 + 8 + 4 + 800), w.len);
    ASSERT_EQ(0x04, w.data[1]); /* little-endian on every platform */

    prts_reader r;
    prts_reader_init(&r, w.data, w.len);
    ASSERT_EQ(0xab, prts_r_u8(&r));
    ASSERT_EQ(0x01020304u, prts_r_u32(&r));
    ASSERT_EQ(0x1122334455667788ull, prts_r_u64(&r));
    ASSERT_EQ(-1.5f, prts_r_f32(&r));
    ASSERT_EQ(0.1, prts_r_f64(&r));
    char *s = prts_r_str(&r, 64);
    ASSERT_STREQ("keep", s);
    prts_free(s);
    s = prts_r_str(&r, 64);
    ASSERT_STREQ("", s);
    prts_free(s);
    for (int i = 0; i < 100; i++) ASSERT_EQ((uint64_t)i, prts_r_u64(&r));
    ASSERT_FALSE(r.failed);
    ASSERT_EQ(0u, prts_r_u32(&r)); /* past the end */
    ASSERT_TRUE(r.failed);
    ASSERT_EQ(0, prts_r_u8(&r));

    /* strings: too long for the limit, or longer than the data */
    prts_reader_init(&r, w.data + 25, w.len - 25);
    ASSERT_TRUE(prts_r_str(&r, 2) == NULL);
    ASSERT_TRUE(r.failed);
    unsigned char bad[6] = {200, 0, 0, 0, 'a', 'b'};
    prts_reader_init(&r, bad, sizeof(bad));
    ASSERT_TRUE(prts_r_str(&r, 1000) == NULL);
    prts_writer_free(&w);
    ASSERT_TRUE(w.data == NULL);
}
