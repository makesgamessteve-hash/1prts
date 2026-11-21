/* 1stPRTS engine: real file-system access on top of SDL3. MIT License. */
#include <SDL3/SDL.h>
#include <string.h>

#include "prts/prts_fs.h"

bool prts_fs_is_dir(const char *path)
{
    SDL_PathInfo info;
    return SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY;
}

bool prts_fs_is_file(const char *path)
{
    SDL_PathInfo info;
    return SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_FILE;
}

typedef struct enum_ctx {
    prts_fs_enum_fn fn;
    void *user;
} enum_ctx;

static SDL_EnumerationResult SDLCALL enum_cb(void *userdata, const char *dirname, const char *fname)
{
    PRTS_UNUSED(dirname);
    enum_ctx *ctx = userdata;
    if (strcmp(fname, ".") != 0 && strcmp(fname, "..") != 0) ctx->fn(fname, ctx->user);
    return SDL_ENUM_CONTINUE;
}

bool prts_fs_enumerate(const char *dir, prts_fs_enum_fn fn, void *user)
{
    if (!prts_fs_is_dir(dir)) return false;
    enum_ctx ctx = {fn, user};
    return SDL_EnumerateDirectory(dir, enum_cb, &ctx);
}

bool prts_fs_read_file(const char *path, char **out_data, size_t *out_size)
{
    size_t size = 0;
    void *data = SDL_LoadFile(path, &size); /* SDL adds a terminating NUL */
    if (!data) {
        *out_data = NULL;
        return false;
    }
    char *copy = prts_malloc(size + 1);
    memcpy(copy, data, size);
    copy[size] = '\0';
    SDL_free(data);
    *out_data = copy;
    if (out_size) *out_size = size;
    return true;
}

bool prts_fs_mkdirs(const char *path) { return SDL_CreateDirectory(path); }

bool prts_fs_write_file(const char *path, const void *data, size_t size)
{
    const char *slash = strrchr(path, '/');
    if (slash && slash != path) {
        char *parent = prts_strndup(path, (size_t)(slash - path));
        bool ok = prts_fs_mkdirs(parent);
        prts_free(parent);
        if (!ok) return false;
    }
    return SDL_SaveFile(path, data, size);
}

static SDL_EnumerationResult SDLCALL remove_cb(void *userdata, const char *dirname, const char *fname)
{
    PRTS_UNUSED(userdata);
    char *full = prts_path_join(dirname, fname);
    if (prts_fs_is_dir(full)) prts_fs_remove_tree(full);
    else SDL_RemovePath(full);
    prts_free(full);
    return SDL_ENUM_CONTINUE;
}

bool prts_fs_remove_tree(const char *path)
{
    if (prts_fs_is_dir(path)) SDL_EnumerateDirectory(path, remove_cb, NULL);
    return SDL_RemovePath(path);
}

static char *strip_trailing_sep(const char *p)
{
    char *s = prts_strdup(p);
    size_t n = strlen(s);
    while (n > 1 && (s[n - 1] == '/' || s[n - 1] == '\\')) s[--n] = '\0';
    return s;
}

char *prts_fs_base_dir(void)
{
    const char *base = SDL_GetBasePath();
    return strip_trailing_sep(base ? base : ".");
}

char *prts_fs_user_dir(void)
{
    char *pref = SDL_GetPrefPath("1stPRTS", "1stprts");
    if (!pref) return NULL;
    char *out = strip_trailing_sep(pref);
    SDL_free(pref);
    return out;
}

bool prts_fs_stat(const char *path, int64_t *mtime_ns, uint64_t *size)
{
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path, &info)) return false;
    *mtime_ns = (int64_t)info.modify_time;
    *size = (uint64_t)info.size;
    return true;
}
