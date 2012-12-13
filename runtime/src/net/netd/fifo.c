# include <stdlib.h>
# include "fifo.h"

FIFO *fifo_create(int len)
{
	FIFO *q=(FIFO *)calloc(1, sizeof(FIFO));

	if (!q)
		return NULL;

	if (len<FIFO_LEN_DEFAULT)
		len=FIFO_LEN_DEFAULT;

	q->queue=(void **)calloc(len, sizeof(void *));
	if (!q->queue) {
		free(q);
		return NULL;
	}

	q->buf_size=len;
	/* q->header=q->tail=q->size=q->realsize=0; */

	if (thr_mtx_init(&(q->mtx))) {
		free(q->queue);
		free(q);
		return NULL;
	}

	if (thr_cnd_init(&(q->full))) {
		thr_mtx_destroy(&(q->mtx));
		free(q->queue);
		free(q);
		return NULL;
	}

	if (thr_cnd_init(&(q->empty))) {
		thr_cnd_destroy(&(q->full));
		thr_mtx_destroy(&(q->mtx));
		free(q->queue);
		free(q);
		return NULL;
	}

	return q;
}

int fifo_destroy(FIFO *q)
{
	if (!q)
		return -1;

	thr_cnd_destroy(&(q->full));
	thr_cnd_destroy(&(q->empty));
	thr_mtx_destroy(&(q->mtx));
	free(q->queue);
	free(q);

	return 0;
}

/**
 * It is quite complicated to implement n value semaphore
 * using conditional variables. You need to have a wait
 * counter, if the wait counter is >0 for the conditional
 * variable, then you need to ask the new comer to block 
 * instead of getting into the critical section even though
 * the real value is > 0.
 */
int fifo_enqueue(FIFO *q, void *item)
{
	int buf_len=q->buf_size;
	int newtail;

	if (thr_mtx_lock(&(q->mtx)))
		return -1;

	newtail=(q->tail+1)%buf_len;

	while (newtail==q->header) {
		if (thr_cnd_wait(&(q->full), &(q->mtx))) {
			thr_mtx_unlock(&(q->mtx));
			return -1;
		}
		newtail=(q->tail+1)%buf_len;
	}

	q->queue[q->tail]=item;
	q->tail=newtail;

	thr_cnd_signal(&(q->empty));

	thr_mtx_unlock(&(q->mtx));

	return 0;
}

void *fifo_dequeue(FIFO *q)
{
	void *item;

	if (thr_mtx_lock(&(q->mtx)))
		return NULL;

	while (q->tail==q->header) {
		if (thr_cnd_wait(&(q->empty), &(q->mtx))) {
			thr_mtx_unlock(&(q->mtx));
			return NULL;
		}
	}

	item=q->queue[q->header];
	q->header=(q->header+1)%q->buf_size;

	thr_cnd_signal(&(q->full));

	thr_mtx_unlock(&(q->mtx));

	return item;
}
