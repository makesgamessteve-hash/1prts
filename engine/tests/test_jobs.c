/* Unit tests: job system (worker threads and inline mode). MIT License. */
#include <string.h>

#include "prts/prts_jobs.h"
#include "utest.h"

#define NJOBS 64

typedef struct job_log {
    int ran[NJOBS * 2];
    int nran;
    int applied[NJOBS * 2];
    int napplied;
    prts_jobs *jobs;
} job_log;

typedef struct test_job {
    int id;
    int spin;
    int result; /* written by run (worker), read by done (main thread) */
    job_log *log;
} test_job;

static void job_run(void *data)
{
    test_job *t = data;
    volatile int x = 0;
    for (int i = 0; i < t->spin; i++) x += i;
    t->result = t->id * 10;
}

static void job_run_logged(void *data)
{
    test_job *t = data;
    job_run(data);
    t->log->ran[t->log->nran++] = t->id; /* inline mode only: runs on the main thread */
}

static void job_done(void *data)
{
    test_job *t = data;
    t->log->applied[t->log->napplied++] = t->result / 10;
}

UTEST(jobs, inline_mode_runs_in_slices_and_applies_in_order)
{
    job_log log;
    memset(&log, 0, sizeof(log));
    prts_jobs *j = prts_jobs_create(0);
    ASSERT_EQ(0, prts_jobs_threads(j));
    test_job jobs[5];
    for (int i = 0; i < 5; i++) {
        jobs[i] = (test_job){i, 10, 0, &log};
        ASSERT_EQ((uint64_t)(i + 1), prts_jobs_submit(j, job_run_logged, job_done, &jobs[i]));
    }
    /* nothing runs or appears before the apply point */
    ASSERT_EQ((size_t)5, prts_jobs_pending(j));
    ASSERT_EQ(0, log.nran);
    /* a zero slice still runs one job per apply, so work always progresses */
    prts_jobs_set_slice_ns(j, 0);
    ASSERT_EQ((size_t)1, prts_jobs_apply_completed(j));
    ASSERT_EQ(1, log.nran);
    ASSERT_EQ(1, log.napplied);
    prts_jobs_set_slice_ns(j, 1000000000u);
    ASSERT_EQ((size_t)4, prts_jobs_apply_completed(j));
    ASSERT_EQ((size_t)0, prts_jobs_apply_completed(j));
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(i, log.ran[i]);
        ASSERT_EQ(i, log.applied[i]);
    }
    /* wait runs everything now but applies nothing */
    for (int i = 0; i < 3; i++) prts_jobs_submit(j, job_run_logged, job_done, &jobs[i]);
    prts_jobs_wait(j);
    ASSERT_EQ(8, log.nran);
    ASSERT_EQ(5, log.napplied);
    ASSERT_EQ((size_t)3, prts_jobs_pending(j));
    ASSERT_EQ((size_t)3, prts_jobs_apply_completed(j));
    ASSERT_EQ(8, log.napplied);
    prts_jobs_destroy(j);
    prts_jobs_destroy(NULL);
}

UTEST(jobs, worker_results_appear_only_at_apply_points_in_submission_order)
{
    ASSERT_TRUE(prts_jobs_default_threads() >= 1);
    job_log log;
    memset(&log, 0, sizeof(log));
    prts_jobs *j = prts_jobs_create(4);
    ASSERT_EQ(4, prts_jobs_threads(j));
    static test_job jobs[NJOBS];
    for (int i = 0; i < NJOBS; i++) {
        /* early jobs take longest, so workers finish out of order */
        jobs[i] = (test_job){i, (NJOBS - i) * 2000, 0, &log};
        prts_jobs_submit(j, job_run, job_done, &jobs[i]);
    }
    prts_jobs_wait(j);
    for (int i = 0; i < NJOBS; i++) ASSERT_EQ(i * 10, jobs[i].result);
    /* every job has run, but no result was applied yet */
    ASSERT_EQ(0, log.napplied);
    ASSERT_EQ((size_t)NJOBS, prts_jobs_pending(j));
    ASSERT_EQ((size_t)NJOBS, prts_jobs_apply_completed(j));
    for (int i = 0; i < NJOBS; i++) ASSERT_EQ(i, log.applied[i]);
    ASSERT_EQ((size_t)0, prts_jobs_pending(j));
    prts_jobs_destroy(j);
}

static void chain_done(void *data)
{
    test_job *t = data;
    job_done(data);
    if (t->id < 3) {
        t->id++;
        prts_jobs_submit(t->log->jobs, job_run, chain_done, t);
    }
}

UTEST(jobs, done_callbacks_may_submit_and_destroy_finishes_everything)
{
    for (int threads = 0; threads <= 2; threads += 2) {
        job_log log;
        memset(&log, 0, sizeof(log));
        prts_jobs *j = prts_jobs_create(threads);
        log.jobs = j;
        test_job chain = {0, 100, 0, &log};
        prts_jobs_submit(j, job_run, chain_done, &chain);
        prts_jobs_flush(j);
        ASSERT_EQ(4, log.napplied);
        ASSERT_EQ(3, log.applied[3]);
        /* jobs without a done callback, still queued when the system is destroyed */
        test_job quiet[3];
        for (int i = 0; i < 3; i++) {
            quiet[i] = (test_job){i + 1, 1000, 0, &log};
            prts_jobs_submit(j, job_run, NULL, &quiet[i]);
        }
        test_job last = {9, 1000, 0, &log};
        prts_jobs_submit(j, job_run, job_done, &last);
        prts_jobs_destroy(j);
        ASSERT_EQ(30, quiet[2].result);
        ASSERT_EQ(5, log.napplied);
        ASSERT_EQ(9, log.applied[4]);
    }
}
