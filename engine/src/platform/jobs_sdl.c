/* 1stPRTS engine: job system on SDL3 threads, with an inline mode for builds without threads.
 * MIT License. */
#include <SDL3/SDL.h>
#include <string.h>

#include "prts/prts_jobs.h"

typedef enum job_state { JOB_QUEUED = 0, JOB_RUNNING, JOB_FINISHED } job_state;

typedef struct job {
    prts_job_fn run, done;
    void *data;
    job_state state;
} job;

struct prts_jobs {
    SDL_Mutex *lock;          /* NULL in inline mode */
    SDL_Condition *work;      /* signalled when a job is queued or on shutdown */
    SDL_Condition *finished;  /* signalled when a job finishes */
    SDL_Thread **threads;
    int nthreads;
    job *jobs;                /* jobs[i] has sequence number base + i */
    size_t len, cap;
    uint64_t base;
    uint64_t next_run;        /* sequence number of the next queued job */
    size_t unfinished;
    bool quit;
    uint64_t slice_ns;
};

static void lock(prts_jobs *j)
{
    if (j->lock) SDL_LockMutex(j->lock);
}

static void unlock(prts_jobs *j)
{
    if (j->lock) SDL_UnlockMutex(j->lock);
}

int prts_jobs_default_threads(void)
{
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    return 0;
#else
    int n = SDL_GetNumLogicalCPUCores() - 1;
    return PRTS_CLAMP(n, 1, 8);
#endif
}

/* Takes the next queued job (lock held). */
static job take(prts_jobs *j, uint64_t *seq)
{
    *seq = j->next_run++;
    job *slot = &j->jobs[(size_t)(*seq - j->base)];
    slot->state = JOB_RUNNING;
    return *slot;
}

static bool has_queued(const prts_jobs *j) { return j->next_run < j->base + j->len; }

static void mark_finished(prts_jobs *j, uint64_t seq)
{
    j->jobs[(size_t)(seq - j->base)].state = JOB_FINISHED;
    j->unfinished--;
}

static int SDLCALL worker(void *data)
{
    prts_jobs *j = data;
    SDL_LockMutex(j->lock);
    for (;;) {
        while (!j->quit && !has_queued(j)) SDL_WaitCondition(j->work, j->lock);
        if (!has_queued(j)) break; /* quit with nothing left to do */
        uint64_t seq;
        job jb = take(j, &seq);
        SDL_UnlockMutex(j->lock);
        jb.run(jb.data);
        SDL_LockMutex(j->lock);
        mark_finished(j, seq);
        SDL_BroadcastCondition(j->finished);
    }
    SDL_UnlockMutex(j->lock);
    return 0;
}

prts_jobs *prts_jobs_create(int threads)
{
    prts_jobs *j = prts_calloc(1, sizeof(*j));
    j->base = 1;
    j->next_run = 1;
    j->slice_ns = 2000000u;
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    threads = 0;
#endif
    if (threads > 0) {
        j->lock = SDL_CreateMutex();
        j->work = SDL_CreateCondition();
        j->finished = SDL_CreateCondition();
        j->threads = prts_calloc((size_t)threads, sizeof(SDL_Thread *));
        for (int i = 0; i < threads; i++) {
            SDL_Thread *t = SDL_CreateThread(worker, "prts_job", j);
            if (!t) break; /* fewer workers (or inline mode) if the platform refuses */
            j->threads[j->nthreads++] = t;
        }
    }
    return j;
}

int prts_jobs_threads(const prts_jobs *j) { return j->nthreads; }

void prts_jobs_set_slice_ns(prts_jobs *j, uint64_t ns) { j->slice_ns = ns; }

uint64_t prts_jobs_submit(prts_jobs *j, prts_job_fn run, prts_job_fn done, void *data)
{
    lock(j);
    if (j->len == j->cap) {
        j->cap = j->cap ? j->cap * 2u : 16u;
        j->jobs = prts_realloc(j->jobs, j->cap * sizeof(job));
    }
    job jb = {run, done, data, JOB_QUEUED};
    j->jobs[j->len++] = jb;
    j->unfinished++;
    uint64_t seq = j->base + j->len - 1u;
    if (j->nthreads) SDL_SignalCondition(j->work);
    unlock(j);
    return seq;
}

/* Inline mode: runs one queued job on this thread. */
static void run_one_inline(prts_jobs *j)
{
    uint64_t seq;
    job jb = take(j, &seq);
    jb.run(jb.data);
    mark_finished(j, seq);
}

void prts_jobs_wait(prts_jobs *j)
{
    if (!j->nthreads) {
        while (has_queued(j)) run_one_inline(j);
        return;
    }
    SDL_LockMutex(j->lock);
    while (j->unfinished > 0) SDL_WaitCondition(j->finished, j->lock);
    SDL_UnlockMutex(j->lock);
}

size_t prts_jobs_apply_completed(prts_jobs *j)
{
    if (!j->nthreads && has_queued(j)) {
        uint64_t start = SDL_GetTicksNS();
        do {
            run_one_inline(j);
        } while (has_queued(j) && SDL_GetTicksNS() - start < j->slice_ns);
    }
    lock(j);
    size_t n = 0;
    while (n < j->len && j->jobs[n].state == JOB_FINISHED) n++;
    job *ready = NULL;
    if (n) {
        ready = prts_malloc(n * sizeof(job));
        memcpy(ready, j->jobs, n * sizeof(job));
        memmove(j->jobs, j->jobs + n, (j->len - n) * sizeof(job));
        j->len -= n;
        j->base += n;
    }
    unlock(j);
    /* callbacks run without the lock, so they may submit more jobs */
    for (size_t i = 0; i < n; i++) {
        if (ready[i].done) ready[i].done(ready[i].data);
    }
    prts_free(ready);
    return n;
}

void prts_jobs_flush(prts_jobs *j)
{
    while (prts_jobs_pending(j) > 0) {
        prts_jobs_wait(j);
        prts_jobs_apply_completed(j);
    }
}

size_t prts_jobs_pending(const prts_jobs *j) { return j->len; }

void prts_jobs_destroy(prts_jobs *j)
{
    if (!j) return;
    prts_jobs_flush(j);
    if (j->lock) {
        SDL_LockMutex(j->lock);
        j->quit = true;
        SDL_BroadcastCondition(j->work);
        SDL_UnlockMutex(j->lock);
        for (int i = 0; i < j->nthreads; i++) SDL_WaitThread(j->threads[i], NULL);
        SDL_DestroyCondition(j->work);
        SDL_DestroyCondition(j->finished);
        SDL_DestroyMutex(j->lock);
    }
    prts_free(j->threads);
    prts_free(j->jobs);
    prts_free(j);
}
