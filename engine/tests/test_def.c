/* Unit tests: prts.def (definitions merged by id). MIT License. */
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

#include "prts/prts_log.h"
#include "prts/prts_script.h"
#include "utest.h"

static const char *def_eval(prts_script *s, const char *expr)
{
    static char buf[512];
    char code[1024];
    snprintf(code, sizeof(code), "__result = tostring(%s)", expr);
    if (!prts_script_run_string(s, code, "=test")) return "<error>";
    lua_State *L = prts_script_lua(s);
    lua_getglobal(L, "__result");
    snprintf(buf, sizeof(buf), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return buf;
}

UTEST(def, define_get_list_and_deep_merge)
{
    prts_vfs *v = prts_vfs_create();
    prts_script *s = prts_script_create(v, 1);
    ASSERT_TRUE(prts_script_run_string(s,
        "first = prts.def.define('unit', { id = 'archer', hp = 40, speed = 3.5, tags = { 'ranged', 'foot' },\n"
        "  weapon = { name = 'bow', damage = 6, range = { min = 2, max = 60 } }, [1] = 'one' })\n"
        "prts.def.define('unit', { id = 'pikeman', hp = 60 })\n"
        "prts.def.define('faction', { id = 'crown' })\n"
        /* a mod changes one nested field and replaces an array */
        "second = prts.def.define('unit', { id = 'archer', weapon = { range = { max = 80 } }, tags = { 'elite' },\n"
        "  speed = false, [true] = 'yes', [2.5] = 'half' })\n", "=defs"));
    ASSERT_STREQ("true", def_eval(s, "first"));
    ASSERT_STREQ("false", def_eval(s, "second"));
    ASSERT_STREQ("80", def_eval(s, "prts.def.get('unit', 'archer').weapon.range.max"));
    ASSERT_STREQ("2", def_eval(s, "prts.def.get('unit', 'archer').weapon.range.min"));
    ASSERT_STREQ("6", def_eval(s, "prts.def.get('unit', 'archer').weapon.damage"));
    ASSERT_STREQ("40", def_eval(s, "prts.def.get('unit', 'archer').hp"));
    ASSERT_STREQ("false", def_eval(s, "prts.def.get('unit', 'archer').speed"));
    ASSERT_STREQ("1", def_eval(s, "#prts.def.get('unit', 'archer').tags"));
    ASSERT_STREQ("elite", def_eval(s, "prts.def.get('unit', 'archer').tags[1]"));
    ASSERT_STREQ("yes", def_eval(s, "prts.def.get('unit', 'archer')[true]"));
    ASSERT_STREQ("half", def_eval(s, "prts.def.get('unit', 'archer')[2.5]"));
    ASSERT_STREQ("one", def_eval(s, "prts.def.get('unit', 'archer')[1]"));
    /* get returns a copy: changing it does not change the definition */
    ASSERT_STREQ("40", def_eval(s, "(function() local d = prts.def.get('unit', 'archer') d.hp = 1 return prts.def.get('unit', 'archer').hp end)()"));
    /* the registry keeps its own copy of what was defined */
    ASSERT_STREQ("60", def_eval(s, "(function() local t = { id = 'x', hp = 60 } prts.def.define('unit', t) t.hp = 2 return prts.def.get('unit', 'x').hp end)()"));
    ASSERT_STREQ("nil", def_eval(s, "prts.def.get('unit', 'nobody')"));
    ASSERT_STREQ("nil", def_eval(s, "prts.def.get('spell', 'fireball')"));
    ASSERT_STREQ("archer,pikeman,x", def_eval(s, "table.concat(prts.def.list('unit'), ',')"));
    ASSERT_STREQ("0", def_eval(s, "#prts.def.list('spell')"));
    ASSERT_STREQ("faction,unit", def_eval(s, "table.concat(prts.def.kinds(), ',')"));
    /* an empty table leaves a table field unchanged; a plain value replaces a table */
    ASSERT_STREQ("6", def_eval(s, "(function() prts.def.define('unit', { id = 'archer', weapon = {} }) return prts.def.get('unit', 'archer').weapon.damage end)()"));
    ASSERT_STREQ("none", def_eval(s, "(function() prts.def.define('unit', { id = 'archer', weapon = 'none' }) return prts.def.get('unit', 'archer').weapon end)()"));
    /* prts.def.<kind>{ ... } is shorthand for define */
    ASSERT_STREQ("true", def_eval(s, "prts.def.weapon{ id = 'crown.lance', damage = 9 }"));
    ASSERT_STREQ("false", def_eval(s, "prts.def.weapon({ id = 'crown.lance', reach = 3 }, 'ignored')"));
    ASSERT_STREQ("9 3", def_eval(s, "prts.def.get('weapon', 'crown.lance').damage .. ' ' .. prts.def.get('weapon', 'crown.lance').reach"));
    ASSERT_STREQ("nil", def_eval(s, "prts.def[1]"));
    ASSERT_STREQ("<error>", def_eval(s, "prts.def['bad kind']"));
    ASSERT_STREQ("<error>", def_eval(s, "prts.def.weapon('x')"));
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}

UTEST(def, rejects_code_and_bad_arguments_with_clear_errors)
{
    prts_vfs *v = prts_vfs_create();
    prts_script *s = prts_script_create(v, 1);
    prts_log_set_min_level(PRTS_LOG_ERROR + 1);
    struct {
        const char *code;
        const char *message;
    } cases[] = {
        {"prts.def.define('unit', { id = 'a', on_hit = function() end })", "unit 'a': field 'on_hit' is a function"},
        {"prts.def.define('unit', { id = 'a', ai = { think = { print } } })", "field 'ai.think[1]' is a function"},
        {"prts.def.define('unit', { id = 'a', co = coroutine.create(print) })", "field 'co' is a thread"},
        {"prts.def.define('unit', { id = 'a', [{}] = 1 })", "has a table as a key"},
        {"local t = { id = 'a' } t.self = t prts.def.define('unit', t)", "nested too deeply"},
        {"prts.def.define('unit', { hp = 1 })", "needs a non-empty string 'id'"},
        {"prts.def.define('unit', { id = '' })", "needs a non-empty string 'id'"},
        {"prts.def.define('unit', { id = 5 })", "needs a non-empty string 'id'"},
        {"prts.def.define('bad kind!', { id = 'a' })", "invalid definition kind"},
        {"prts.def.define('', { id = 'a' })", "invalid definition kind"},
        {"prts.def.define('unit', 5)", "table expected"},
        {"prts.def.get('unit')", "string expected"},
        {"prts.def.list('a b')", "invalid definition kind"},
    };
    for (size_t i = 0; i < PRTS_ARRAY_COUNT(cases); i++) {
        ASSERT_FALSE(prts_script_run_string(s, cases[i].code, "=bad"));
        ASSERT_TRUE(strstr(prts_script_last_error(s), cases[i].message) != NULL);
    }
    /* a rejected definition leaves nothing behind */
    ASSERT_STREQ("0", def_eval(s, "#prts.def.list('unit')"));
    prts_log_set_min_level(PRTS_LOG_INFO);
    ASSERT_EQ(0, lua_gettop(prts_script_lua(s)));
    prts_script_destroy(s);
    prts_vfs_destroy(v);
}
