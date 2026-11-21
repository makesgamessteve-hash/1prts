/* 1stPRTS engine: scripting internals shared by the script module's files. MIT License. */
#ifndef PRTS_SCRIPT_INTERNAL_H
#define PRTS_SCRIPT_INTERNAL_H

#include <lua.h>

#include "prts/prts_script.h"

struct prts_script {
    lua_State *L;
    prts_vfs *vfs;
    uint64_t budget;
    uint64_t instructions;
    int depth;
    size_t errors;
    char *last_error;
    prts_rng rng;
    uint64_t tick;
    double tick_seconds;
    bool coverage;
    prts_array cov_files; /* prts_cov_file */
};

void prts_script_open_core_api(prts_script *s);

void prts_coverage_register_chunk(prts_script *s, lua_State *L, const char *vpath, const char *mount_dir);
void prts_coverage_hit(prts_script *s, lua_State *L, lua_Debug *ar);
void prts_coverage_free(prts_script *s);

#endif
