# ifndef _TQUEUE_H_
# define _TQUEUE_H_

# include "thread.h"

/**
 * This is a simplified version of the original tqueue interface.
 * Most significantly, it is not multi-thread safe.
 * Callers are responsible for mutual exclusion.
 * Creation/Termination Interface: 	
 * 		Q_create, 
 * 		Q_init, 
 * 		Q_reset, 
 * 		Q_free, 
 *
 * Queue Operation:
 * 		enQ, 
 * 		deQ, 
 * 		Q_probe, 
 * 		Q_find, 
 * 		Q_removeElem, 
 *
 * Item management:
 * 		Q_newElem, 
 * 		Q_freeElem 
 */
/**
 * Q_create implies Q_init, cmp function can be NULL, in which case the queue 
 * is just a standard queue. Queues created by Q_create must be freed by Q_free.
 * Q_init initialize the queue, this is useful when we already have a TQueue object
 * e.g. in stack or inside another structure.
 * Q_reset resets the queue to intial states, all elements in the queue are freed 
 *
 * enQ add one element to the end of the queue
 * deQ move the element at the head of the queue, if not empty, from the queue
 *
 * Q_probe, Q_find are not enabled until a non-NULL cmp function is provided for Q_create, 
 * Q_probe and Q_find extend the queue to a simple priority queue. The cmp takes an element 
 * and a src field, a context and a tag, each of which is an integer. context/tag/src can be
 * combined to define pretty a wide range of search criteria. The need for these
 * three fields (instead of one pointer to a structure) is quite MPI specific.
 * Both Q_probe and Q_find return the first element searching from the head of the queue such 
 * that cmp(elem, src, tag) returns 0, or NULL if none of such elements exist. Q_probe differs from
 * Q_find in that Q_probe does not remove the element from the queue while Q_find does.
 * In other words, Q_probe + Q_removeElem == Q_find.
 *
 * All elements are to be allocated/deallocated by Q_newElem/Q_freeElem
 */

/** 
 * Traverse function: 
 * 		Q_first, 
 * 		Q_last, 
 * 		Q_next. 
 *
 * Queue Check:
 * 		Q_isEmpty,
 * 		Q_cnt,
 */

/**
 * Q_first returns the first item in the queue, if queue empty , returns NULL;
 *
 * Q_next takes an item as parameter and returns the following item in the queue, 
 * if already at the end of the queue, return NULL;
 *
 * Q_last returns the end of the queue, if queue empty, return NULL.
 *
 * Q_isEmpty returns 1 if queue is empty or 0 otherwise.
 * Q_cnt returns the length of the queue at this point.
 */ 

/**
 * Profiling counter:
 * 		Q_incr, 
 * 		Q_decr, 
 * 		Q_ucnt, 
 * 		Q_resetUcnt 
 */

/** 
 * These new functions are provided specifically for the MPI routines.
 * Q_incr will atomically add a specific number to a user counter and return the value
 * of the counter after the addition. 
 * 
 * Q_decr, similarly, will atomically subtract a number from a user counter and return the
 * value of the counter after the subtraction. Note, Q_decr(q, n) is equivalent to 
 * Q_incr(q, -n).
 *
 * Q_ucnt returns the current value of the count. It is equivalent to Q_incr(q, 0).
 *
 * Q_resetUcnt will reset the count to be 0. But at the return time, it is not guarantteed
 * the value of the user counter still be 0.
 */

typedef struct BUCKET
{
	struct BUCKET *next;
	struct BUCKET *prev;
} BUCKET;

typedef struct queue
{
	BUCKET 	qctrl;	/* next to the head and prev to the end of the queue */
	int 	count;	/* number of elements currently in queue */
	int 	ucount;	/* not used by queue management, but can be used by user */
	int 	(*cmp)(void *item1, int context, int tag, int src);/* comparison function */
			/* those elements in the freed list will be deferred when nTraverse==0 */
	BUCKET * allocate; /* linked list of allocable BUCKET's */
	int		item_size; /* items in each queue are equal sized */
	int 	chunk_group; /* queue items are allocated in chunks */
			     /* since handles are recycled, the size of chunk_group */
			     /* is not quite sensitive to performance */
	thr_mtx_t alloc_lock; /* quick hack */
}TQueue;

/* even quicker macros */
# define QHEAD(q) ((q)->qctrl.next)
# define QTAIL(q) ((q)->qctrl.prev)

/* Creation/Termination Interface: 	*/
TQueue * Q_create( int (* cmp_function)(void *elem, int context, int tag, int src), \
	int item_size, int chunk_group);
void Q_init(TQueue *queue, int (* cmp_function)(void *elem, int context, int tag, int src), \
	int item_size, int chunk_group); /* init the queue */
void Q_reset(TQueue *queue); /* set queue to be an empty queue */
void Q_free(TQueue *queue);

/* Queue Operation: */
void enQ(TQueue *queue, void *elem);
void *deQ(TQueue *queue);
void *Q_removeElem(TQueue *queue, void *elem);
void *Q_probe(TQueue *queue, int context, int tag, int src);
void *Q_find(TQueue *queue, int context, int tag, int src);

/* Item management: */
void *Q_newElem(TQueue *queue);
void Q_freeElem(TQueue *queue, void *elem);

/* Traverse function:  */
void *Q_first(TQueue *queue);
void *Q_next(TQueue *queue, void *item);
void *Q_last(TQueue *queue);

/* Queue Check: */
int Q_isEmpty(TQueue *queue);
int Q_cnt(TQueue *queue); /* return the length of the queue */

/* Profiling counter: */
int Q_incr(TQueue *queue, int n);
int Q_decr(TQueue *queue, int n);
int Q_ucnt(TQueue *queue);
void Q_resetUcnt(TQueue *queue);
# endif
