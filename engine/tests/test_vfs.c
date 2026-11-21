/* Unit tests: virtual file system. MIT License. */
#include <string.h>

#include "prts/prts_fs.h"
#include "prts/prts_vfs.h"
#include "utest.h"

#define FIX PRTS_TEST_FIXTURES "/vfs"

UTEST(vfs, path_rules)
{
    ASSERT_TRUE(prts_vfs_path_valid("models/box.glb"));
    ASSERT_TRUE(prts_vfs_path_valid("a"));
    ASSERT_TRUE(prts_vfs_path_valid("a/.hidden"));
    /* Spec: Path escape attempt */
    ASSERT_FALSE(prts_vfs_path_valid("../../etc/passwd"));
    ASSERT_FALSE(prts_vfs_path_valid("C:/Windows/win.ini"));
    ASSERT_FALSE(prts_vfs_path_valid("/etc/passwd"));
    ASSERT_FALSE(prts_vfs_path_valid("a\\b"));
    ASSERT_FALSE(prts_vfs_path_valid("a//b"));
    ASSERT_FALSE(prts_vfs_path_valid("a/./b"));
    ASSERT_FALSE(prts_vfs_path_valid("a/"));
    ASSERT_FALSE(prts_vfs_path_valid(""));
    ASSERT_FALSE(prts_vfs_path_valid(NULL));
    ASSERT_STREQ("ok", prts_vfs_result_name(PRTS_VFS_OK));
    ASSERT_STREQ("invalid path", prts_vfs_result_name(PRTS_VFS_INVALID_PATH));
    ASSERT_STREQ("not found", prts_vfs_result_name(PRTS_VFS_NOT_FOUND));
    ASSERT_STREQ("i/o error", prts_vfs_result_name(PRTS_VFS_IO_ERROR));
}

UTEST(vfs, mod_overrides_and_case)
{
    prts_vfs *v = prts_vfs_create();
    ASSERT_TRUE(prts_vfs_mount(v, FIX "/base", "base"));
    ASSERT_TRUE(prts_vfs_mount(v, FIX "/mod", "mod"));
    ASSERT_FALSE(prts_vfs_mount(v, FIX "/missing", "missing"));
    ASSERT_EQ((size_t)2, prts_vfs_mount_count(v));
    ASSERT_STREQ("mod", prts_vfs_mount_id(v, 1));
    ASSERT_TRUE(strstr(prts_vfs_mount_dir(v, 0), "base") != NULL);
    ASSERT_TRUE(prts_vfs_mount_id(v, 5) == NULL);
    ASSERT_TRUE(prts_vfs_mount_dir(v, 5) == NULL);

    /* Spec: Mod overrides a base file */
    char *data = NULL;
    size_t size = 0;
    ASSERT_EQ(PRTS_VFS_OK, prts_vfs_read(v, "sounds/door_open.wav", &data, &size));
    ASSERT_STREQ("mod door\n", data);
    ASSERT_EQ((size_t)9, size);
    prts_free(data);
    ASSERT_EQ(1, prts_vfs_find(v, "sounds/door_open.wav"));
    ASSERT_EQ(0, prts_vfs_find(v, "models/box.glb"));

    /* Spec: Same behavior across platforms (case-sensitive everywhere) */
    ASSERT_FALSE(prts_vfs_exists(v, "Models/Box.glb"));
    ASSERT_FALSE(prts_vfs_exists(v, "models/Box.glb"));
    ASSERT_TRUE(prts_vfs_exists(v, "models/box.glb"));
    ASSERT_FALSE(prts_vfs_exists(v, "models"));
    ASSERT_EQ(PRTS_VFS_NOT_FOUND, prts_vfs_read(v, "models/none.glb", &data, NULL));
    ASSERT_EQ(PRTS_VFS_INVALID_PATH, prts_vfs_read(v, "../x", &data, &size));
    ASSERT_TRUE(data == NULL);
    prts_vfs_destroy(v);
    prts_vfs_destroy(NULL);
}

typedef struct names {
    char buf[512];
    int dirs;
} names;

static void collect(const char *name, bool is_dir, void *user)
{
    names *n = user;
    strcat(n->buf, name);
    strcat(n->buf, ";");
    if (is_dir) n->dirs++;
}

UTEST(vfs, merged_listing)
{
    prts_vfs *v = prts_vfs_create();
    prts_vfs_mount(v, FIX "/base", "base");
    prts_vfs_mount(v, FIX "/mod", "mod");
    /* Spec: Mod adds a new file */
    names n = {{0}, 0};
    ASSERT_EQ(PRTS_VFS_OK, prts_vfs_list(v, "scripts/defs/props", collect, &n));
    ASSERT_STREQ("a_barrel.lua;new_crate.lua;", n.buf);
    names root = {{0}, 0};
    ASSERT_EQ(PRTS_VFS_OK, prts_vfs_list(v, "", collect, &root));
    ASSERT_STREQ("models;scripts;sounds;", root.buf);
    ASSERT_EQ(3, root.dirs);
    names none = {{0}, 0};
    ASSERT_EQ(PRTS_VFS_NOT_FOUND, prts_vfs_list(v, "nothing", collect, &none));
    ASSERT_EQ(PRTS_VFS_INVALID_PATH, prts_vfs_list(v, "../", collect, &none));
    names rootnull = {{0}, 0};
    ASSERT_EQ(PRTS_VFS_OK, prts_vfs_list(v, NULL, collect, &rootnull));
    prts_vfs_destroy(v);
}

UTEST(vfs, user_area)
{
    const char *dir = PRTS_TEST_TMP "/vfs-user";
    prts_fs_remove_tree(dir);
    prts_vfs *v = prts_vfs_create();
    char *data = NULL;
    size_t size = 0;
    ASSERT_EQ(PRTS_VFS_IO_ERROR, prts_vfs_user_write(v, "config.lua", "x", 1));
    ASSERT_EQ(PRTS_VFS_NOT_FOUND, prts_vfs_user_read(v, "config.lua", &data, &size));
    prts_vfs_set_user_dir(v, dir);
    ASSERT_STREQ(dir, prts_vfs_user_dir(v));
    /* Spec: Settings persist (the storage half) */
    ASSERT_EQ(PRTS_VFS_OK, prts_vfs_user_write(v, "saves/slot1.sav", "keep", 4));
    ASSERT_EQ(PRTS_VFS_OK, prts_vfs_user_read(v, "saves/slot1.sav", &data, &size));
    ASSERT_STREQ("keep", data);
    prts_free(data);
    ASSERT_EQ(PRTS_VFS_NOT_FOUND, prts_vfs_user_read(v, "saves/none.sav", &data, &size));
    ASSERT_EQ(PRTS_VFS_INVALID_PATH, prts_vfs_user_write(v, "../escape", "x", 1));
    ASSERT_EQ(PRTS_VFS_INVALID_PATH, prts_vfs_user_read(v, "/abs", &data, &size));
    ASSERT_EQ(PRTS_VFS_IO_ERROR, prts_vfs_user_write(v, "saves", "x", 1)); /* a directory */
    ASSERT_EQ(PRTS_VFS_NOT_FOUND, prts_vfs_user_read(v, "saves", &data, &size));
    prts_vfs_set_user_dir(v, NULL);
    ASSERT_TRUE(prts_vfs_user_dir(v) == NULL);
    prts_vfs_destroy(v);
}

UTEST(fs, helpers)
{
    char *base = prts_fs_base_dir();
    ASSERT_TRUE(prts_fs_is_dir(base));
    prts_free(base);
    char *user = prts_fs_user_dir();
    ASSERT_TRUE(user != NULL);
    prts_free(user);
    ASSERT_FALSE(prts_fs_enumerate(FIX "/missing", NULL, NULL));
    char *data = NULL;
    ASSERT_FALSE(prts_fs_read_file(FIX "/missing.txt", &data, NULL));
    ASSERT_TRUE(data == NULL);
    const char *tmp = PRTS_TEST_TMP "/fs/a/b/c.txt";
    ASSERT_TRUE(prts_fs_write_file(tmp, "c", 1));
    ASSERT_TRUE(prts_fs_is_file(tmp));
    ASSERT_TRUE(prts_fs_write_file("rootfile_test.txt", "r", 1));
    ASSERT_TRUE(prts_fs_remove_tree("rootfile_test.txt"));
    ASSERT_TRUE(prts_fs_remove_tree(PRTS_TEST_TMP "/fs"));
    ASSERT_FALSE(prts_fs_is_dir(PRTS_TEST_TMP "/fs"));
    ASSERT_FALSE(prts_fs_write_file("/proc/1stprts-denied/x/y.txt", "x", 1));
}
