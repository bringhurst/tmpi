# include <stdlib.h>
# include <sys/time.h>
# include <unistd.h>
# include "../thread.h"
# include "tmpi_debug.h"
# include "ukey.h"

static int max_threads=THR_MAX_THREADS;
struct thr_parm {
	pthread_t thr;
	int id;
	void *arg;
	int (*f)(void*);
	void *ret;
};

static struct thr_parm *threads=NULL;
static int thr_idx=0;

int thr_init(int max)
{
	if (threads) {
		tmpi_error(DBG_INTERNAL, "thr_init has already been called");
		return -1;
	}

	if (max<THR_MAX_THREADS) {
		/* tmpi_error(DBG_CALLER, "thr_init max thread number less than default."); */
		max=THR_MAX_THREADS;
	}
	max_threads=max;

	threads=(struct thr_parm *)calloc(max_threads, sizeof(struct thr_parm));
	return 0;
}

int thr_end()
{
	return 0;
}

int thr_getMaxThreads()
{
	return max_threads;
}

/** considering pthread is already kernel threads, I don't need binded flag **/
thr_t thr_createbatch(int (*func)(void *arg), int id, int binded, void *arg)
{
	/* probably I need to use fetch_and_add to make it multithread safe */
	int i;

	if (!threads) {
		tmpi_error(DBG_INTERNAL, "thr_init has not been called");
		return NULL;
	}

	i=thr_idx++; 
	threads[i].f=func;
	threads[i].id=id;
	threads[i].arg=arg;
	return &(threads[i].thr);
}

/**
 * return THREAD_ERROR on error, 
 */
# define THREAD_ERROR ((thr_t)NULL)

thr_t thr_createone(void * (*func)(void *arg), void *arg)
{
	thr_t thr=(pthread_t *)malloc(sizeof(pthread_t));
	int ret;

	ret=pthread_create(thr, NULL, (void *)func, arg);
	if (ret==0)
		return thr; /* this works because I know pthread_t is primitive type. */
	else
		return THREAD_ERROR;
}

/**
 * can only be applied to stand alone threads.
 */
int thr_joinone(thr_t thread, void **ret)
{
	if (thread)
		return pthread_join(*thread, ret);
	else
		return -1;
}
	
void *thr_stub(void *arg)
{
	struct thr_parm *parm=arg;
	ukey_setid(parm->id);
	parm->ret=(void *)(*(parm->f))(parm->arg);
	return &(parm->ret);
}

int thr_start(int nvp)
{
	int i=0;
	int ret=0;
	
	for (; i<thr_idx; i++) {
		pthread_create(&(threads[i].thr), NULL, thr_stub, (void *)&(threads[i]));
	}

	for (i=0; i<thr_idx; i++) {
		pthread_join(threads[i].thr, &(threads[i].ret));
		ret+=(int)(threads[i].ret);
	}

	return ret;
}

/**
 * Mutex Lock
 */
int thr_mtx_init(thr_mtx_t *mtx)
{
	return pthread_mutex_init(mtx, NULL);
}

int thr_mtx_destroy(thr_mtx_t *mtx)
{
	return pthread_mutex_destroy(mtx);
}

int thr_mtx_lock(thr_mtx_t *mtx)
{
	return pthread_mutex_lock(mtx);
}

int thr_mtx_unlock(thr_mtx_t *mtx)
{
	return pthread_mutex_unlock(mtx);
}

int thr_mtx_trylock(thr_mtx_t *mtx)
{
	return pthread_mutex_trylock(mtx);
}

/**
 * Conditional variables
 */
int thr_cnd_init(thr_cnd_t *cnd)
{
	return pthread_cond_init(cnd, NULL);
}

int thr_cnd_destroy(thr_cnd_t *cnd)
{
	return pthread_cond_destroy(cnd);
}

int thr_cnd_wait(thr_cnd_t *cnd, thr_mtx_t *mtx)
{
	return pthread_cond_wait(cnd, mtx);
}

/**
 * Not like pthread_timedwait, time_in_us describe approximately,
 * how long the caller wants to wait for the condition variable 
 * before it times out. It is not changed by thr_cnd_timedwait().
 * So one can construct once and reuse it for subsequent calls.
 */
int thr_cnd_timedwait(thr_cnd_t *cnd, thr_mtx_t *mtx, int time_in_us)
{
	struct timespec span;
	struct timeval now;
	long long total_in_us;

	gettimeofday(&now, NULL);
	total_in_us=now.tv_sec*1000000+now.tv_usec+time_in_us;
	span.tv_sec = total_in_us / 1000000;
	span.tv_nsec = (total_in_us % 1000000) * 1000;
	return pthread_cond_timedwait(cnd, mtx, &span);
}

int thr_cnd_signal(thr_cnd_t *cnd)
{
	return pthread_cond_signal(cnd);
}

int thr_cnd_bcast(thr_cnd_t *cnd)
{
	return pthread_cond_broadcast(cnd);
}

/**
 * Aboart execution
 */
void thr_exit(int exitcode)
{
	pthread_exit((void *)exitcode);
}
