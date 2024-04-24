/* worker thread pool based on POSIX threads
 * author: John Tsiombikas <nuclear@member.fsf.org>
 * This code is public domain.
 */
#ifndef THREADPOOL_H_
#define THREADPOOL_H_

struct dtx_thread_pool;

/* type of the function accepted as work or completion callback */
typedef void (*dtx_tpool_callback)(void*);

#ifdef __cplusplus
extern "C" {
#endif

/* if num_threads == 0, auto-detect how many threads to spawn */
struct dtx_thread_pool *dtx_tpool_create(int num_threads);
void dtx_tpool_destroy(struct dtx_thread_pool *tpool);

/* if begin_batch is called before an enqueue, the worker threads will not be
 * signalled to start working until end_batch is called.
 */
void dtx_tpool_begin_batch(struct dtx_thread_pool *tpool);
void dtx_tpool_end_batch(struct dtx_thread_pool *tpool);

/* if enqueue is called without calling begin_batch first, it will immediately
 * wake up the worker threads to start working on the enqueued item
 */
int dtx_tpool_enqueue(struct dtx_thread_pool *tpool, void *data,
		dtx_tpool_callback work_func, dtx_tpool_callback done_func);
/* clear the work queue. does not cancel any currently running jobs */
void dtx_tpool_clear(struct dtx_thread_pool *tpool);

/* returns the number of queued work items */
int dtx_tpool_queued_jobs(struct dtx_thread_pool *tpool);
/* returns the number of active (working) threads */
int dtx_tpool_active_jobs(struct dtx_thread_pool *tpool);
/* returns the number of pending jobs, both in queue and active */
int dtx_tpool_pending_jobs(struct dtx_thread_pool *tpool);

/* wait for all pending jobs to be completed */
void dtx_tpool_wait(struct dtx_thread_pool *tpool);
/* wait until the pending jobs are down to the target specified
 * for example, to wait until a single job has been completed:
 *   dtx_tpool_wait_pending(tpool, dtx_tpool_pending_jobs(tpool) - 1);
 * this interface is slightly awkward to avoid race conditions. */
void dtx_tpool_wait_pending(struct dtx_thread_pool *tpool, int pending_target);
/* wait for all pending jobs to be completed for up to "timeout" milliseconds */
long dtx_tpool_timedwait(struct dtx_thread_pool *tpool, long timeout);

/* returns the number of processors on the system.
 * individual cores in multi-core processors are counted as processors.
 */
int dtx_tpool_num_processors(void);

#ifdef __cplusplus
}
#endif

#endif	/* THREADPOOL_H_ */
