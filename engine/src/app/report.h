/* 1stPRTS engine: test result reports in JUnit XML format (for CI). MIT License. */
#ifndef PRTS_REPORT_H
#define PRTS_REPORT_H

#include "prts/prts_base.h"

typedef struct prts_report prts_report;

prts_report *prts_report_create(const char *suite);
void prts_report_destroy(prts_report *r);
/* A test case that takes a while (a scenario): begin it, then end it with its result. */
void prts_report_begin(prts_report *r, const char *classname, const char *name);
bool prts_report_is_open(const prts_report *r);
/* failure NULL: passed. Ends the open case. */
void prts_report_end(prts_report *r, double seconds, const char *failure);
/* Ends the open case with this failure, or adds a failed case if none is open. */
void prts_report_fail(prts_report *r, const char *classname, const char *name, const char *failure);
/* begin + end. */
void prts_report_case(prts_report *r, const char *classname, const char *name, double seconds, const char *failure);
size_t prts_report_count(const prts_report *r);
size_t prts_report_failures(const prts_report *r);
/* Writes <testsuites><testsuite><testcase/>...; creates parent folders. */
bool prts_report_write_junit(const prts_report *r, const char *real_path);

#endif
