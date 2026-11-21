/* Unit tests: core module. MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_base.h"
#include "prts/prts_log.h"
#include "prts/prts_math.h"
#include "prts/prts_rng.h"
#include "utest.h"

UTEST(core, array_push_grow_and_access)
{
    prts_array a;
    prts_array_init(&a, sizeof(int));
    for (int i = 0; i < 100; i++) prts_array_push(&a, &i);
    prts_array_push(&a, NULL);
    ASSERT_EQ((size_t)101, a.len);
    ASSERT_EQ(42, *(int *)prts_array_at(&a, 42));
    ASSERT_EQ(0, *(int *)prts_array_at(&a, 100));
    ASSERT_TRUE(prts_array_at(&a, 101) == NULL);
    prts_array_clear(&a);
    ASSERT_EQ((size_t)0, a.len);
    prts_array_free(&a);
    ASSERT_TRUE(a.data == NULL);
    ASSERT_EQ(sizeof(int), a.elem);
}

UTEST(core, string_builder)
{
    prts_str s = {0};
    ASSERT_STREQ("", prts_str_cstr(&s));
    prts_str_append(&s, "Aye, ");
    prts_str_appendf(&s, "my %s %d", "lord", 7);
    prts_str_append_n(&s, "!!!", 1);
    ASSERT_STREQ("Aye, my lord 7!", prts_str_cstr(&s));
    for (int i = 0; i < 200; i++) prts_str_append(&s, "x");
    ASSERT_EQ((size_t)215, s.len);
    prts_str_appendf(&s, "%s", "");
    ASSERT_EQ((size_t)215, s.len);
    prts_str_free(&s);
    ASSERT_TRUE(s.data == NULL);
}

UTEST(core, string_helpers)
{
    char *d = prts_strdup("keep");
    ASSERT_STREQ("keep", d);
    prts_free(d);
    char *n = prts_strndup("keep tower", 4);
    ASSERT_STREQ("keep", n);
    prts_free(n);
    char *n2 = prts_strndup("ab", 10);
    ASSERT_STREQ("ab", n2);
    prts_free(n2);
    ASSERT_TRUE(prts_str_starts_with("scripts/main.lua", "scripts/"));
    ASSERT_FALSE(prts_str_starts_with("main.lua", "scripts/"));
    ASSERT_TRUE(prts_str_ends_with("main.lua", ".lua"));
    ASSERT_FALSE(prts_str_ends_with("a", ".lua"));
    char *j = prts_path_join("data/", "/base");
    ASSERT_STREQ("data/base", j);
    prts_free(j);
    char *j2 = prts_path_join("", "base");
    ASSERT_STREQ("base", j2);
    prts_free(j2);
    char *j3 = prts_path_join("", "");
    ASSERT_STREQ("", j3);
    prts_free(j3);
    void *z = prts_calloc(0, 0);
    void *m = prts_malloc(0);
    m = prts_realloc(m, 16);
    prts_free(z);
    prts_free(m);
}

static int g_sink_calls;
static prts_log_level g_last_level;
static char g_last_msg[256];

static void test_sink(prts_log_level level, const char *message, void *user)
{
    (void)user;
    g_sink_calls++;
    g_last_level = level;
    strncpy(g_last_msg, message, sizeof(g_last_msg) - 1);
}

UTEST(core, log_sink_levels_and_error_count)
{
    prts_log_set_sink(test_sink, NULL);
    prts_log_reset_error_count();
    g_sink_calls = 0;
    prts_log_set_min_level(PRTS_LOG_WARN);
    prts_log_info("hidden %d", 1);
    ASSERT_EQ(0, g_sink_calls);
    prts_log_error("broke %s", "it");
    ASSERT_EQ(1, g_sink_calls);
    ASSERT_EQ(PRTS_LOG_ERROR, g_last_level);
    ASSERT_STREQ("broke it", g_last_msg);
    ASSERT_EQ((size_t)1, prts_log_error_count());
    prts_log_set_min_level(PRTS_LOG_DEBUG);
    prts_log_debug("dbg");
    prts_log_warn("w");
    ASSERT_EQ(3, g_sink_calls);
    prts_log_set_sink(NULL, NULL);
    prts_log_info("default sink works");
    prts_log_set_min_level(PRTS_LOG_INFO);
    ASSERT_STREQ("debug", prts_log_level_name(PRTS_LOG_DEBUG));
    ASSERT_STREQ("info", prts_log_level_name(PRTS_LOG_INFO));
    ASSERT_STREQ("warn", prts_log_level_name(PRTS_LOG_WARN));
    ASSERT_STREQ("error", prts_log_level_name(PRTS_LOG_ERROR));
    prts_log_reset_error_count();
}

UTEST(core, vector_math)
{
    prts_vec3 a = prts_v3(1, 2, 3), b = prts_v3(4, 5, 6);
    prts_vec3 c = prts_v3_add(a, b);
    ASSERT_EQ(5.0f, c.x);
    c = prts_v3_sub(b, a);
    ASSERT_EQ(3.0f, c.z);
    ASSERT_EQ(32.0f, prts_v3_dot(a, b));
    prts_vec3 x = prts_v3_cross(prts_v3(1, 0, 0), prts_v3(0, 1, 0));
    ASSERT_EQ(1.0f, x.z);
    ASSERT_NEAR(1.0f, prts_v3_length(prts_v3_normalize(b)), 1e-6f);
    ASSERT_EQ(0.0f, prts_v3_length(prts_v3_normalize(prts_v3(0, 0, 0))));
    prts_vec3 l = prts_v3_lerp(a, b, 0.5f);
    ASSERT_NEAR(2.5f, l.x, 1e-6f);
    prts_vec3 r = prts_v3_rotate_y(prts_v3(0, 0, 1), (float)(PRTS_PI / 2));
    ASSERT_NEAR(1.0f, r.x, 1e-6f);
    prts_vec3 f = prts_view_forward(0, 0);
    ASSERT_NEAR(-1.0f, f.z, 1e-6f);
    f = prts_view_forward(0, (float)(PRTS_PI / 2));
    ASSERT_NEAR(1.0f, f.y, 1e-6f);
    ASSERT_NEAR(0.5f, prts_wrap_angle(0.5f + (float)(4 * PRTS_PI)), 1e-4f);
    ASSERT_NEAR((float)(PRTS_PI - 0.5), prts_wrap_angle((float)(-PRTS_PI - 0.5)), 1e-4f);
    ASSERT_NEAR((float)(-PRTS_PI + 0.5), prts_wrap_angle((float)(PRTS_PI + 0.5)), 1e-4f);
    ASSERT_EQ(1.0f, prts_clampf(5, 0, 1));
    ASSERT_EQ(0.0f, prts_clampf(-5, 0, 1));
    ASSERT_EQ(0.5f, prts_clampf(0.5f, 0, 1));
}

UTEST(core, double_precision_far_from_origin)
{
    /* Spec: Large-world precision. 64 km from the origin, a wall 2 m away is still exact. */
    prts_dvec3 cam = prts_dv3(64000.123456, 12.5, -64000.987654);
    prts_dvec3 wall = prts_dv3_add(cam, prts_dv3(2.0, 0.25, -0.0001));
    prts_vec3 rel = prts_dv3_relative(wall, cam);
    ASSERT_NEAR(2.0f, rel.x, 1e-4f);
    ASSERT_NEAR(0.25f, rel.y, 1e-4f);
    ASSERT_NEAR(-0.0001f, rel.z, 1e-4f);
    prts_dvec3 d = prts_dv3_sub(wall, cam);
    ASSERT_NEAR(2.0, d.x, 1e-9);
    prts_dvec3 h = prts_dv3_lerp(cam, wall, 0.5);
    ASSERT_NEAR(64001.123456, h.x, 1e-9);
    prts_dvec3 fv = prts_dv3_from_v3(prts_v3(1, 2, 3));
    ASSERT_EQ(3.0, fv.z);
}

UTEST(core, matrices)
{
    prts_mat4 id = prts_mat4_identity();
    prts_vec3 p = prts_mat4_mul_point(prts_mat4_translate(prts_v3(1, 2, 3)), prts_v3(1, 1, 1));
    ASSERT_EQ(2.0f, p.x);
    ASSERT_EQ(4.0f, p.z);
    prts_mat4 m = prts_mat4_mul(id, prts_mat4_rotate_y((float)(PRTS_PI / 2)));
    p = prts_mat4_mul_point(m, prts_v3(0, 0, 1));
    ASSERT_NEAR(1.0f, p.x, 1e-6f);
    p = prts_mat4_mul_point(prts_mat4_rotate_x((float)(PRTS_PI / 2)), prts_v3(0, 1, 0));
    ASSERT_NEAR(1.0f, p.z, 1e-6f);
    /* The view matrix maps the forward direction to -Z. */
    float yaw = 0.7f, pitch = -0.3f;
    prts_vec3 fwd = prts_view_forward(yaw, pitch);
    p = prts_mat4_mul_point(prts_mat4_view_yaw_pitch(yaw, pitch), fwd);
    ASSERT_NEAR(0.0f, p.x, 1e-5f);
    ASSERT_NEAR(0.0f, p.y, 1e-5f);
    ASSERT_NEAR(-1.0f, p.z, 1e-5f);
    /* A point on the near plane maps to NDC z = -1. */
    prts_mat4 proj = prts_mat4_perspective((float)(PRTS_PI / 2), 1.0f, 0.1f, 100.0f);
    p = prts_mat4_mul_point(proj, prts_v3(0, 0, -0.1f));
    ASSERT_NEAR(-1.0f, p.z, 1e-4f);
}

UTEST(core, rng_deterministic_and_ranges)
{
    prts_rng a, b;
    prts_rng_seed(&a, 1234, 1);
    prts_rng_seed(&b, 1234, 1);
    for (int i = 0; i < 100; i++) ASSERT_EQ(prts_rng_next(&a), prts_rng_next(&b));
    prts_rng_seed(&b, 1235, 1);
    ASSERT_NE(prts_rng_next(&a), prts_rng_next(&b));
    for (int i = 0; i < 1000; i++) {
        double d = prts_rng_next_double(&a);
        ASSERT_TRUE(d >= 0.0 && d < 1.0);
        int64_t r = prts_rng_range(&a, -3, 3);
        ASSERT_TRUE(r >= -3 && r <= 3);
    }
    ASSERT_EQ((int64_t)7, prts_rng_range(&a, 7, 2));
}

UTEST(core, hashing)
{
    /* FNV-1a 64 reference vectors */
    ASSERT_EQ(0xcbf29ce484222325ULL, prts_hash_bytes("", 0));
    ASSERT_EQ(0xaf63dc4c8601ec8cULL, prts_hash_str("a"));
    prts_hasher h1, h2;
    prts_hasher_init(&h1);
    prts_hasher_init(&h2);
    prts_hasher_f64(&h1, 0.0);
    prts_hasher_f64(&h2, -0.0);
    ASSERT_EQ(h1.h, h2.h);
    prts_hasher_u64(&h1, 1);
    ASSERT_NE(h1.h, h2.h);
}
