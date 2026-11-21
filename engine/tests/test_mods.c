/* Unit tests: mod manifests, dependencies and load order. MIT License. */
#include <string.h>

#include "app/engine_internal.h"
#include "app/mods.h"
#include "prts/prts_log.h"
#include "utest.h"

UTEST(mods, versions_and_dependency_specs)
{
    uint32_t v[3];
    ASSERT_TRUE(prts_version_parse("1.2.3", v));
    ASSERT_EQ(3u, v[2]);
    ASSERT_TRUE(prts_version_parse("0.1", v));
    ASSERT_EQ(1u, v[1]);
    ASSERT_EQ(0u, v[2]);
    ASSERT_TRUE(prts_version_parse("7", v));
    ASSERT_EQ(7u, v[0]);
    const char *bad[] = {"", "x", "1.", "1..2", "1.2.3.4", "1.2a", "-1", "99999999999"};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(bad); i++) ASSERT_FALSE(prts_version_parse(bad[i], v));
    uint32_t a[3] = {0, 1, 0}, b[3] = {0, 1, 2}, c[3] = {1, 0, 0};
    ASSERT_EQ(-1, prts_version_compare(a, b));
    ASSERT_EQ(1, prts_version_compare(c, b));
    ASSERT_EQ(0, prts_version_compare(a, a));

    struct {
        const char *text;
        prts_version_op op;
        const char *id;
        bool at_010, at_020;
    } specs[] = {
        {"base", PRTS_VER_ANY, "base", true, true},       {" base >= 0.1 ", PRTS_VER_GE, "base", true, true},
        {"base>0.1", PRTS_VER_GT, "base", false, true},    {"my-mod == 0.1.0", PRTS_VER_EQ, "my-mod", true, false},
        {"base = 0.2", PRTS_VER_EQ, "base", false, true},  {"base <= 0.1", PRTS_VER_LE, "base", true, false},
        {"base < 0.2", PRTS_VER_LT, "base", true, false},
    };
    uint32_t v010[3] = {0, 1, 0}, v020[3] = {0, 2, 0};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(specs); i++) {
        prts_mod_dep d;
        ASSERT_TRUE(prts_mod_dep_parse(specs[i].text, &d));
        ASSERT_EQ((int)specs[i].op, (int)d.op);
        ASSERT_STREQ(specs[i].id, d.id);
        ASSERT_STREQ(specs[i].text, d.text);
        ASSERT_EQ(specs[i].at_010, prts_mod_dep_satisfied(&d, v010));
        ASSERT_EQ(specs[i].at_020, prts_mod_dep_satisfied(&d, v020));
        prts_free(d.id);
        prts_free(d.text);
    }
    const char *bad_specs[] = {"", ">= 1", "base >=", "base >>> 1", "base >= x", "base >= 1 extra", "ba$e",
                               "base >= 123456789012345678901234567890123"};
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(bad_specs); i++) {
        prts_mod_dep d;
        ASSERT_FALSE(prts_mod_dep_parse(bad_specs[i], &d));
        ASSERT_TRUE(d.id == NULL);
    }
}

static prts_str g_log;

static void capture(prts_log_level level, const char *message, void *user)
{
    PRTS_UNUSED(level);
    PRTS_UNUSED(user);
    prts_str_appendf(&g_log, "%s\n", message);
}

static prts_engine *boot_with_mods(const char **mods, int n)
{
    prts_engine_config cfg;
    prts_engine_config_default(&cfg);
    cfg.data_dir = PRTS_TEST_DATA;
    cfg.user_dir = PRTS_TEST_FIXTURES "/modsets/user";
    cfg.headless = true;
    for (int i = 0; i < n; i++) cfg.mods[cfg.mod_count++] = mods[i];
    char err[1024];
    prts_str_free(&g_log);
    prts_log_set_sink(capture, NULL);
    prts_log_set_min_level(PRTS_LOG_INFO);
    prts_engine *e = prts_engine_create(&cfg, err, sizeof(err));
    prts_log_set_sink(NULL, NULL);
    return e;
}

static bool logged(const char *text) { return strstr(prts_str_cstr(&g_log), text) != NULL; }

UTEST(mods, dependencies_and_load_order)
{
    /* given in a shuffled order: the load order must not depend on it */
    const char *mods[] = {"m_zeta",    "m_old",     "m_alpha",   "m_missing", "m_chain",  "m_beta",
                          "m_cycle_a", "m_core",    "m_cycle_b", "m_newbase", "m_badspec", "m_badlist",
                          "m_badafter", "m_notstr", "m_badver",  "m_wrongid"};
    prts_engine *e = boot_with_mods(mods, (int)PRTS_ARRAY_COUNT(mods));
    ASSERT_TRUE(e != NULL);
    const char *want[] = {"base", "m_beta", "m_core", "m_alpha", "m_zeta"};
    ASSERT_EQ(PRTS_ARRAY_COUNT(want), prts_vfs_mount_count(e->vfs));
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(want); i++) ASSERT_STREQ(want[i], prts_vfs_mount_id(e->vfs, i));
    ASSERT_STREQ("0.1.0", e->mod_versions[0]);
    ASSERT_STREQ("1.2.0", e->mod_versions[2]);
    ASSERT_TRUE(logged("mod load order: m_beta, m_core, m_alpha, m_zeta"));
    ASSERT_TRUE(logged("mod 'm_old' needs 'm_core >= 2.0', but 'm_core' 1.2.0 is installed"));
    ASSERT_TRUE(logged("mod 'm_newbase' needs 'base >= 9', but 'base' 0.1.0 is installed"));
    ASSERT_TRUE(logged("mod 'm_missing' needs 'm_nowhere', which is not enabled or not installed"));
    ASSERT_TRUE(logged("mod 'm_chain' needs 'm_missing', which is skipped"));
    ASSERT_TRUE(logged("mods 'm_cycle_a', 'm_cycle_b' depend on each other in a cycle"));
    ASSERT_TRUE(logged("mod 'm_badspec' has an invalid depends entry 'base >>> 1'"));
    ASSERT_TRUE(logged("mod 'm_badlist': 'load_after' in mod.lua must be a list of strings"));
    ASSERT_TRUE(logged("mod 'm_badafter' has an invalid load_after entry 'm_core >= 1'"));
    ASSERT_TRUE(logged("mod 'm_notstr' has an invalid depends entry '(not a string)'"));
    ASSERT_TRUE(logged("mod 'm_badver' has an invalid version"));
    ASSERT_TRUE(logged("mod folder 'm_wrongid' has a mod.lua with id 'someone_else'"));
    prts_engine_destroy(e);
}

UTEST(mods, repeats_are_ignored)
{
    const char *mods[] = {"m_beta", "m_beta", "base"};
    prts_engine *e = boot_with_mods(mods, 3);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ((size_t)2, prts_vfs_mount_count(e->vfs));
    ASSERT_STREQ("0.3", e->mod_versions[1]);
    ASSERT_TRUE(logged("mod 'm_beta' is enabled more than once"));
    ASSERT_TRUE(logged("mod 'base' is enabled more than once"));
    prts_engine_destroy(e);
    prts_str_free(&g_log);
}

UTEST(mods, resolve_without_mods)
{
    prts_mod base;
    prts_mod_init(&base, "base");
    size_t order[1];
    ASSERT_EQ((size_t)0, prts_mods_resolve(NULL, 0, &base, order));
    prts_mod_free(&base);
}
