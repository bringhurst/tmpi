/**
 * A quite complicated double link list implementation 
 */
# include <stdio.h>
# include <ctype.h>
# include <stdlib.h>
# include <string.h>
# include "tqueue.h"
# include "tmpi_debug.h"
# include "atomic.h"
# include "thread.h"

void *Q_newElem(TQueue *p)
{

 	BUCKET *elem;
	register BUCKET *b, *b1;
	register int size1;

	thr_mtx_lock(&(p->alloc_lock));
	if (!p->allocate) {
		int i;

		size1=p->item_size;
 		if (!(b=elem=(BUCKET *)malloc(size1*p->chunk_group))) {
			thr_mtx_unlock(&(p->alloc_lock));
			return NULL;
 		}

		b1=(BUCKET *)((char *)b+size1);
		for (i=1; i<p->chunk_group-1; i++) {
			b=b1;
			b1=(BUCKET *)((char *)b+size1);
			b->prev=b1;
		}
			
		b1->prev=NULL;

		p->allocate=(BUCKET *)((char *)elem+size1);
	}
	else {
		elem=p->allocate;
		
		p->allocate=p->allocate->prev;
	}

	thr_mtx_unlock(&(p->alloc_lock));

	elem->next=NULL;

 	return (void *)(elem+1); /* return pointer to user space */
}

void Q_freeElem(TQueue *p, void *elem)
{
	BUCKET *e;

	if (!elem) return;

	e=(BUCKET *)elem-1;

	thr_mtx_lock(&(p->alloc_lock));
	e->prev = p->allocate;
	p->allocate = e;
	thr_mtx_unlock(&(p->alloc_lock));
}

TQueue * Q_create( int (* cmp_function)(void *elem, int context, int tag, int src), 
		int item_size, int group)
{
 	TQueue *p;
	
	if ( (item_size <= 0) || (group <= 1) ) 
		return NULL;

 	if ( (p=(TQueue *)malloc(sizeof(TQueue))) ) {
		Q_init(p, cmp_function, item_size, group); 
 	}
 	else {
		return NULL;
 	}
 	return p;
}

/* p must be allocatd by Q_create */
void Q_free(TQueue *p)
{
	thr_mtx_destroy(&(p->alloc_lock));
	free(p); /* all handle memory are lost! */
}

__inline__ static BUCKET *Q_delElem(TQueue *p, BUCKET *elem)
{
	elem->prev->next=elem->next;
	elem->next->prev=elem->prev;

	p->count--;

	return elem;
}

void *Q_removeElem(TQueue *p, void *elem)
{
	Q_delElem(p, (BUCKET *)elem-1);

	return elem;
}

void Q_init( TQueue *p, int (* cmp_function)(void *elem, int context, int tag, int src), 
		int item_size, int group)
{
	p->count = 0;
	p->ucount = 0;
	p->cmp = cmp_function;
	p->qctrl.prev=p->qctrl.next=&(p->qctrl);
	p->allocate=NULL;
	thr_mtx_init(&(p->alloc_lock));

	/* item_size must be a multiple of 8 */
 	/* BUCKET is already of size multiple of 8 */
	if (item_size % 0x07)
		p->item_size=(item_size+7)>>3<<3;
	else	
		p->item_size=item_size;

	p->item_size+=sizeof(BUCKET);
	p->chunk_group=group;

	Q_freeElem(p, Q_newElem(p)); /* first touch to generate initial handle cache */

	return;
}

void Q_reset( TQueue *p)
{
	
	while ( &(p->qctrl) != p->qctrl.next )
		Q_freeElem(p, Q_removeElem(p, p->qctrl.next));

	p->ucount=0;
	thr_mtx_init(&(p->alloc_lock));
}

void enQ(TQueue *p, void *elem)
{
	BUCKET *e=(BUCKET *)elem-1;
	
	e->next=&(p->qctrl);
	e->prev=p->qctrl.prev;
	p->qctrl.prev->next=e;
	p->qctrl.prev=e;
	p->count++;

	return;
}
	
void * deQ(TQueue *p)
{

	if ( p->qctrl.next==&(p->qctrl) )
		return NULL;

	return (void *)(Q_delElem(p, p->qctrl.next)+1);
}

void Q_status(TQueue *p)
{
	int nalloc=0;
	BUCKET *e;

	e=p->allocate;
	while (e) {
		nalloc++;
		e=e->prev;
	}

	tmpi_error(DBG_INFO, "Queue: \n\tQ->count = %d;\n\tQ->ucount = %d;\n\tQ->allocate=%d;\n", p->count, p->ucount, nalloc);

	return;
}

void printQ(TQueue *p)
{
	BUCKET *e=p->qctrl.next;
	
	if (e==&(p->qctrl)) {
		tmpi_error(DBG_INFO, "empty queue\n");
		ASSERT(Q_isEmpty(p));
		return;
	}
	tmpi_error(DBG_INFO, "Queue: Head");
	while (e!=&(p->qctrl)) {
		tmpi_error(DBG_INFO, "-->%d", *(int *)(e+1));
		e=e->next;
	}
	tmpi_error(DBG_INFO, "<--Tail\n");
}

void *Q_probe(TQueue *p, int context, int tag, int src)
{
	register BUCKET *e;
	register BUCKET *f;
	int done=0;
	
	if (!p->cmp) return NULL;
	
	f=&(p->qctrl);
	e=p->qctrl.next;
	while (e && (e!=f) ) {
		if ( (*p->cmp)((void *)(e+1), context, tag, src)==0) {
			done=1;
			break;
		}
		e=e->next;
	}

	if (!done) return NULL;
	else return (void *)(e+1); 
}

/* master operation */
void *Q_find(TQueue *p, int context, int tag, int src)
{
	register BUCKET *f, *e;
	
	if (!p->cmp) return NULL;

	f=&(p->qctrl);
	e=p->qctrl.next;

	while (e && (e!=f) ) {
		if ( (*p->cmp)((void *)(e+1), context, tag, src)==0) {
			Q_delElem(p, e);
			return (void *)(e+1);
		}
		e=e->next;
	}

	return NULL;
}

int Q_isEmpty(TQueue *q)
{
	if (q->count) return 0;

	return 1;
}

void *Q_first(TQueue *p)
{
	register BUCKET *e=&(p->qctrl);
	register BUCKET *f=e->next;

	return (e==f)? NULL: (void *)(f+1);
}

void *Q_next(TQueue *p, void *item)
{
	register BUCKET *e=&(p->qctrl);
	register BUCKET *f=((BUCKET *)item-1)->next;

	return (e==f)? NULL : (void *)(f+1);
}

void *Q_last(TQueue *p)
{
	register BUCKET *e=&(p->qctrl);
	register BUCKET *f=e->prev;

	return (e==f)? NULL: (void *)(f+1);
}

int Q_incr(TQueue *p, int n)
{
	if (n)
		return anf32(&(p->ucount), n);
	else
		return p->ucount;
}

int Q_decr(TQueue *p, int n)
{
	if (n)
		return snf32(&(p->ucount), n);
	else
		return p->ucount;
}

int Q_ucnt(TQueue *p)
{
	return p->ucount;
}

void Q_resetUcnt(TQueue *p)
{
	p->ucount=0;
	return;
}

int Q_cnt(TQueue *p)
{
	return p->count;
}
