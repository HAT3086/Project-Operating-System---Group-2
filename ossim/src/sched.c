/*
 * sched.c: CPU schedulers.
 *
 * FCFS and RR are given. Checkpoint 1 adds SJF, SRTF, PRIO and MLFQ.
 * Each one is a scheduler_t with the same hooks as FCFS and RR. There are
 * notes above each stub. Functions you will probably need:
 *   proc_burst_remaining(p)   ticks left in p's current cpu burst
 *   proc_total_remaining(p)   cpu ticks left in the whole process
 *   q_pop_min(&q, key)        remove the element with the smallest key
 *   q_min_key(&q, key)        the smallest key, without removing (for preemption)
 */
#include <string.h>
#include "sched.h"
#include "queue.h"

scheduler_t *sched = NULL;

/* FCFS */
static queue_t fcfs_q;

static void   fcfs_init(void)                { q_init(&fcfs_q); }
static void   fcfs_enqueue(pcb_t *p)         { q_push(&fcfs_q, p); }
static bool   fcfs_should_preempt(pcb_t *r)  { (void)r; return false; }
static pcb_t *fcfs_pick(void)                { return q_pop(&fcfs_q); }
static void   fcfs_on_dispatch(pcb_t *p)     { (void)p; }
static void   fcfs_on_tick(pcb_t *r)         { (void)r; }
static int    fcfs_ready_count(void)         { return q_size(&fcfs_q); }

static scheduler_t sched_fcfs = {
    "fcfs", fcfs_init, fcfs_enqueue, fcfs_should_preempt, fcfs_pick,
    fcfs_on_dispatch, fcfs_on_tick, fcfs_ready_count
};

/* RR */
static queue_t rr_q;

static void   rr_init(void)            { q_init(&rr_q); }
static void   rr_enqueue(pcb_t *p)     { q_push(&rr_q, p); }
static pcb_t *rr_pick(void)            { return q_pop(&rr_q); }
static void   rr_on_dispatch(pcb_t *p) { p->quantum_used = 0; }
static void   rr_on_tick(pcb_t *r)     { r->quantum_used++; }
static int    rr_ready_count(void)     { return q_size(&rr_q); }

/* Preempt when the quantum is used up and someone else is ready. If nobody
 * is ready, the process keeps the CPU and starts a new quantum, so a process
 * that arrives later waits until that quantum is over. */
static bool rr_should_preempt(pcb_t *r)
{
    if (r->quantum_used < cfg.quantum) return false;
    r->quantum_used = 0;
    return !q_empty(&rr_q);
}

static scheduler_t sched_rr = {
    "rr", rr_init, rr_enqueue, rr_should_preempt, rr_pick,
    rr_on_dispatch, rr_on_tick, rr_ready_count
};

/* SJF: a FIFO list with a stable minimum-burst selection. Exact burst
 * lengths are available in this simulator; a real OS could predict them
 * with tau_next = alpha * actual + (1 - alpha) * tau_previous. */
static queue_t sjf_q;
static void   sjf_init(void)               { q_init(&sjf_q); }
static void   sjf_enqueue(pcb_t *p)        { q_push(&sjf_q, p); }
static bool   sjf_should_preempt(pcb_t *r) { (void)r; return false; }
static pcb_t *sjf_pick(void)               { return q_pop_min(&sjf_q, proc_burst_remaining); }
static void   sjf_on_dispatch(pcb_t *p)    { (void)p; }
static void   sjf_on_tick(pcb_t *r)        { (void)r; }
static int    sjf_ready_count(void)        { return q_size(&sjf_q); }
static scheduler_t sched_sjf = {
    "sjf", sjf_init, sjf_enqueue, sjf_should_preempt, sjf_pick,
    sjf_on_dispatch, sjf_on_tick, sjf_ready_count
};

/* SRTF: a strictly shorter READY burst preempts; an equal remaining
 * length leaves the running process on the CPU. Selection ties are FIFO. */
static queue_t srtf_q;
static void   srtf_init(void)        { q_init(&srtf_q); }
static void   srtf_enqueue(pcb_t *p) { q_push(&srtf_q, p); }
static pcb_t *srtf_pick(void)        { return q_pop_min(&srtf_q, proc_burst_remaining); }
static int    srtf_ready_count(void) { return q_size(&srtf_q); }
static bool srtf_should_preempt(pcb_t *r)
{
    return !q_empty(&srtf_q) &&
           q_min_key(&srtf_q, proc_burst_remaining) < proc_burst_remaining(r);
}
static scheduler_t sched_srtf = {
    "srtf", srtf_init, srtf_enqueue, srtf_should_preempt, srtf_pick,
    sjf_on_dispatch, sjf_on_tick, srtf_ready_count
};

/* PRIO: smallest priority first, stable FIFO on equal priorities.
 * Higher priorities preempt immediately; equal priorities rotate at q.
 * A dispatch (including after higher-priority preemption) starts a fresh
 * quantum. Lower-priority contenders never force a quantum rotation. */
static queue_t prio_q;
static int    priority_key(const pcb_t *p) { return p->priority; }
static void   prio_init(void)              { q_init(&prio_q); }
static void   prio_enqueue(pcb_t *p)       { q_push(&prio_q, p); }
static pcb_t *prio_pick(void)              { return q_pop_min(&prio_q, priority_key); }
static int    prio_ready_count(void)       { return q_size(&prio_q); }
static bool prio_should_preempt(pcb_t *r)
{
    if (!q_empty(&prio_q) && q_min_key(&prio_q, priority_key) < r->priority)
        return true;
    if (r->quantum_used < cfg.quantum) return false;
    r->quantum_used = 0;
    return !q_empty(&prio_q) && q_min_key(&prio_q, priority_key) == r->priority;
}
static scheduler_t sched_prio = {
    "prio", prio_init, prio_enqueue, prio_should_preempt, prio_pick,
    rr_on_dispatch, rr_on_tick, prio_ready_count
};

/* MLFQ: three FIFO queues, level 0 highest, budgets q/2q/4q.
 * Exhaustion is processed in on_tick: a process that blocks exactly at
 * its quantum boundary still moves down (level 2 stays at level 2).
 * Higher-level preemption preserves the unused part of a quantum; an
 * early block starts a fresh quantum at the same level on waking.
 * All arrivals, wakeups and preemptions enqueue at the tail.
 * Boosts at t=20,40,... occur after events, before selection. READY queues
 * merge in old level order, FIFO within each. The running process joins
 * their tail if anyone is READY. Blocked/new processes are reset too. */
static queue_t mlfq_q[3];
static bool mlfq_expired[MAX_PROCS + 1];
static bool mlfq_interrupted[MAX_PROCS + 1];
static tick_t mlfq_last_boost;

static void mlfq_init(void)
{
    for (int level = 0; level < 3; level++) q_init(&mlfq_q[level]);
    memset(mlfq_expired, 0, sizeof(mlfq_expired));
    memset(mlfq_interrupted, 0, sizeof(mlfq_interrupted));
    mlfq_last_boost = -1;
    for (int i = 0; i < nprocs; i++) {
        procs[i].level = 0;
        procs[i].quantum_used = 0;
    }
}

static int mlfq_ready_count(void)
{
    return q_size(&mlfq_q[0]) + q_size(&mlfq_q[1]) + q_size(&mlfq_q[2]);
}

static bool mlfq_boost(void)
{
    if (now <= 0 || now % 20 != 0 || mlfq_last_boost == now) return false;
    mlfq_last_boost = now;
    for (int level = 1; level < 3; level++) {
        pcb_t *p;
        while ((p = q_pop(&mlfq_q[level])) != NULL) q_push(&mlfq_q[0], p);
    }
    for (int i = 0; i < nprocs; i++) {
        procs[i].level = 0;
        procs[i].quantum_used = 0;
    }
    memset(mlfq_expired, 0, sizeof(mlfq_expired));
    memset(mlfq_interrupted, 0, sizeof(mlfq_interrupted));
    trace("MLFQ boost: all processes to level 0");
    return true;
}

static void mlfq_enqueue(pcb_t *p)
{
    if (!mlfq_interrupted[p->pid]) p->quantum_used = 0;
    mlfq_interrupted[p->pid] = false;
    mlfq_expired[p->pid] = false;
    q_push(&mlfq_q[p->level], p);
}

static pcb_t *mlfq_pick(void)
{
    mlfq_boost();                 /* also runs while the CPU is idle */
    for (int level = 0; level < 3; level++)
        if (!q_empty(&mlfq_q[level])) return q_pop(&mlfq_q[level]);
    return NULL;
}

static bool mlfq_should_preempt(pcb_t *r)
{
    if (mlfq_boost()) return mlfq_ready_count() > 0;
    bool exhausted = mlfq_expired[r->pid];
    mlfq_expired[r->pid] = false;
    for (int level = 0; level <= r->level; level++) {
        if (!q_empty(&mlfq_q[level]) && (level < r->level || exhausted)) {
            mlfq_interrupted[r->pid] = !exhausted;
            return true;
        }
    }
    return false;
}

static void mlfq_on_tick(pcb_t *r)
{
    if (++r->quantum_used >= (long long)cfg.quantum * (1 << r->level)) {
        if (r->level < 2) r->level++;
        r->quantum_used = 0;
        mlfq_expired[r->pid] = true;
        trace("MLFQ P%d exhausted quantum, level %d", r->pid, r->level);
    }
}
static scheduler_t sched_mlfq = {
    "mlfq", mlfq_init, mlfq_enqueue, mlfq_should_preempt, mlfq_pick,
    sjf_on_dispatch, mlfq_on_tick, mlfq_ready_count
};

static scheduler_t *all[] = {
    &sched_fcfs, &sched_rr, &sched_sjf, &sched_srtf, &sched_prio, &sched_mlfq
};

void sched_select(const char *name)
{
    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
        if (strcmp(all[i]->name, name) == 0) {
            sched = all[i];
            sched->init();
            return;
        }
    }
    die("unknown scheduler '%s' (fcfs, rr, sjf, srtf, prio, mlfq)", name);
}
