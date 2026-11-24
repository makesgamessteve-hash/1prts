/* 1stPRTS engine: prts.files, plain text files the player makes, kept in the user data folder
 * (files/<name>): maps from the editor, notes. Scripts only reach this one folder, by name.
 * MIT License. */
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_fs.h"

#define FILES_DIR "files"
#define FILES_MAX (1024 * 1024) /* bytes in one file */

/* A name: 1-64 letters, digits, '_' or '-', with an optional extension of 1-8 lowercase
 * letters or digits after one dot ("keep_test.map"). */
bool prts_files_name_valid(const char *name)
{
    size_t n = strlen(name);
    if (n == 0) return false;
    const char *dot = strchr(name, '.');
    size_t stem = dot ? (size_t)(dot - name) : n;
    if (stem == 0 || stem > 64) return false;
    for (size_t i = 0; i < stem; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return false;
        }
    }
    if (!dot) return true;
    size_t ext = n - stem - 1;
    if (ext == 0 || ext > 8) return false;
    for (const char *p = dot + 1; *p; p++) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9'))) return false;
    }
    return true;
}

static const char *check_name(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    if (!prts_files_name_valid(name)) {
        luaL_error(L, "invalid file name '%s' (use 1-64 letters, digits, '_' or '-', and an optional .ext)", name);
    }
    return lua_pushfstring(L, FILES_DIR "/%s", name);
}

static int fail(lua_State *L, const char *msg)
{
    lua_pushnil(L);
    lua_pushstring(L, msg);
    return 2;
}

/* prts.files.write(name, text) -> true | nil, reason */
static int l_files_write(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *vpath = check_name(L);
    size_t len = 0;
    const char *text = luaL_checklstring(L, 2, &len);
    if (len > FILES_MAX) return fail(L, "too large (at most 1 MB)");
    if (prts_vfs_user_write(e->vfs, vpath, text, len) != PRTS_VFS_OK) return fail(L, "cannot write in the user folder");
    lua_pushboolean(L, 1);
    return 1;
}

/* prts.files.read(name) -> text | nil, reason */
static int l_files_read(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *vpath = check_name(L);
    char *data = NULL;
    size_t size = 0;
    prts_vfs_result r = prts_vfs_user_read(e->vfs, vpath, &data, &size);
    if (r != PRTS_VFS_OK) return fail(L, prts_vfs_result_name(r));
    lua_pushlstring(L, data, size);
    prts_free(data);
    return 1;
}

typedef struct names {
    char **v;
    int n, cap;
} names;

static void collect(const char *name, void *user)
{
    names *s = user;
    if (!prts_files_name_valid(name)) return;
    if (s->n == s->cap) {
        s->cap = s->cap ? s->cap * 2 : 16;
        s->v = prts_realloc(s->v, (size_t)s->cap * sizeof(*s->v));
    }
    s->v[s->n++] = prts_strdup(name);
}

static int by_name(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

/* prts.files.list() -> sorted array of names */
static int l_files_list(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    names s = {0};
    char *dir = prts_path_join(e->user_dir, FILES_DIR);
    prts_fs_enumerate(dir, collect, &s);
    prts_free(dir);
    if (s.n > 1) qsort(s.v, (size_t)s.n, sizeof(*s.v), by_name);
    lua_createtable(L, s.n, 0);
    for (int i = 0; i < s.n; i++) {
        lua_pushstring(L, s.v[i]);
        lua_rawseti(L, -2, i + 1);
        prts_free(s.v[i]);
    }
    prts_free(s.v);
    return 1;
}

/* prts.files.remove(name) -> true if a file was removed */
static int l_files_remove(lua_State *L)
{
    prts_engine *e = prts_engine_from_lua(L);
    const char *vpath = check_name(L);
    char *real = prts_path_join(e->user_dir, vpath);
    bool ok = prts_fs_is_file(real) && prts_fs_remove_tree(real);
    prts_free(real);
    lua_pushboolean(L, ok);
    return 1;
}

void prts_engine_open_files_api(prts_engine *e)
{
    static const prts_script_fn files[] = {{"write", l_files_write},
                                           {"read", l_files_read},
                                           {"list", l_files_list},
                                           {"remove", l_files_remove},
                                           {NULL, NULL}};
    prts_script_register(e->script, "files", files);
}
