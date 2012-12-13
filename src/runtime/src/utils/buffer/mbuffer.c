/*
 * mbuffer.c -- message buffer management
 * In this version, each thread only access a piece of dedicated buffer.
 * Mutual exclusion is still necessary.
 */

# include <stdlib.h>
# include "tmpi_debug.h"
# include "mbuffer.h"
# include "thread.h"
# include "machine.h"

# if (0)
int mb_global_init(int nproc) 
{
	return 0;
}

int mb_global_end()
{
	return 0;
}

int mb_init(int myid, int bsize) 
{
	return 0;
}

int mb_end(int myid)
{
	return 0;
}


void *mb_alloc(int myid, int isize) 
{
	return malloc(isize);
}

void mb_free(int myid, void* item)
{
	free(item);
}

int mb_attach_buffer(int myid, void *buf, int bsize)
{
	return 0;
}

int mb_detach_buffer(int myid, void *buf, int *bsize)
{
	*(void **)buf=NULL;

	return 0;
}

# else

/* Alignment must be a power of 2 */
#define Alignment 32
#define AlignmentMask 0xffffffe0
#define AdjustSize(x) (((x) + Alignment - 1) & AlignmentMask)	/* x must 32-bit */

/* buffer handle */
typedef struct BufferHandle {
	struct BufferHandle *pre;
	struct BufferHandle *next;
	int free;
	unsigned size;
} BufferHandle;

/**
 * The size field of BufferHandle has been changed to reflect the size of 
 * the whole buffer instead of the user seen buffer (which does not include
 * the size of the handle). 
 */

#define BUF_HEADER_SIZE (AdjustSize(sizeof(BufferHandle)))

#define MinFragment (BUF_HEADER_SIZE+64)

static BufferHandle **SysBuffer=NULL, **UsrBuffer=NULL;
static thr_mtx_t **BufferLock=NULL;

/* message buffer initialization by whole system */
int mb_global_init(int nproc) {
	SysBuffer = (BufferHandle**) calloc (nproc, sizeof(BufferHandle*));
	UsrBuffer = (BufferHandle**) calloc (nproc, sizeof(BufferHandle*));
	BufferLock = (thr_mtx_t **) calloc (nproc, sizeof(thr_mtx_t *));

	if ((!SysBuffer)||(!UsrBuffer)||(!BufferLock)) {
		mb_global_end();
		return -1;
	}

	return 0;
}

int mb_global_end()
{
	if (SysBuffer) {
		free(SysBuffer);
		SysBuffer=NULL;
	}

	if (UsrBuffer) {
		free(UsrBuffer);
		UsrBuffer=NULL;
	}

	if (BufferLock) {
		free(BufferLock);
		BufferLock=NULL;
	}

	return 0;
}

/* message buffer initialization by each thread */
int mb_init(int myid, int bsize) {
	int lock_size;
	int done=0;

	while (!done && (bsize>0) ) {
		if (bsize<0x100000) {
			bsize=0x100000;
		}

		SysBuffer[myid] = (BufferHandle*) malloc (bsize); 

		if (SysBuffer[myid]!=NULL) {
			done=1;
			break;
		}
		else {
			bsize-=0x100000;
		}
	}

	if (!done) {
		tmpi_error(DBG_INTERNAL, "Message buffer init failed.");
		return -1;
	}

	SysBuffer[myid]->pre = NULL;
	SysBuffer[myid]->next = NULL;
	SysBuffer[myid]->free = 1;
	SysBuffer[myid]->size = bsize;

	UsrBuffer[myid] = NULL;

	lock_size=(sizeof(thr_mtx_t)+CACHE_LINE-1)/CACHE_LINE*CACHE_LINE;
	BufferLock[myid]=(thr_mtx_t *)calloc(1, lock_size);
	if (BufferLock[myid]) {
		thr_mtx_init(BufferLock[myid]);
	}
	else {
		mb_end(myid);
		return -1;
	}

	return 0;
}

int mb_end(int myid)
{
	if (SysBuffer[myid]) {
		free(SysBuffer[myid]);
		SysBuffer[myid]=NULL;
	}

	if (BufferLock[myid]) {
		thr_mtx_destroy(BufferLock[myid]);
		free(BufferLock[myid]);
		BufferLock[myid]=NULL;
	}

	return 0;
}

/* buffer allocation */
void *mb_alloc(int myid, int isize) {
	register BufferHandle *tmp, *tmpnext;
	register unsigned adjusted_size;

	thr_mtx_lock(BufferLock[myid]);
	adjusted_size = AdjustSize(isize) + BUF_HEADER_SIZE; 

	tmp = SysBuffer[myid]; 
	while (tmp!=NULL && (tmp->free==0 || tmp->size<adjusted_size)) 
		tmp = tmp->next;

	if (tmp==NULL && UsrBuffer[myid]!=NULL) {
		tmp = UsrBuffer[myid];
		while (tmp!=NULL && (tmp->free==0 || tmp->size<adjusted_size)) 
			tmp = tmp->next;
	}

	if (tmp==NULL) {
		thr_mtx_unlock(BufferLock[myid]);
		return NULL;
	}

	tmp->free = 0;

	if (tmp->size >= adjusted_size + MinFragment) { /* need split */
		tmpnext = (BufferHandle*)(((char*)tmp)+adjusted_size);
		tmpnext->pre = tmp;
		tmpnext->next = tmp->next;
		if (tmpnext->next)
			tmpnext->next->pre=tmpnext;
		tmpnext->free = 1;
		tmpnext->size = tmp->size-adjusted_size;

		tmp->size = adjusted_size;
		tmp->next = tmpnext;
	}

	thr_mtx_unlock(BufferLock[myid]);
	return (((char*)tmp)+BUF_HEADER_SIZE);
}

/* free an allocated buffer */
void mb_free(int myid, void* item)
{
	register BufferHandle *bh = (BufferHandle*)(((char*)item)-BUF_HEADER_SIZE);

	thr_mtx_lock(BufferLock[myid]);
	if (bh->next && bh->next->free==1) {
		if (bh->pre && bh->pre->free==1) {
			bh->pre->size += bh->size + bh->next->size;
			bh->pre->next = bh->next->next;
			if (bh->next->next)
				bh->next->next->pre=bh->pre;
		}
		else {
			bh->free = 1;
			bh->size += bh->next->size;
			bh->next = bh->next->next;
			if (bh->next)
				bh->next->pre = bh;
		}
	}
	else {
		if (bh->pre && bh->pre->free==1) {
			bh->pre->size += bh->size;
			bh->pre->next = bh->next;
			if (bh->next)
				bh->next->pre=bh->pre;
		}
		else
			bh->free = 1;
	}
	thr_mtx_unlock(BufferLock[myid]);
}

int mb_attach_buffer(int myid, void *buf, int bsize)
{
	BufferHandle **ubhandle=UsrBuffer+myid;

	if (*ubhandle != NULL) return -1;

	thr_mtx_lock(BufferLock[myid]);
	*ubhandle=(BufferHandle *)buf;
	(*ubhandle)->pre = NULL;
	(*ubhandle)->next = NULL;
	(*ubhandle)->free = 1;
	(*ubhandle)->size = bsize;
	thr_mtx_unlock(BufferLock[myid]);

	return 0;
}

int mb_detach_buffer(int myid, void *buf, int *bsize)
{
	BufferHandle **ubhandle=UsrBuffer+myid;

	if (*ubhandle==NULL) return -1;

	thr_mtx_lock(BufferLock[myid]);
	*((BufferHandle**)buf) = *ubhandle;
	*bsize = (*ubhandle)->size;

	if ( ( (*ubhandle)->free == 0) || ((*ubhandle)->next!=NULL) ) {
		thr_mtx_unlock(BufferLock[myid]);
		return -1;
	}
	else {
		*ubhandle = NULL;
		thr_mtx_unlock(BufferLock[myid]);
		return 0;
	}
}

# endif
