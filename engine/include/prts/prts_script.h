/* 1stPRTS engine: sandboxed Lua scripting runtime. MIT License. */
#ifndef PRTS_SCRIPT_H
#define PRTS_SCRIPT_H

#include "prts_base.h"
#include "prts_rng.h"
#include "prts_vfs.h"

typedef struct lua_State lua_State;
typedef struct prts_script prts_script;

prts_script *prts_script_create(prts_vfs *vfs, uint64_t seed);
void prts_script_destroy(prts_script *s);
lua_State *prts_script_lua(prts_script *s);
prts_script *prts_script_from_lua(lua_State *L);

/* Runs a script file from the VFS in its mod's environment. Errors are logged. */
bool prts_script_run_file(prts_script *s, const char *vpath);
/* Runs source text in the shared environment. Errors are logged. */
bool prts_script_run_string(prts_script *s, const char *code, const char *chunk_name);
/* Calls the function below nargs arguments on the stack (like lua_pcall) with error
 * containment, a traceback and the instruction budget. On error nothing is left. */
bool prts_script_call(prts_script *s, int nargs, int nresults);
/* Evaluates a data-only chunk (no globals, small budget). Pushes the result on success. */
bool prts_script_eval_data(prts_script *s, const char *code, const char *chunk_name);

/* Hard limit on Lua instructions per entry point (deterministic). */
void prts_script_set_budget(prts_script *s, uint64_t instructions);
/* Starts a new budget window, for entry points that resume coroutines directly. */
void prts_script_reset_budget(prts_script *s);
size_t prts_script_error_count(const prts_script *s);
const char *prts_script_last_error(const prts_script *s);

/* Registers C functions as prts.<ns>.<name>; ns NULL registers into prts itself. */
typedef struct prts_script_fn {
    const char *name;
    int (*fn)(lua_State *L);
} prts_script_fn;
void prts_script_register(prts_script *s, const char *ns, const prts_script_fn *fns);

/* Opaque engine pointers that API functions can look up by key. */
void prts_script_set_context(prts_script *s, const char *key, void *ptr);
void *prts_script_get_context(prts_script *s, const char *key);

/* Events: calls every handler registered with prts.events.on(event, key, fn), in key
 * order, with the nargs values on top of the stack (which are popped). Returns the number
 * of handlers that failed. */
int prts_script_emit(prts_script *s, const char *event, int nargs);

/* Simulation clock and random stream visible to scripts. */
void prts_script_set_time(prts_script *s, uint64_t tick, double tick_seconds);
prts_rng *prts_script_rng(prts_script *s);

/* Hot reload (developer builds): remembers the file of every module loaded with require.
 * check_reload re-runs the modules whose file changed since (in their mod's environment),
 * updates the require cache (a module table is updated in place, so code holding it sees the
 * new functions), then calls main.lua's on_reload(names) if it defines one. Entity data is not
 * touched. Returns how many modules were reloaded. */
void prts_script_enable_hot_reload(prts_script *s);
int prts_script_check_reload(prts_script *s);

/* Line coverage of Lua chunks, written in lcov format. */
void prts_script_enable_coverage(prts_script *s);
bool prts_script_write_coverage(prts_script *s, const char *real_path);

/* Plain data (saves, replays, state hashes): nil, booleans, numbers, strings and tables of
 * them; shared and cyclic tables are kept. Equal data always gives equal bytes (keys are
 * written in a fixed order). With strict, any other value (function, userdata, thread, or a
 * table used as a key) fails and err names the field, after `what`; without strict such
 * values are written as a stand-in for their type, which is enough for hashing. */
struct prts_writer;
struct prts_reader;
bool prts_script_write_plain(prts_script *s, int idx, struct prts_writer *w, bool strict, const char *what,
                             char *err, size_t err_size);
/* Pushes the value read. Returns false (pushing nothing, marking the reader failed) for
 * malformed data. */
bool prts_script_read_plain(prts_script *s, struct prts_reader *r);

#endif
