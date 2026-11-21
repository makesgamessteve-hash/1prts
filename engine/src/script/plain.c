/* 1stPRTS engine: plain Lua data to bytes and back (saves, replays, state hashes). MIT License.
 *
 * Encoding: one tag byte per value, then its payload. Tables list their pairs with the keys in
 * a fixed order (numbers ascending, then strings by bytes, then false, true), so equal data
 * always gives equal bytes whatever order Lua's hash part keeps. A table met a second time is
 * written as a reference to its first appearance, so shared and cyclic tables survive. */
#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prts/prts_serial.h"
#include "script/script_internal.h"

enum {
    TAG_NIL = 0,
    TAG_FALSE,
    TAG_TRUE,
    TAG_INT,
    TAG_FLOAT,
    TAG_STRING,
    TAG_TABLE,
    TAG_REF,
    TAG_OPAQUE /* hash-only stand-in for a function, userdata or thread */
};

#define PLAIN_MAX_DEPTH 200
#define PLAIN_MAX_STRING (16u * 1024u * 1024u)

typedef struct plain_ctx {
    lua_State *L;
    prts_writer *w;
    bool strict;
    int refs; /* stack index: table -> id */
    lua_Integer next_id;
    const char *what;
    char path[200];
    size_t path_len;
    char *err;
    size_t err_size;
    bool failed;
} plain_ctx;

typedef struct key_desc {
    int rank; /* 0 number, 1 string, 2 boolean */
    bool is_int;
    lua_Integer i;
    double d;
    const char *s;
    size_t len;
    bool b;
} key_desc;

static int compare_keys(const void *pa, const void *pb)
{
    const key_desc *a = pa, *b = pb;
    if (a->rank != b->rank) return a->rank < b->rank ? -1 : 1;
    if (a->rank == 0) {
        if (a->is_int && b->is_int) return a->i < b->i ? -1 : a->i > b->i;
        double x = a->is_int ? (double)a->i : a->d, y = b->is_int ? (double)b->i : b->d;
        return x < y ? -1 : x > y;
    }
    if (a->rank == 1) {
        int c = memcmp(a->s, b->s, PRTS_MIN(a->len, b->len));
        if (c) return c;
        return a->len < b->len ? -1 : a->len > b->len;
    }
    return (int)a->b - (int)b->b;
}

static void fail(plain_ctx *c, const char *problem)
{
    if (c->failed) return;
    c->failed = true;
    if (c->path_len) {
        snprintf(c->err, c->err_size, "%s field '%s' %s; script data must be plain (numbers, strings, booleans and "
                 "tables) to be saved", c->what, c->path, problem);
    } else {
        snprintf(c->err, c->err_size, "%s %s; script data must be plain (numbers, strings, booleans and tables) to be "
                 "saved", c->what, problem);
    }
}

static void path_key(plain_ctx *c, const key_desc *k)
{
    char buf[64];
    int n;
    if (k->rank == 1) {
        n = snprintf(c->path + c->path_len, sizeof(c->path) - c->path_len, c->path_len ? ".%.*s" : "%.*s",
                     (int)PRTS_MIN(k->len, 60u), k->s);
    } else {
        if (k->rank == 2) snprintf(buf, sizeof(buf), "%s", k->b ? "true" : "false");
        else if (k->is_int) snprintf(buf, sizeof(buf), "%lld", (long long)k->i);
        else snprintf(buf, sizeof(buf), "%.17g", k->d);
        n = snprintf(c->path + c->path_len, sizeof(c->path) - c->path_len, "[%s]", buf);
    }
    if (n > 0) c->path_len = PRTS_MIN(c->path_len + (size_t)n, sizeof(c->path) - 1u);
}

static void write_key(plain_ctx *c, const key_desc *k)
{
    if (k->rank == 2) {
        prts_w_u8(c->w, k->b ? TAG_TRUE : TAG_FALSE);
    } else if (k->rank == 1) {
        prts_w_u8(c->w, TAG_STRING);
        prts_w_u32(c->w, (uint32_t)k->len);
        prts_w_bytes(c->w, k->s, k->len);
    } else if (k->is_int) {
        prts_w_u8(c->w, TAG_INT);
        prts_w_u64(c->w, (uint64_t)k->i);
    } else {
        prts_w_u8(c->w, TAG_FLOAT);
        prts_w_f64(c->w, k->d);
    }
}

static void push_key(lua_State *L, const key_desc *k)
{
    if (k->rank == 2) lua_pushboolean(L, k->b);
    else if (k->rank == 1) lua_pushlstring(L, k->s, k->len);
    else if (k->is_int) lua_pushinteger(L, k->i);
    else lua_pushnumber(L, k->d);
}

static void write_value(plain_ctx *c, int idx, int depth);

static void write_table(plain_ctx *c, int idx, int depth)
{
    lua_State *L = c->L;
    lua_pushvalue(L, idx);
    if (lua_rawget(L, c->refs) == LUA_TNUMBER) {
        prts_w_u8(c->w, TAG_REF);
        prts_w_u32(c->w, (uint32_t)lua_tointeger(L, -1));
        lua_pop(L, 1);
        return;
    }
    lua_pop(L, 1);
    if (depth > PLAIN_MAX_DEPTH) {
        fail(c, "is nested too deeply");
        return;
    }
    lua_pushvalue(L, idx);
    lua_pushinteger(L, c->next_id++);
    lua_rawset(L, c->refs);
    prts_array keys;
    prts_array_init(&keys, sizeof(key_desc));
    lua_pushnil(L);
    while (lua_next(L, idx)) {
        lua_pop(L, 1);
        key_desc k;
        memset(&k, 0, sizeof(k));
        int t = lua_type(L, -1);
        if (t == LUA_TNUMBER) {
            k.is_int = lua_isinteger(L, -1);
            if (k.is_int) k.i = lua_tointeger(L, -1);
            else k.d = lua_tonumber(L, -1);
        } else if (t == LUA_TSTRING) {
            k.rank = 1;
            k.s = lua_tolstring(L, -1, &k.len); /* stays valid: the table keeps its keys */
        } else if (t == LUA_TBOOLEAN) {
            k.rank = 2;
            k.b = lua_toboolean(L, -1);
        } else {
            if (c->strict) {
                fail(c, lua_pushfstring(L, "has a %s as a key", lua_typename(L, t)));
                lua_pop(L, 1);
            }
            continue; /* not data: left out of the hash */
        }
        prts_array_push(&keys, &k);
    }
    if (keys.len > 1) qsort(keys.data, keys.len, keys.elem, compare_keys);
    prts_w_u8(c->w, TAG_TABLE);
    prts_w_u32(c->w, (uint32_t)keys.len);
    luaL_checkstack(L, 4, "plain data too deep");
    for (size_t i = 0; i < keys.len && !c->failed; i++) {
        const key_desc *k = prts_array_at(&keys, i);
        size_t mark = c->path_len;
        write_key(c, k);
        push_key(L, k);
        lua_rawget(L, idx);
        path_key(c, k);
        write_value(c, lua_gettop(L), depth + 1);
        lua_pop(L, 1);
        c->path_len = mark;
        c->path[mark] = '\0';
    }
    prts_array_free(&keys);
}

static void write_value(plain_ctx *c, int idx, int depth)
{
    lua_State *L = c->L;
    switch (lua_type(L, idx)) {
    case LUA_TNIL: prts_w_u8(c->w, TAG_NIL); break;
    case LUA_TBOOLEAN: prts_w_u8(c->w, lua_toboolean(L, idx) ? TAG_TRUE : TAG_FALSE); break;
    case LUA_TNUMBER:
        if (lua_isinteger(L, idx)) {
            prts_w_u8(c->w, TAG_INT);
            prts_w_u64(c->w, (uint64_t)lua_tointeger(L, idx));
        } else {
            prts_w_u8(c->w, TAG_FLOAT);
            prts_w_f64(c->w, lua_tonumber(L, idx));
        }
        break;
    case LUA_TSTRING: {
        size_t len = 0;
        const char *s = lua_tolstring(L, idx, &len);
        prts_w_u8(c->w, TAG_STRING);
        prts_w_u32(c->w, (uint32_t)len);
        prts_w_bytes(c->w, s, len);
        break;
    }
    case LUA_TTABLE: write_table(c, idx, depth); break;
    default: {
        int t = lua_type(L, idx);
        if (c->strict) {
            fail(c, lua_pushfstring(L, "is a %s", lua_typename(L, t)));
            lua_pop(L, 1);
        }
        prts_w_u8(c->w, TAG_OPAQUE);
        prts_w_u8(c->w, (uint8_t)t);
    }
    }
}

bool prts_script_write_plain(prts_script *s, int idx, prts_writer *w, bool strict, const char *what, char *err,
                             size_t err_size)
{
    lua_State *L = s->L;
    idx = lua_absindex(L, idx);
    plain_ctx c;
    memset(&c, 0, sizeof(c));
    c.L = L;
    c.w = w;
    c.strict = strict;
    c.what = what ? what : "value";
    c.err = err;
    c.err_size = err_size;
    c.next_id = 1;
    lua_newtable(L);
    c.refs = lua_gettop(L);
    write_value(&c, idx, 0);
    lua_settop(L, c.refs - 1);
    return !c.failed;
}

/* --- reading ------------------------------------------------------------------------------ */

typedef struct read_ctx {
    lua_State *L;
    prts_reader *r;
    int refs;
    lua_Integer next_id;
} read_ctx;

static bool read_value(read_ctx *c, int depth, bool as_key)
{
    lua_State *L = c->L;
    prts_reader *r = c->r;
    uint8_t tag = prts_r_u8(r);
    if (r->failed || depth > PLAIN_MAX_DEPTH) return false;
    luaL_checkstack(L, 4, "plain data too deep");
    switch (tag) {
    case TAG_NIL:
        if (as_key) return false;
        lua_pushnil(L);
        return true;
    case TAG_FALSE:
    case TAG_TRUE: lua_pushboolean(L, tag == TAG_TRUE); return true;
    case TAG_INT: lua_pushinteger(L, (lua_Integer)prts_r_u64(r)); return !r->failed;
    case TAG_FLOAT: {
        double d = prts_r_f64(r);
        if (as_key && d != d) return false; /* NaN can't be a key */
        lua_pushnumber(L, d);
        return !r->failed;
    }
    case TAG_STRING: {
        uint32_t len = prts_r_u32(r);
        if (r->failed || len > PLAIN_MAX_STRING || len > r->len - r->pos) return false;
        lua_pushlstring(L, (const char *)r->data + r->pos, len);
        r->pos += len;
        return true;
    }
    case TAG_TABLE: {
        if (as_key) return false;
        uint32_t n = prts_r_u32(r);
        if (r->failed || n > r->len - r->pos) return false; /* each pair takes at least two bytes */
        lua_createtable(L, 0, (int)PRTS_MIN(n, 1024u));
        int t = lua_gettop(L);
        lua_pushvalue(L, t);
        lua_rawseti(L, c->refs, c->next_id++);
        for (uint32_t i = 0; i < n; i++) {
            if (!read_value(c, depth + 1, true)) return false;
            if (!read_value(c, depth + 1, false) || lua_isnil(L, -1)) return false;
            lua_rawset(L, t);
        }
        return true;
    }
    case TAG_REF: {
        uint32_t id = prts_r_u32(r);
        if (as_key || r->failed) return false;
        return lua_rawgeti(L, c->refs, (lua_Integer)id) == LUA_TTABLE;
    }
    default: return false; /* TAG_OPAQUE never appears in saved data */
    }
}

bool prts_script_read_plain(prts_script *s, prts_reader *r)
{
    lua_State *L = s->L;
    int top = lua_gettop(L);
    lua_newtable(L);
    read_ctx c = {L, r, top + 1, 1};
    bool ok = read_value(&c, 0, false);
    if (!ok) {
        r->failed = true;
        lua_settop(L, top);
        return false;
    }
    lua_remove(L, top + 1);
    return true;
}
