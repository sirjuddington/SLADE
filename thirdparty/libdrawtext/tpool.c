/* worker thread pool based on POSIX threads
 * author: John Tsiombikas <nuclear@member.fsf.org>
 * This code is public domain.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef USE_THREADS
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "tpool.h"

struct work_item {
	void *data;
	dtx_tpool_callback work, done;
	struct work_item *next;
};

struct dtx_thread_pool {
	pthread_t *threads;
	int num_threads;

	int qsize;
	struct work_item *workq, *workq_tail;
	pthread_mutex_t workq_mutex;
	pthread_cond_t workq_condvar;

	int nactive;	/* number of active workers (not sleeping) */

	pthread_cond_t done_condvar;

	int should_quit;
	int in_batch;
};

static void *thread_func(void *args);

struct dtx_thread_pool *dtx_tpool_create(int num_threads)
{
	int i;
	struct dtx_thread_pool *tpool;

	if(!(tpool = calloc(1, sizeof *tpool))) {
		return 0;
	}
	pthread_mutex_init(&tpool->workq_mutex, 0);
	pthread_cond_init(&tpool->workq_condvar, 0);
	pthread_cond_init(&tpool->done_condvar, 0);

	if(num_threads <= 0) {
		num_threads = dtx_tpool_num_processors();
	}
	tpool->num_threads = num_threads;

	if(!(tpool->threads = calloc(num_threads, sizeof *tpool->threads))) {
		free(tpool);
		return 0;
	}
	for(i=0; i<num_threads; i++) {
		if(pthread_create(tpool->threads + i, 0, thread_func, tpool) == -1) {
			tpool->threads[i] = 0;
			dtx_tpool_destroy(tpool);
			return 0;
		}
	}
	return tpool;
}

void dtx_tpool_destroy(struct dtx_thread_pool *tpool)
{
	int i;
	if(!tpool) return;

	dtx_tpool_clear(tpool);
	tpool->should_quit = 1;

	pthread_cond_broadcast(&tpool->workq_condvar);

	if(tpool->threads) {
		printf("dtx_thread_pool: waiting for %d worker threads to stop ", tpool->num_threads);
		fflush(stdout);

		for(i=0; i<tpool->num_threads; i++) {
			pthread_join(tpool->threads[i], 0);
			putchar('.');
			fflush(stdout);
		}
		putchar('\n');
		free(tpool->threads);
	}

	pthread_mutex_destroy(&tpool->workq_mutex);
	pthread_cond_destroy(&tpool->workq_condvar);
	pthread_cond_destroy(&tpool->done_condvar);
}

void dtx_tpool_begin_batch(struct dtx_thread_pool *tpool)
{
	tpool->in_batch = 1;
}

void dtx_tpool_end_batch(struct dtx_thread_pool *tpool)
{
	tpool->in_batch = 0;
	pthread_cond_broadcast(&tpool->workq_condvar);
}

int dtx_tpool_enqueue(struct dtx_thread_pool *tpool, void *data,
		dtx_tpool_callback work_func, dtx_tpool_callback done_func)
{
	struct work_item *job;

	if(!(job = malloc(sizeof *job))) {
		return -1;
	}
	job->work = work_func;
	job->done = done_func;
	job->data = data;
	job->next = 0;

	pthread_mutex_lock(&tpool->workq_mutex);
	if(tpool->workq) {
		tpool->workq_tail->next = job;
		tpool->workq_tail = job;
	} else {
		tpool->workq = tpool->workq_tail = job;
	}
	++tpool->qsize;
	pthread_mutex_unlock(&tpool->workq_mutex);

	if(!tpool->in_batch) {
		pthread_cond_broadcast(&tpool->workq_condvar);
	}
	return 0;
}

void dtx_tpool_clear(struct dtx_thread_pool *tpool)
{
	pthread_mutex_lock(&tpool->workq_mutex);
	while(tpool->workq) {
		void *tmp = tpool->workq;
		tpool->workq = tpool->workq->next;
		free(tmp);
	}
	tpool->workq = tpool->workq_tail = 0;
	tpool->qsize = 0;
	pthread_mutex_unlock(&tpool->workq_mutex);
}

int dtx_tpool_queued_jobs(struct dtx_thread_pool *tpool)
{
	int res;
	pthread_mutex_lock(&tpool->workq_mutex);
	res = tpool->qsize;
	pthread_mutex_unlock(&tpool->workq_mutex);
	return res;
}

int dtx_tpool_active_jobs(struct dtx_thread_pool *tpool)
{
	int res;
	pthread_mutex_lock(&tpool->workq_mutex);
	res = tpool->nactive;
	pthread_mutex_unlock(&tpool->workq_mutex);
	return res;
}

int dtx_tpool_pending_jobs(struct dtx_thread_pool *tpool)
{
	int res;
	pthread_mutex_lock(&tpool->workq_mutex);
	res = tpool->qsize + tpool->nactive;
	pthread_mutex_unlock(&tpool->workq_mutex);
	return res;
}

void dtx_tpool_wait(struct dtx_thread_pool *tpool)
{
	pthread_mutex_lock(&tpool->workq_mutex);
	while(tpool->nactive || tpool->qsize) {
		pthread_cond_wait(&tpool->done_condvar, &tpool->workq_mutex);
	}
	pthread_mutex_unlock(&tpool->workq_mutex);
}

void dtx_tpool_wait_one(struct dtx_thread_pool *tpool)
{
	int cur_pending;
	pthread_mutex_lock(&tpool->workq_mutex);
	cur_pending = tpool->qsize + tpool->nactive;
	if(cur_pending) {
		while(tpool->qsize + tpool->nactive >= cur_pending) {
			pthread_cond_wait(&tpool->done_condvar, &tpool->workq_mutex);
		}
	}
	pthread_mutex_unlock(&tpool->workq_mutex);
}

long dtx_tpool_timedwait(struct dtx_thread_pool *tpool, long timeout)
{
	long sec;
	struct timespec tout_ts;
	struct timeval tv0, tv;
	gettimeofday(&tv0, 0);

	sec = timeout / 1000;
	tout_ts.tv_nsec = tv0.tv_usec * 1000 + (timeout % 1000) * 1000000;
	tout_ts.tv_sec = tv0.tv_sec + sec;

	pthread_mutex_lock(&tpool->workq_mutex);
	while(tpool->nactive || tpool->qsize) {
		if(pthread_cond_timedwait(&tpool->done_condvar,
					&tpool->workq_mutex, &tout_ts) == ETIMEDOUT) {
			break;
		}
	}
	pthread_mutex_unlock(&tpool->workq_mutex);

	gettimeofday(&tv, 0);
	return (tv.tv_sec - tv0.tv_sec) * 1000 + (tv.tv_usec - tv0.tv_usec) / 1000;
}

static void *thread_func(void *args)
{
	struct dtx_thread_pool *tpool = args;

	pthread_mutex_lock(&tpool->workq_mutex);
	while(!tpool->should_quit) {
		pthread_cond_wait(&tpool->workq_condvar, &tpool->workq_mutex);

		while(!tpool->should_quit && tpool->workq) {
			/* grab the first job */
			struct work_item *job = tpool->workq;
			tpool->workq = tpool->workq->next;
			if(!tpool->workq)
				tpool->workq_tail = 0;
			++tpool->nactive;
			--tpool->qsize;
			pthread_mutex_unlock(&tpool->workq_mutex);

			/* do the job */
			job->work(job->data);
			if(job->done) {
				job->done(job->data);
			}

			pthread_mutex_lock(&tpool->workq_mutex);
			/* notify everyone interested that we're done with this job */
			pthread_cond_broadcast(&tpool->done_condvar);
			--tpool->nactive;
		}
	}
	pthread_mutex_unlock(&tpool->workq_mutex);

	return 0;
}
#endif	/* USE_THREADS */

/* The following highly platform-specific code detects the number
 * of processors available in the system. It's used by the thread pool
 * to autodetect how many threads to spawn.
 * Currently works on: Linux, BSD, Darwin, and Windows.
 */

#if defined(__APPLE__) && defined(__MACH__)
# ifndef __unix__
#  define __unix__	1
# endif	/* unix */
# ifndef __bsd__
#  define __bsd__	1
# endif	/* bsd */
#endif	/* apple */

#if defined(unix) || defined(__unix__)
#include <unistd.h>

# ifdef __bsd__
#  include <sys/sysctl.h>
# endif
#endif

#if defined(WIN32) || defined(__WIN32__)
#include <windows.h>
#endif


int dtx_tpool_num_processors(void)
{
#if defined(unix) || defined(__unix__)
# if defined(__bsd__)
	/* BSD systems provide the num.processors through sysctl */
	int num, mib[] = {CTL_HW, HW_NCPU};
	size_t len = sizeof num;

	sysctl(mib, 2, &num, &len, 0, 0);
	return num;

# elif defined(__sgi)
	/* SGI IRIX flavour of the _SC_NPROC_ONLN sysconf */
	return sysconf(_SC_NPROC_ONLN);
# else
	/* Linux (and others?) have the _SC_NPROCESSORS_ONLN sysconf */
	return sysconf(_SC_NPROCESSORS_ONLN);
# endif	/* bsd/sgi/other */

#elif defined(WIN32) || defined(__WIN32__)
	/* under windows we need to call GetSystemInfo */
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#endif
}
