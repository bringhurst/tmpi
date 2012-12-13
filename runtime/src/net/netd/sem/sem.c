# include "thread.h"
# include "sem.h"

int mysem_init(MYSEM *sem, int n)
{
	if (!sem)
		return -1;

	if (n<0)
		return -1;

	sem->waiters=sem->pendings=0;
	sem->value=n;
		
	if (thr_cnd_init(&(sem->wait)))
		return -1;

	if (thr_mtx_init(&(sem->mtx))) {
		thr_cnd_destroy(&(sem->wait));
		return -1;
	}

	return 0;
}

int mysem_destroy(MYSEM *sem)
{
	thr_cnd_destroy(&(sem->wait));
	thr_mtx_destroy(&(sem->mtx));
	
	return 0;
}

/**
 * P:
 * lock
 * if (nWaiter+nPending>0) then
 * 		nWaiter++;
 * 		wait;
 * 		nPending--;
 * end if
 * 
 * while (N==0) 
 * 		nWaiter++;
 * 		wait;
 * 		nPending--;
 * end while
 * 
 * N--;
 * 
 * unlock
 *
 * V:
 * lock
 * N++;
 * if (nWaiter>0) then
 * 		nPending++;
 * 		nWaiter--;
 * 		signal;
 * end if
 * unlock
 */

int mysem_P(MYSEM *sem)
{
	if (thr_mtx_lock(&(sem->mtx)))
		return -1;

	if ( (sem->waiters+sem->pendings)>0 ) {
		sem->waiters++;
		thr_cnd_wait(&(sem->wait), &(sem->mtx));
		sem->pendings--;
	}

	while (sem->value<=0) {
		sem->waiters++;
		thr_cnd_wait(&(sem->wait), &(sem->mtx));
		sem->pendings--;
	}

	sem->value--;

	thr_mtx_unlock(&(sem->mtx));

	return 0;
}

int mysem_V(MYSEM *sem)
{
	if (thr_mtx_lock(&(sem->mtx)))
		return -1;

	sem->value++;

	if (sem->waiters>0) {
		sem->waiters--;
		sem->pendings++;
		thr_cnd_signal(&(sem->wait));
	}

	thr_mtx_unlock(&(sem->mtx));

	return 0;
}
