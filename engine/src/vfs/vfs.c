/* 1stPRTS engine: virtual file system with mod mounts. MIT License. */
#include "prts/prts_vfs.h"

#include <stdlib.h>
#include <string.h>

#include "prts/prts_fs.h"

typedef struct prts_mount {
    char *dir;
    char *id;
} prts_mount;

struct prts_vfs {
    prts_array mounts; /* prts_mount */
    char *user_dir;
};

bool prts_vfs_path_valid(const char *vpath)
{
    if (!vpath || !*vpath) return false;
    if (vpath[0] == '/') return false;
    if (strchr(vpath, '\\') || strchr(vpath, ':')) return false;
    const char *seg = vpath;
    for (;;) {
        const char *end = strchr(seg, '/');
        size_t len = end ? (size_t)(end - seg) : strlen(seg);
        if (len == 0) return false;
        if (len == 1 && seg[0] == '.') return false;
        if (len == 2 && seg[0] == '.' && seg[1] == '.') return false;
        if (!end) return true;
        seg = end + 1;
    }
}

const char *prts_vfs_result_name(prts_vfs_result r)
{
    switch (r) {
    case PRTS_VFS_OK: return "ok";
    case PRTS_VFS_INVALID_PATH: return "invalid path";
    case PRTS_VFS_NOT_FOUND: return "not found";
    default: return "i/o error";
    }
}

prts_vfs *prts_vfs_create(void)
{
    prts_vfs *vfs = prts_calloc(1, sizeof(*vfs));
    prts_array_init(&vfs->mounts, sizeof(prts_mount));
    return vfs;
}

void prts_vfs_destroy(prts_vfs *vfs)
{
    if (!vfs) return;
    for (size_t i = 0; i < vfs->mounts.len; i++) {
        prts_mount *m = prts_array_at(&vfs->mounts, i);
        prts_free(m->dir);
        prts_free(m->id);
    }
    prts_array_free(&vfs->mounts);
    prts_free(vfs->user_dir);
    prts_free(vfs);
}

bool prts_vfs_mount(prts_vfs *vfs, const char *real_dir, const char *mount_id)
{
    if (!prts_fs_is_dir(real_dir)) return false;
    prts_mount m = {prts_strdup(real_dir), prts_strdup(mount_id)};
    prts_array_push(&vfs->mounts, &m);
    return true;
}

size_t prts_vfs_mount_count(const prts_vfs *vfs) { return vfs->mounts.len; }

const char *prts_vfs_mount_id(const prts_vfs *vfs, size_t index)
{
    prts_mount *m = prts_array_at(&vfs->mounts, index);
    return m ? m->id : NULL;
}

const char *prts_vfs_mount_dir(const prts_vfs *vfs, size_t index)
{
    prts_mount *m = prts_array_at(&vfs->mounts, index);
    return m ? m->dir : NULL;
}

typedef struct name_search {
    const char *name;
    size_t len;
    bool found;
} name_search;

static void match_name(const char *name, void *user)
{
    name_search *s = user;
    if (strlen(name) == s->len && memcmp(name, s->name, s->len) == 0) s->found = true;
}

/* True if every segment of vpath exists under root with exactly this spelling. */
static bool exact_case(const char *root, const char *vpath)
{
    char *dir = prts_strdup(root);
    const char *seg = vpath;
    bool ok = true;
    while (ok && *seg) {
        const char *end = strchr(seg, '/');
        size_t len = end ? (size_t)(end - seg) : strlen(seg);
        name_search s = {seg, len, false};
        ok = prts_fs_enumerate(dir, match_name, &s) && s.found;
        if (ok) {
            char *part = prts_strndup(seg, len);
            char *next = prts_path_join(dir, part);
            prts_free(part);
            prts_free(dir);
            dir = next;
        }
        seg = end ? end + 1 : seg + len;
    }
    prts_free(dir);
    return ok;
}

static bool mount_has(const prts_mount *m, const char *vpath, bool want_dir)
{
    char *real = prts_path_join(m->dir, vpath);
    bool ok = want_dir ? prts_fs_is_dir(real) : prts_fs_is_file(real);
    prts_free(real);
    return ok && exact_case(m->dir, vpath);
}

int prts_vfs_find(prts_vfs *vfs, const char *vpath)
{
    if (!prts_vfs_path_valid(vpath)) return -1;
    for (size_t i = vfs->mounts.len; i-- > 0;) {
        if (mount_has(prts_array_at(&vfs->mounts, i), vpath, false)) return (int)i;
    }
    return -1;
}

bool prts_vfs_exists(prts_vfs *vfs, const char *vpath) { return prts_vfs_find(vfs, vpath) >= 0; }

prts_vfs_result prts_vfs_read(prts_vfs *vfs, const char *vpath, char **out_data, size_t *out_size)
{
    *out_data = NULL;
    if (out_size) *out_size = 0;
    if (!prts_vfs_path_valid(vpath)) return PRTS_VFS_INVALID_PATH;
    int idx = prts_vfs_find(vfs, vpath);
    if (idx < 0) return PRTS_VFS_NOT_FOUND;
    prts_mount *m = prts_array_at(&vfs->mounts, (size_t)idx);
    char *real = prts_path_join(m->dir, vpath);
    size_t size = 0;
    bool ok = prts_fs_read_file(real, out_data, &size);
    prts_free(real);
    if (out_size) *out_size = size;
    return ok ? PRTS_VFS_OK : PRTS_VFS_IO_ERROR;
}

typedef struct list_entry {
    char *name;
    bool is_dir;
} list_entry;

typedef struct list_ctx {
    prts_array *entries;
    const char *dir;
} list_ctx;

static void collect_entry(const char *name, void *user)
{
    list_ctx *ctx = user;
    for (size_t i = 0; i < ctx->entries->len; i++) {
        list_entry *e = prts_array_at(ctx->entries, i);
        if (strcmp(e->name, name) == 0) return;
    }
    char *real = prts_path_join(ctx->dir, name);
    list_entry e = {prts_strdup(name), prts_fs_is_dir(real)};
    prts_free(real);
    prts_array_push(ctx->entries, &e);
}

static int compare_entries(const void *a, const void *b)
{
    return strcmp(((const list_entry *)a)->name, ((const list_entry *)b)->name);
}

prts_vfs_result prts_vfs_list(prts_vfs *vfs, const char *vdir, prts_vfs_list_fn fn, void *user)
{
    bool root = !vdir || !*vdir;
    if (!root && !prts_vfs_path_valid(vdir)) return PRTS_VFS_INVALID_PATH;
    prts_array entries;
    prts_array_init(&entries, sizeof(list_entry));
    bool any = false;
    for (size_t i = 0; i < vfs->mounts.len; i++) {
        prts_mount *m = prts_array_at(&vfs->mounts, i);
        if (!root && !mount_has(m, vdir, true)) continue;
        char *real = root ? prts_strdup(m->dir) : prts_path_join(m->dir, vdir);
        list_ctx ctx = {&entries, real};
        any = prts_fs_enumerate(real, collect_entry, &ctx) || any;
        prts_free(real);
    }
    if (entries.len > 1) qsort(entries.data, entries.len, entries.elem, compare_entries);
    for (size_t i = 0; i < entries.len; i++) {
        list_entry *e = prts_array_at(&entries, i);
        fn(e->name, e->is_dir, user);
        prts_free(e->name);
    }
    prts_array_free(&entries);
    return any ? PRTS_VFS_OK : PRTS_VFS_NOT_FOUND;
}

void prts_vfs_set_user_dir(prts_vfs *vfs, const char *real_dir)
{
    prts_free(vfs->user_dir);
    vfs->user_dir = real_dir ? prts_strdup(real_dir) : NULL;
}

const char *prts_vfs_user_dir(const prts_vfs *vfs) { return vfs->user_dir; }

prts_vfs_result prts_vfs_user_write(prts_vfs *vfs, const char *vpath, const void *data, size_t size)
{
    if (!prts_vfs_path_valid(vpath)) return PRTS_VFS_INVALID_PATH;
    if (!vfs->user_dir) return PRTS_VFS_IO_ERROR;
    char *real = prts_path_join(vfs->user_dir, vpath);
    bool ok = prts_fs_write_file(real, data, size);
    prts_free(real);
    return ok ? PRTS_VFS_OK : PRTS_VFS_IO_ERROR;
}

prts_vfs_result prts_vfs_user_read(prts_vfs *vfs, const char *vpath, char **out_data, size_t *out_size)
{
    *out_data = NULL;
    if (!prts_vfs_path_valid(vpath)) return PRTS_VFS_INVALID_PATH;
    if (!vfs->user_dir) return PRTS_VFS_NOT_FOUND;
    char *real = prts_path_join(vfs->user_dir, vpath);
    bool found = prts_fs_is_file(real);
    bool ok = found && prts_fs_read_file(real, out_data, out_size);
    prts_free(real);
    if (!found) return PRTS_VFS_NOT_FOUND;
    return ok ? PRTS_VFS_OK : PRTS_VFS_IO_ERROR;
}
