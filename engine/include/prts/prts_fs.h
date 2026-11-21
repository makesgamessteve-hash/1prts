/* 1stPRTS engine: real file-system access (implemented in platform/). MIT License. */
#ifndef PRTS_FS_H
#define PRTS_FS_H

#include "prts_base.h"

bool prts_fs_is_dir(const char *path);
bool prts_fs_is_file(const char *path);
/* Calls fn for each entry name in dir (no "." or ".."). Returns false if dir can't be read. */
typedef void (*prts_fs_enum_fn)(const char *name, void *user);
bool prts_fs_enumerate(const char *dir, prts_fs_enum_fn fn, void *user);
/* Reads a whole file into a NUL-terminated buffer the caller frees with prts_free. */
bool prts_fs_read_file(const char *path, char **out_data, size_t *out_size);
/* Writes a whole file, creating parent directories. */
bool prts_fs_write_file(const char *path, const void *data, size_t size);
bool prts_fs_mkdirs(const char *path);
bool prts_fs_remove_tree(const char *path);
/* Directory of the executable (with trailing separator removed). Caller frees. */
char *prts_fs_base_dir(void);
/* Per-user writable directory for 1stPRTS. Caller frees. NULL if unavailable. */
char *prts_fs_user_dir(void);

#endif
