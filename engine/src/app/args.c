/* 1stPRTS engine: command-line options. MIT License. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prts/prts_engine.h"

void prts_engine_config_default(prts_engine_config *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->seed = 1;
    cfg->width = 1280;
    cfg->height = 720;
}

const char *prts_args_usage(void)
{
    return "usage: 1stprts [options]\n"
           "  --data <dir>          data folder (default: data/ next to the executable)\n"
           "  --user <dir>          user data folder (settings, saves, logs)\n"
           "  --mod <id>            enable a mod (repeatable)\n"
           "  --headless            run without window, GPU or audio\n"
           "  --hidden              create the window hidden (screenshots)\n"
           "  --size <w>x<h>        window size (default 1280x720)\n"
           "  --seed <n>            simulation seed (default 1)\n"
           "  --ticks <n>           stop after n simulation ticks\n"
           "  --test-lua            run the Lua unit tests in scripts/tests and exit\n"
           "  --scenario <file>     run an automated game test and exit\n"
           "  --out <dir>           where scenario screenshots are written\n"
           "  --start <map>         start a map at once (skips the front-end menu)\n"
           "  --front-end           show the front-end menu even in test runs\n"
           "  --no-dev              turn developer features (console, observer camera) off\n"
           "  --lua-coverage <file> write Lua line coverage (lcov) on exit\n"
           "  --version, --help\n";
}

static bool parse_u64(const char *s, uint64_t *out)
{
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (!*s || *end || *s == '-') return false;
    *out = (uint64_t)v;
    return true;
}

int prts_args_parse(int argc, char **argv, prts_engine_config *cfg, char *err, size_t err_size)
{
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *v = i + 1 < argc ? argv[i + 1] : NULL;
        bool takes = strcmp(a, "--data") == 0 || strcmp(a, "--user") == 0 || strcmp(a, "--mod") == 0 ||
                     strcmp(a, "--size") == 0 || strcmp(a, "--seed") == 0 || strcmp(a, "--ticks") == 0 ||
                     strcmp(a, "--scenario") == 0 || strcmp(a, "--out") == 0 || strcmp(a, "--lua-coverage") == 0 ||
                     strcmp(a, "--start") == 0;
        if (takes && !v) {
            snprintf(err, err_size, "option %s needs a value", a);
            return 1;
        }
        if (strcmp(a, "--help") == 0) {
            snprintf(err, err_size, "%s", prts_args_usage());
            return 2;
        } else if (strcmp(a, "--version") == 0) {
            snprintf(err, err_size, "1stPRTS engine %s (scripting API %d)\n", PRTS_ENGINE_VERSION, PRTS_API_VERSION);
            return 2;
        } else if (strcmp(a, "--headless") == 0) {
            cfg->headless = true;
        } else if (strcmp(a, "--hidden") == 0) {
            cfg->hidden = true;
        } else if (strcmp(a, "--test-lua") == 0) {
            cfg->test_lua = true;
        } else if (strcmp(a, "--front-end") == 0) {
            cfg->front_end = true;
        } else if (strcmp(a, "--no-dev") == 0) {
            cfg->no_dev = true;
        } else if (strcmp(a, "--start") == 0) {
            cfg->start_map = v;
            i++;
        } else if (strcmp(a, "--data") == 0) {
            cfg->data_dir = v;
            i++;
        } else if (strcmp(a, "--user") == 0) {
            cfg->user_dir = v;
            i++;
        } else if (strcmp(a, "--scenario") == 0) {
            cfg->scenario = v;
            i++;
        } else if (strcmp(a, "--out") == 0) {
            cfg->out_dir = v;
            i++;
        } else if (strcmp(a, "--lua-coverage") == 0) {
            cfg->lua_coverage = v;
            i++;
        } else if (strcmp(a, "--mod") == 0) {
            if (cfg->mod_count >= PRTS_MAX_MODS) {
                snprintf(err, err_size, "too many --mod options (at most %d)", PRTS_MAX_MODS);
                return 1;
            }
            cfg->mods[cfg->mod_count++] = v;
            i++;
        } else if (strcmp(a, "--seed") == 0 || strcmp(a, "--ticks") == 0) {
            uint64_t n = 0;
            if (!parse_u64(v, &n)) {
                snprintf(err, err_size, "option %s needs a whole number, not '%s'", a, v);
                return 1;
            }
            if (a[2] == 's') cfg->seed = n;
            else cfg->ticks = (int)n;
            i++;
        } else if (strcmp(a, "--size") == 0) {
            int w = 0, h = 0;
            char tail = 0;
            if (sscanf(v, "%dx%d%c", &w, &h, &tail) != 2 || w < 64 || h < 64) {
                snprintf(err, err_size, "option --size needs WIDTHxHEIGHT (at least 64x64), not '%s'", v);
                return 1;
            }
            cfg->width = w;
            cfg->height = h;
            i++;
        } else {
            snprintf(err, err_size, "unknown option '%s'\n%s", a, prts_args_usage());
            return 1;
        }
    }
    return 0;
}
