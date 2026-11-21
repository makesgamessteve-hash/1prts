/* 1stPRTS engine: background jobs (asset decoding, mesh building, later pathfinding).
 * MIT License.
 *
 * A job has a `run` function, which may execute on a worker thread and must not touch
 * simulation, script or GPU state, and an optional `done` function, which always runs on the
 * main thread inside prts_jobs_apply_completed. Results therefore only become visible at
 * that defined point (once per frame in the engine), and `done` callbacks run in submission
 * order, whatever order the workers finish in.
 *
 * With zero threads (and always on single-threaded web builds) the system runs in inline
 * mode: prts_jobs_apply_completed first runs queued jobs on the main thread for at most one
 * time slice, then applies them. */
#ifndef PRTS_JOBS_H
#define PRTS_JOBS_H

#include "prts_base.h"

typedef struct prts_jobs prts_jobs;
typedef void (*prts_job_fn)(void *data);

/* Worker count for this machine: logical cores - 1 (1..8), or 0 where there are no threads. */
int prts_jobs_default_threads(void);
/* threads <= 0 (or no thread support) creates an inline job system. */
prts_jobs *prts_jobs_create(int threads);
/* Finishes and applies every submitted job, then stops the workers. */
void prts_jobs_destroy(prts_jobs *j);
int prts_jobs_threads(const prts_jobs *j);
/* Time budget for inline mode per apply call (default 2 ms). At least one job always runs. */
void prts_jobs_set_slice_ns(prts_jobs *j, uint64_t ns);

/* Queues a job and returns its sequence number (1, 2, ...). run must not be NULL. */
uint64_t prts_jobs_submit(prts_jobs *j, prts_job_fn run, prts_job_fn done, void *data);
/* Main thread: calls `done` for finished jobs in submission order, stopping at the first one
 * that has not finished yet. Returns how many jobs were applied. */
size_t prts_jobs_apply_completed(prts_jobs *j);
/* Blocks until every submitted job has run (inline mode runs them now). Does not apply. */
void prts_jobs_wait(prts_jobs *j);
/* wait + apply: a deterministic sync point. */
void prts_jobs_flush(prts_jobs *j);
/* Jobs submitted but not yet applied. */
size_t prts_jobs_pending(const prts_jobs *j);

#endif
