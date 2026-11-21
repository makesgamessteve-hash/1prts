/* 1stPRTS engine: mod manifests, dependencies and load order. MIT License. */
#ifndef PRTS_MODS_H
#define PRTS_MODS_H

#include "prts/prts_base.h"

typedef struct prts_engine prts_engine;

typedef enum prts_version_op {
    PRTS_VER_ANY = 0, /* "id" */
    PRTS_VER_GE,      /* "id >= 1.2" */
    PRTS_VER_GT,      /* "id > 1.2" */
    PRTS_VER_EQ,      /* "id = 1.2" or "id == 1.2" */
    PRTS_VER_LE,      /* "id <= 1.2" */
    PRTS_VER_LT       /* "id < 1.2" */
} prts_version_op;

typedef struct prts_mod_dep {
    char *id;
    prts_version_op op;
    uint32_t version[3];
    char *text; /* as written, for messages */
} prts_mod_dep;

typedef struct prts_mod {
    char *id;
    char *dir;
    char *version_text;
    char *title; /* the game's name for the window title (base game only), or NULL */
    uint32_t version[3];
    prts_array depends;    /* prts_mod_dep */
    prts_array load_after; /* char * */
    bool skip;
} prts_mod;

/* "1", "1.2" or "1.2.3" (missing parts are 0). */
bool prts_version_parse(const char *text, uint32_t out[3]);
int prts_version_compare(const uint32_t a[3], const uint32_t b[3]);
/* Parses "id", "id >= 1.2" and the other operators. Fills out (caller frees with
 * prts_mod_free's helpers) and returns false for malformed text. */
bool prts_mod_dep_parse(const char *text, prts_mod_dep *out);
bool prts_mod_dep_satisfied(const prts_mod_dep *dep, const uint32_t version[3]);

void prts_mod_init(prts_mod *m, const char *id);
void prts_mod_free(prts_mod *m);

/* Decides which mods load and in which order. `base` is always available. Mods with missing
 * or too-old dependencies, or in a dependency/load_after cycle, get skip = true and a logged
 * message. Writes the indices of the mods to load, in load order (dependencies and load_after
 * targets first, ties by id), to order and returns how many there are. */
size_t prts_mods_resolve(prts_mod *mods, size_t count, const prts_mod *base, size_t *order);

/* Engine boot: checks the base game's manifest (fills err; its `title` names the window), and
 * finds, checks, orders and mounts the mods named with --mod. */
bool prts_engine_check_base(prts_engine *e, const char *dir, char *err, size_t err_size);
void prts_engine_mount_mods(prts_engine *e);

#endif
