/* 1stPRTS engine: test result reports in JUnit XML format. MIT License. */
#include "app/report.h"

#include <string.h>

#include "prts/prts_fs.h"

typedef struct report_case {
    char *classname;
    char *name;
    double seconds;
    char *failure; /* NULL: passed */
} report_case;

struct prts_report {
    char *suite;
    prts_array cases; /* report_case */
    bool open;
};

prts_report *prts_report_create(const char *suite)
{
    prts_report *r = prts_calloc(1, sizeof(*r));
    r->suite = prts_strdup(suite);
    prts_array_init(&r->cases, sizeof(report_case));
    return r;
}

void prts_report_destroy(prts_report *r)
{
    if (!r) return;
    for (size_t i = 0; i < r->cases.len; i++) {
        report_case *c = prts_array_at(&r->cases, i);
        prts_free(c->classname);
        prts_free(c->name);
        prts_free(c->failure);
    }
    prts_array_free(&r->cases);
    prts_free(r->suite);
    prts_free(r);
}

void prts_report_begin(prts_report *r, const char *classname, const char *name)
{
    if (r->open) prts_report_end(r, 0.0, "a new test case started before this one ended");
    report_case c = {prts_strdup(classname), prts_strdup(name), 0.0, NULL};
    prts_array_push(&r->cases, &c);
    r->open = true;
}

bool prts_report_is_open(const prts_report *r) { return r->open; }

void prts_report_end(prts_report *r, double seconds, const char *failure)
{
    if (!r->open) return;
    report_case *c = prts_array_at(&r->cases, r->cases.len - 1u);
    c->seconds = seconds;
    c->failure = failure ? prts_strdup(failure) : NULL;
    r->open = false;
}

void prts_report_fail(prts_report *r, const char *classname, const char *name, const char *failure)
{
    if (!r->open) prts_report_begin(r, classname, name);
    prts_report_end(r, 0.0, failure);
}

void prts_report_case(prts_report *r, const char *classname, const char *name, double seconds, const char *failure)
{
    prts_report_begin(r, classname, name);
    prts_report_end(r, seconds, failure);
}

size_t prts_report_count(const prts_report *r) { return r->cases.len; }

size_t prts_report_failures(const prts_report *r)
{
    size_t n = 0;
    for (size_t i = 0; i < r->cases.len; i++) {
        if (((report_case *)prts_array_at(&r->cases, i))->failure) n++;
    }
    return n;
}

/* XML text: escapes markup characters and drops control characters XML 1.0 forbids. With
 * first_line, stops at the first line break (for the message attribute). */
static void append_xml(prts_str *out, const char *text, bool first_line)
{
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        unsigned char c = *p;
        if (first_line && (c == '\n' || c == '\r')) break;
        switch (c) {
        case '&': prts_str_append(out, "&amp;"); break;
        case '<': prts_str_append(out, "&lt;"); break;
        case '>': prts_str_append(out, "&gt;"); break;
        case '"': prts_str_append(out, "&quot;"); break;
        case '\'': prts_str_append(out, "&apos;"); break;
        default:
            if (c < 0x20 && c != '\n' && c != '\r' && c != '\t') prts_str_append(out, "?");
            else prts_str_append_n(out, (const char *)p, 1);
        }
    }
}

bool prts_report_write_junit(const prts_report *r, const char *real_path)
{
    prts_str x = {0};
    size_t failures = prts_report_failures(r);
    double total = 0.0;
    for (size_t i = 0; i < r->cases.len; i++) total += ((report_case *)prts_array_at(&r->cases, i))->seconds;
    prts_str_append(&x, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites name=\"1stprts\" tests=\"");
    prts_str_appendf(&x, "%zu\" failures=\"%zu\" time=\"%.3f\">\n  <testsuite name=\"", r->cases.len, failures, total);
    append_xml(&x, r->suite, false);
    prts_str_appendf(&x, "\" tests=\"%zu\" failures=\"%zu\" errors=\"0\" skipped=\"0\" time=\"%.3f\">\n", r->cases.len,
                     failures, total);
    for (size_t i = 0; i < r->cases.len; i++) {
        const report_case *c = prts_array_at(&r->cases, i);
        prts_str_append(&x, "    <testcase classname=\"");
        append_xml(&x, c->classname, false);
        prts_str_append(&x, "\" name=\"");
        append_xml(&x, c->name, false);
        prts_str_appendf(&x, "\" time=\"%.3f\"", c->seconds);
        if (!c->failure) {
            prts_str_append(&x, "/>\n");
            continue;
        }
        prts_str_append(&x, ">\n      <failure message=\"");
        append_xml(&x, c->failure, true);
        prts_str_append(&x, "\">");
        append_xml(&x, c->failure, false);
        prts_str_append(&x, "</failure>\n    </testcase>\n");
    }
    prts_str_append(&x, "  </testsuite>\n</testsuites>\n");
    bool ok = prts_fs_write_file(real_path, x.data, x.len);
    prts_str_free(&x);
    return ok;
}
