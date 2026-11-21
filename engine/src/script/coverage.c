/* 1stPRTS engine: Lua line coverage (lcov output). MIT License.
 * Executable lines are found by walking the function prototypes of each loaded chunk,
 * which needs Lua's internal headers (the VM is vendored, see third_party/lua). */
#include <lauxlib.h>
#include <ldebug.h>
#include <lobject.h>
#include <lopcodes.h>
#include <lstate.h>
#include <stdio.h>
#include <string.h>

#include "prts/prts_fs.h"
#include "script/script_internal.h"

typedef struct prts_cov_file {
    char *vpath;     /* source name without '@' */
    char *real_path; /* file on disk */
    int max_line;
    unsigned char *executable; /* per line: 1 if executable */
    uint32_t *hits;            /* per line hit counts */
} prts_cov_file;

static prts_cov_file *find_file(prts_script *s, const char *vpath)
{
    for (size_t i = 0; i < s->cov_files.len; i++) {
        prts_cov_file *f = prts_array_at(&s->cov_files, i);
        if (strcmp(f->vpath, vpath) == 0) return f;
    }
    return NULL;
}

static void mark_lines(prts_cov_file *f, const Proto *p)
{
    /* The last instruction is the implicit 'return' at 'end', which is not a real line, and
     * VARARGPREP runs before line hooks start, so neither counts as an executable line. */
    for (int pc = 0; pc + 1 < p->sizecode; pc++) {
        if (GET_OPCODE(p->code[pc]) == OP_VARARGPREP) continue;
        int line = luaG_getfuncline(p, pc);
        if (line > 0 && line <= f->max_line) f->executable[line] = 1;
    }
    for (int i = 0; i < p->sizep; i++) mark_lines(f, p->p[i]);
}

static int max_line_of(const Proto *p)
{
    int m = p->lastlinedefined;
    for (int pc = 0; pc < p->sizecode; pc++) {
        int line = luaG_getfuncline(p, pc);
        if (line > m) m = line;
    }
    for (int i = 0; i < p->sizep; i++) {
        int sub = max_line_of(p->p[i]);
        if (sub > m) m = sub;
    }
    return m;
}

void prts_coverage_register_chunk(prts_script *s, lua_State *L, const char *vpath, const char *mount_dir)
{
    if (!s->coverage) return;
    const LClosure *cl = clLvalue(s2v(L->top.p - 1));
    const Proto *p = cl->p;
    if (!s->cov_files.elem) prts_array_init(&s->cov_files, sizeof(prts_cov_file));
    if (find_file(s, vpath)) return;
    prts_cov_file f;
    f.vpath = prts_strdup(vpath);
    f.real_path = prts_path_join(mount_dir, vpath);
    f.max_line = max_line_of(p);
    f.executable = prts_calloc((size_t)f.max_line + 1, 1);
    f.hits = prts_calloc((size_t)f.max_line + 1, sizeof(uint32_t));
    mark_lines(&f, p);
    prts_array_push(&s->cov_files, &f);
}

void prts_coverage_hit(prts_script *s, lua_State *L, lua_Debug *ar)
{
    if (!lua_getinfo(L, "S", ar) || ar->source[0] != '@') return;
    prts_cov_file *f = find_file(s, ar->source + 1);
    if (f && ar->currentline > 0 && ar->currentline <= f->max_line) f->hits[ar->currentline]++;
}

bool prts_script_write_coverage(prts_script *s, const char *real_path)
{
    prts_str out = {0};
    for (size_t i = 0; i < s->cov_files.len; i++) {
        prts_cov_file *f = prts_array_at(&s->cov_files, i);
        int found = 0, hit = 0;
        prts_str_appendf(&out, "TN:\nSF:%s\n", f->real_path);
        for (int line = 1; line <= f->max_line; line++) {
            if (!f->executable[line]) continue;
            found++;
            if (f->hits[line]) hit++;
            prts_str_appendf(&out, "DA:%d,%u\n", line, f->hits[line]);
        }
        prts_str_appendf(&out, "LF:%d\nLH:%d\nend_of_record\n", found, hit);
    }
    bool ok = prts_fs_write_file(real_path, prts_str_cstr(&out), out.len);
    prts_str_free(&out);
    return ok;
}

void prts_coverage_free(prts_script *s)
{
    for (size_t i = 0; i < s->cov_files.len; i++) {
        prts_cov_file *f = prts_array_at(&s->cov_files, i);
        prts_free(f->vpath);
        prts_free(f->real_path);
        prts_free(f->executable);
        prts_free(f->hits);
    }
    prts_array_free(&s->cov_files);
}
