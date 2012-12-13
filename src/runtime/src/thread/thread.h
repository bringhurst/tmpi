# ifndef _THREAD_H_
# define _THREAD_H_
# include "thr_def.h"

/**
 * Initialization and termination
 */
/**
 * maximum user threads allowed
 */

# define THR_MAX_THREADS 1024
int thr_init(int max_threads);
int thr_end();

/**
 * Return the maximum threads that can be created by
 * using thr_createbatch. 
 */
int thr_getMaxThreads();
/**
 * Create a thread without actually running it.
 * id is the user level identifier of the
 * thread. binded thread will always be binded
 * to a kernel thread. This is just a hint.
 */
thr_t thr_createbatch(int (*func)(void *arg), int id, int binded, void *arg);

/**
 * running the threads threads created in batch mode 
 * on nvp vitrual processors. Won't terminate until 
 * all batch threads finish their execution.
 */
int thr_start(int nvp);

/**
 * create a standalone thread. always binded.
 */
thr_t thr_createone(void * (*func)(void *arg), void *arg);
int thr_joinone(thr_t thread, void **arg);

/**
 * Mutex Lock
 */
int thr_mtx_init(thr_mtx_t *mtx);
int thr_mtx_destroy(thr_mtx_t *mtx);
int thr_mtx_lock(thr_mtx_t *mtx);
int thr_mtx_unlock(thr_mtx_t *mtx);
int thr_mtx_trylock(thr_mtx_t *mtx);

/**
 * Conditional variables
 */
int thr_cnd_init(thr_cnd_t *cnd);
int thr_cnd_destroy(thr_cnd_t *cnd);
int thr_cnd_wait(thr_cnd_t *cnd, thr_mtx_t *mtx);
int thr_cnd_timedwait(thr_cnd_t *cnd, thr_mtx_t *mtx, int time_in_us);
int thr_cnd_signal(thr_cnd_t *cnd);
int thr_cnd_bcast(thr_cnd_t *cnd);

/**
 * Aboart execution
 */
void thr_exit(int exitcode);

/**
 * Thread specific data
 */
# include "ukey.h"

# endif
