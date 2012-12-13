# ifndef _FIFO_H_
# define _FIFO_H_
# include "thread.h"
/**
 * This implements a very simple bounded buffer
 * FIFO queue. Enqueue and dequeue could block.
 * Application writer needs to be aware of that
 * to avoid deadlock.
 */
typedef struct _fifo {
	int buf_size;
	void **queue;
	int header;
	int tail; 
	thr_mtx_t mtx;
	thr_cnd_t full;
	thr_cnd_t empty;
} FIFO;

# define FIFO_LEN_DEFAULT 1024

FIFO *fifo_create(int len);
int fifo_destroy(FIFO *q);

int fifo_enqueue(FIFO *q, void *item);
void *fifo_dequeue(FIFO *q);

# endif
