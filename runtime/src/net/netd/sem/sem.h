# ifndef _SEM_H_
# define _SEM_H_

# include "thread.h"

typedef struct _mysem {
	thr_cnd_t wait;
	thr_mtx_t mtx;
	int waiters;
	int pendings;
	int value;
} MYSEM;

int mysem_init(MYSEM *sem, int n);
int mysem_destroy(MYSEM *sem);

int mysem_P(MYSEM *sem);
int mysem_V(MYSEM *sem);

# endif

