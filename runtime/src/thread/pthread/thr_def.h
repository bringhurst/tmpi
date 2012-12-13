# ifndef _THR_DEF_H_
# define _THR_DEF_H_
# include <pthread.h>
typedef pthread_t * thr_t;

typedef pthread_mutex_t thr_mtx_t;
typedef pthread_cond_t thr_cnd_t;
# endif
