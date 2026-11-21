/* 1stPRTS engine: virtual file system with mod mounts. MIT License. */
#ifndef PRTS_VFS_H
#define PRTS_VFS_H

#include "prts_base.h"

typedef enum prts_vfs_result {
    PRTS_VFS_OK = 0,
    PRTS_VFS_INVALID_PATH,
    PRTS_VFS_NOT_FOUND,
    PRTS_VFS_IO_ERROR
} prts_vfs_result;

typedef struct prts_vfs prts_vfs;

/* Virtual paths are relative, '/'-separated and case-sensitive. Rejected: empty paths,
 * absolute paths, "..", ".", backslashes, drive letters ("C:") and empty segments. */
bool prts_vfs_path_valid(const char *vpath);
const char *prts_vfs_result_name(prts_vfs_result r);

prts_vfs *prts_vfs_create(void);
void prts_vfs_destroy(prts_vfs *vfs);

/* Adds a read-only mount. Later mounts take priority over earlier ones. */
bool prts_vfs_mount(prts_vfs *vfs, const char *real_dir, const char *mount_id);
size_t prts_vfs_mount_count(const prts_vfs *vfs);
const char *prts_vfs_mount_id(const prts_vfs *vfs, size_t index);
const char *prts_vfs_mount_dir(const prts_vfs *vfs, size_t index);

/* Finds the highest-priority mount that has vpath (exact case). Returns its index or -1. */
int prts_vfs_find(prts_vfs *vfs, const char *vpath);
bool prts_vfs_exists(prts_vfs *vfs, const char *vpath);
prts_vfs_result prts_vfs_read(prts_vfs *vfs, const char *vpath, char **out_data, size_t *out_size);
/* Reads vpath from one mount only (for example every pack's CREDITS.md). */
prts_vfs_result prts_vfs_read_mount(prts_vfs *vfs, size_t mount, const char *vpath, char **out_data,
                                    size_t *out_size);

/* Merged, sorted, de-duplicated listing of a virtual directory ("" for the root). */
typedef void (*prts_vfs_list_fn)(const char *name, bool is_dir, void *user);
prts_vfs_result prts_vfs_list(prts_vfs *vfs, const char *vdir, prts_vfs_list_fn fn, void *user);

/* User data area (settings, saves, logs): the only writable place. */
void prts_vfs_set_user_dir(prts_vfs *vfs, const char *real_dir);
const char *prts_vfs_user_dir(const prts_vfs *vfs);
prts_vfs_result prts_vfs_user_write(prts_vfs *vfs, const char *vpath, const void *data, size_t size);
prts_vfs_result prts_vfs_user_read(prts_vfs *vfs, const char *vpath, char **out_data, size_t *out_size);

#endif
