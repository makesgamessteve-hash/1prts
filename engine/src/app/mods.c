/* 1stPRTS engine: mod manifests, dependencies and load order. MIT License.
 *
 * A mod's mod.lua is a data-only chunk returning a table:
 *   { id = "my_mod", api = "1", version = "1.0.0",
 *     depends = { "base >= 0.1", "other_mod" }, load_after = { "maybe_mod" } }
 * `depends` names mods that must be enabled (and new enough); `load_after` only orders this mod
 * after other enabled mods. The load order is a topological order of both, with ties broken by
 * id, so it never depends on the order of --mod options. */
#include "app/mods.h"

#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/engine_internal.h"
#include "prts/prts_fs.h"
#include "prts/prts_log.h"

/* --- versions and dependency specs ---------------------------------------------------------- */

bool prts_version_parse(const char *text, uint32_t out[3])
{
    out[0] = out[1] = out[2] = 0;
    const char *p = text;
    for (int part = 0; part < 3; part++) {
        if (*p < '0' || *p > '9') return false;
        uint64_t v = 0;
        while (*p >= '0' && *p <= '9') {
            v = v * 10u + (uint64_t)(*p - '0');
            if (v > 0xffffffffu) return false;
            p++;
        }
        out[part] = (uint32_t)v;
        if (*p == '\0') return true;
        if (*p != '.') return false;
        p++;
    }
    return false; /* more than three parts */
}

int prts_version_compare(const uint32_t a[3], const uint32_t b[3])
{
    for (int i = 0; i < 3; i++) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    }
    return 0;
}

static bool id_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
}

bool prts_mod_dep_parse(const char *text, prts_mod_dep *out)
{
    memset(out, 0, sizeof(*out));
    const char *p = text;
    while (*p == ' ') p++;
    const char *id = p;
    while (id_char(*p)) p++;
    if (p == id) return false;
    size_t id_len = (size_t)(p - id);
    while (*p == ' ') p++;
    static const struct {
        const char *text;
        prts_version_op op;
    } ops[] = {{">=", PRTS_VER_GE}, {"<=", PRTS_VER_LE}, {"==", PRTS_VER_EQ},
               {">", PRTS_VER_GT},  {"<", PRTS_VER_LT},  {"=", PRTS_VER_EQ}};
    prts_version_op op = PRTS_VER_ANY;
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(ops) && *p; i++) {
        size_t n = strlen(ops[i].text);
        if (strncmp(p, ops[i].text, n) == 0) {
            op = ops[i].op;
            p += n;
            break;
        }
    }
    uint32_t version[3] = {0, 0, 0};
    if (op != PRTS_VER_ANY) {
        while (*p == ' ') p++;
        const char *v = p;
        while (*p && *p != ' ') p++;
        char buf[32];
        size_t len = (size_t)(p - v);
        if (len == 0 || len >= sizeof(buf)) return false;
        memcpy(buf, v, len);
        buf[len] = '\0';
        if (!prts_version_parse(buf, version)) return false;
        while (*p == ' ') p++;
    }
    if (*p != '\0') return false;
    out->id = prts_strndup(id, id_len);
    out->op = op;
    memcpy(out->version, version, sizeof(version));
    out->text = prts_strdup(text);
    return true;
}

bool prts_mod_dep_satisfied(const prts_mod_dep *dep, const uint32_t version[3])
{
    int c = prts_version_compare(version, dep->version);
    switch (dep->op) {
    case PRTS_VER_GE: return c >= 0;
    case PRTS_VER_GT: return c > 0;
    case PRTS_VER_EQ: return c == 0;
    case PRTS_VER_LE: return c <= 0;
    case PRTS_VER_LT: return c < 0;
    default: return true;
    }
}

void prts_mod_init(prts_mod *m, const char *id)
{
    memset(m, 0, sizeof(*m));
    m->id = prts_strdup(id);
    m->version_text = prts_strdup("0.0.0");
    prts_array_init(&m->depends, sizeof(prts_mod_dep));
    prts_array_init(&m->load_after, sizeof(char *));
}

void prts_mod_free(prts_mod *m)
{
    for (size_t i = 0; i < m->depends.len; i++) {
        prts_mod_dep *d = prts_array_at(&m->depends, i);
        prts_free(d->id);
        prts_free(d->text);
    }
    for (size_t i = 0; i < m->load_after.len; i++) prts_free(*(char **)prts_array_at(&m->load_after, i));
    prts_array_free(&m->depends);
    prts_array_free(&m->load_after);
    prts_free(m->id);
    prts_free(m->dir);
    prts_free(m->version_text);
    memset(m, 0, sizeof(*m));
}

/* --- resolution -------------------------------------------------------------------------------- */

static int find_mod(const prts_mod *mods, size_t count, const char *id)
{
    for (size_t i = 0; i < count; i++) {
        if (strcmp(mods[i].id, id) == 0) return (int)i;
    }
    return -1;
}

/* Skips mods whose dependencies are missing, skipped or too old, until nothing changes. */
static void check_dependencies(prts_mod *mods, size_t count, const prts_mod *base)
{
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < count; i++) {
            prts_mod *m = &mods[i];
            for (size_t d = 0; d < m->depends.len && !m->skip; d++) {
                const prts_mod_dep *dep = prts_array_at(&m->depends, d);
                int t = find_mod(mods, count, dep->id);
                const prts_mod *target = strcmp(dep->id, base->id) == 0 ? base : t >= 0 ? &mods[t] : NULL;
                if (!target || target->skip) {
                    prts_log_error("mod '%s' needs '%s', which is %s; continuing without '%s'", m->id, dep->text,
                                   target ? "skipped" : "not enabled or not installed", m->id);
                } else if (!prts_mod_dep_satisfied(dep, target->version)) {
                    prts_log_error("mod '%s' needs '%s', but '%s' %s is installed; continuing without '%s'", m->id,
                                   dep->text, target->id, target->version_text, m->id);
                } else {
                    continue;
                }
                m->skip = true;
                changed = true;
            }
        }
    }
}

/* True if mod j must load before mod i (i depends on j or loads after it). */
static bool must_precede(const prts_mod *mods, size_t i, size_t j)
{
    const prts_mod *m = &mods[i];
    for (size_t d = 0; d < m->depends.len; d++) {
        if (strcmp(((prts_mod_dep *)prts_array_at(&m->depends, d))->id, mods[j].id) == 0) return true;
    }
    for (size_t a = 0; a < m->load_after.len; a++) {
        if (strcmp(*(char **)prts_array_at(&m->load_after, a), mods[j].id) == 0) return true;
    }
    return false;
}

size_t prts_mods_resolve(prts_mod *mods, size_t count, const prts_mod *base, size_t *order)
{
    check_dependencies(mods, count, base);
    bool *placed = prts_calloc(count ? count : 1u, sizeof(bool));
    size_t n = 0;
    for (;;) {
        /* the ready mod with the smallest id: every enabled mod it follows is placed */
        int pick = -1;
        for (size_t i = 0; i < count; i++) {
            if (mods[i].skip || placed[i]) continue;
            bool ready = true;
            for (size_t j = 0; j < count && ready; j++) {
                if (j != i && !mods[j].skip && !placed[j] && must_precede(mods, i, j)) ready = false;
            }
            if (ready && (pick < 0 || strcmp(mods[i].id, mods[pick].id) < 0)) pick = (int)i;
        }
        if (pick < 0) break;
        placed[pick] = true;
        order[n++] = (size_t)pick;
    }
    /* whatever is left waits on itself: a cycle (or a mod that needs a mod in one) */
    prts_str left = {0};
    for (size_t i = 0; i < count; i++) {
        if (mods[i].skip || placed[i]) continue;
        prts_str_appendf(&left, "%s'%s'", left.len ? ", " : "", mods[i].id);
        mods[i].skip = true;
    }
    if (left.len) {
        prts_log_error("mods %s depend on each other in a cycle (depends/load_after); continuing without them",
                       left.data);
    }
    prts_str_free(&left);
    prts_free(placed);
    return n;
}

/* --- manifests ---------------------------------------------------------------------------------- */

/* Reads an array of strings from field `key` of the table on top of the stack. */
static bool read_string_list(lua_State *L, const char *key, prts_mod *m, bool deps, char *err, size_t err_size)
{
    lua_getfield(L, -1, key);
    int t = lua_gettop(L);
    bool ok = true;
    if (!lua_isnil(L, t) && !lua_istable(L, t)) {
        snprintf(err, err_size, "mod '%s': '%s' in mod.lua must be a list of strings", m->id, key);
        ok = false;
    }
    for (lua_Integer i = 1; ok && lua_istable(L, t) && lua_rawgeti(L, t, i) != LUA_TNIL; i++) {
        const char *text = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : NULL;
        prts_mod_dep dep = {NULL, PRTS_VER_ANY, {0, 0, 0}, NULL};
        ok = text && prts_mod_dep_parse(text, &dep);
        if (ok && deps) {
            prts_array_push(&m->depends, &dep);
        } else if (ok) {
            ok = dep.op == PRTS_VER_ANY; /* load_after names mods, without versions */
            if (ok) prts_array_push(&m->load_after, &dep.id);
            else prts_free(dep.id);
            prts_free(dep.text);
        }
        if (!ok) {
            snprintf(err, err_size, "mod '%s' has an invalid %s entry '%s' in mod.lua", m->id, key,
                     text ? text : "(not a string)");
        }
        lua_pop(L, 1);
    }
    lua_settop(L, t - 1);
    return ok;
}

/* Reads and checks dir/mod.lua into m; returns false (with a message) if it can't be used. */
static bool read_manifest(prts_engine *e, const char *dir, prts_mod *m, char *err, size_t err_size)
{
    const char *id = m->id;
    char *path = prts_path_join(dir, "mod.lua");
    char *code = NULL;
    bool ok = prts_fs_read_file(path, &code, NULL);
    prts_free(path);
    if (!ok) {
        snprintf(err, err_size, "mod '%s' has no mod.lua manifest in %s", id, dir);
        return false;
    }
    lua_State *L = prts_script_lua(e->script);
    ok = prts_script_eval_data(e->script, code, "=mod.lua");
    prts_free(code);
    if (!ok || !lua_istable(L, -1)) {
        if (ok) lua_pop(L, 1);
        snprintf(err, err_size, "mod '%s' has an invalid mod.lua manifest", id);
        return false;
    }
    lua_getfield(L, -1, "api");
    const char *api = lua_tostring(L, -1);
    char want[16];
    snprintf(want, sizeof(want), "%d", PRTS_API_VERSION);
    ok = api && strcmp(api, want) == 0;
    if (!ok) {
        /* Spec: Incompatible mod */
        snprintf(err, err_size, "mod '%s' targets scripting API %s, but this engine provides API %s", id,
                 api ? api : "(none)", want);
    }
    lua_pop(L, 1);
    lua_getfield(L, -1, "id");
    if (ok && (!lua_isstring(L, -1) || strcmp(lua_tostring(L, -1), id) != 0)) {
        snprintf(err, err_size, "mod folder '%s' has a mod.lua with id '%s' (they must match)", id,
                 lua_isstring(L, -1) ? lua_tostring(L, -1) : "(none)");
        ok = false;
    }
    lua_pop(L, 1);
    lua_getfield(L, -1, "version");
    if (ok && !lua_isnil(L, -1)) {
        const char *v = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "";
        ok = prts_version_parse(v, m->version);
        if (ok) {
            prts_free(m->version_text);
            m->version_text = prts_strdup(v);
        } else {
            snprintf(err, err_size, "mod '%s' has an invalid version in mod.lua (use \"1.2.3\")", id);
        }
    }
    lua_pop(L, 1);
    ok = ok && read_string_list(L, "depends", m, true, err, err_size);
    ok = ok && read_string_list(L, "load_after", m, false, err, err_size);
    lua_pop(L, 1);
    if (ok) m->dir = prts_strdup(dir);
    return ok;
}

bool prts_engine_check_base(prts_engine *e, const char *dir, char *err, size_t err_size)
{
    prts_mod base;
    prts_mod_init(&base, "base");
    bool ok = read_manifest(e, dir, &base, err, err_size);
    if (ok) e->mod_versions[0] = prts_strdup(base.version_text);
    prts_mod_free(&base);
    return ok;
}

/* Finds a mod folder: data/<id>, <install>/mods/<id> or <user>/mods/<id>. */
static char *find_mod_dir(const prts_engine *e, const char *id)
{
    char *parent = prts_path_join(e->data_dir, "..");
    char *install_mods = prts_path_join(parent, "mods");
    char *user_mods = prts_path_join(e->user_dir, "mods");
    char *candidates[3] = {prts_path_join(e->data_dir, id), prts_path_join(install_mods, id),
                           prts_path_join(user_mods, id)};
    char *found = NULL;
    for (int c = 0; c < 3; c++) {
        if (!found && prts_fs_is_dir(candidates[c])) found = candidates[c];
        else prts_free(candidates[c]);
    }
    prts_free(parent);
    prts_free(install_mods);
    prts_free(user_mods);
    return found;
}

void prts_engine_mount_mods(prts_engine *e)
{
    prts_mod base;
    prts_mod_init(&base, "base");
    prts_version_parse(e->mod_versions[0] ? e->mod_versions[0] : "0", base.version);
    prts_free(base.version_text);
    base.version_text = prts_strdup(e->mod_versions[0] ? e->mod_versions[0] : "0.0.0");

    prts_mod mods[PRTS_MAX_MODS];
    size_t count = 0;
    for (int i = 0; i < e->cfg.mod_count; i++) {
        const char *id = e->cfg.mods[i];
        if (strcmp(id, "base") == 0 || find_mod(mods, count, id) >= 0) {
            prts_log_warn("mod '%s' is enabled more than once; ignoring the repeat", id);
            continue;
        }
        char *dir = find_mod_dir(e, id);
        if (!dir) {
            prts_log_error("mod '%s' was not found; continuing without it", id);
            continue;
        }
        prts_mod *m = &mods[count];
        prts_mod_init(m, id);
        char err[256];
        if (read_manifest(e, dir, m, err, sizeof(err))) {
            count++;
        } else {
            prts_log_error("%s; continuing without it", err);
            prts_mod_free(m);
        }
        prts_free(dir);
    }
    size_t order[PRTS_MAX_MODS];
    size_t n = prts_mods_resolve(mods, count, &base, order);
    prts_str list = {0};
    for (size_t i = 0; i < n; i++) {
        prts_mod *m = &mods[order[i]];
        prts_vfs_mount(e->vfs, m->dir, m->id);
        e->mod_versions[prts_vfs_mount_count(e->vfs) - 1u] = prts_strdup(m->version_text);
        prts_log_info("mounted mod '%s' %s from %s", m->id, m->version_text, m->dir);
        prts_str_appendf(&list, "%s%s", i ? ", " : "", m->id);
    }
    if (n > 1) prts_log_info("mod load order: %s", list.data);
    prts_str_free(&list);
    for (size_t i = 0; i < count; i++) prts_mod_free(&mods[i]);
    prts_mod_free(&base);
}
