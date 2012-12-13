# include <stdio.h>
# include <stdlib.h>

# include "bcast.h"
# include "ukey.h"
# include "atomic.h"
# include "tmpi_debug.h"

# define cm_alloc(b) malloc(b)
# define cm_free(b) free(b)

# ifndef MIN
# define MIN(a,b) (((a)<(b))?(a):(b))
# endif

/**
 * a not 100% safe lock free allocator because 
 * their could be other proc deallocate handles 
 */
/* allocate a BCHandle for bco_array[which] */
static tmpi_BCHandle *tmpi_bch_alloc(tmpi_BcastObj *bco, int which)
{
	register tmpi_BCHandle **pfree;
	tmpi_BCHandle *ret;
	register tmpi_BCHandle *localfree;

	if (bco->group_size<2) return NULL; /* a group of one proc does not need handle */

	pfree=&(bco->bco_array[which]->free);
	ret=*pfree;

	while (ret) {
		localfree=ret->next;
		if (casp((void **)pfree, (void *)ret, (void *)localfree))
			break;
		ret=*pfree;
	}
		
	if (!ret) { /* no more cached handles */
		/* the following should never happen */
		int i;

		ret=localfree=(tmpi_BCHandle *)calloc(bco->group_size, sizeof(tmpi_BCHandle));
		if (!localfree)
			return NULL;
		localfree++;

		/* link the chunk of handles */
		for (i=0; i<bco->group_size-2; i++)
			localfree[i].next=&(localfree[i+1]);
		
		/* now localfree[i] is the last one */
		do {
			localfree[i].next=*pfree;
		}
		while (!casp((void **)pfree, (void *)localfree[i].next, (void *)localfree));
	}	

	thr_mtx_init(&ret->lock);
	thr_cnd_init(&ret->wait);
	return ret;
}

/* a lock free deallocator */
static void tmpi_bch_free(tmpi_BcastObj *bco, int which, tmpi_BCHandle *handle)
{
	register tmpi_BCHandle **pfree=&(bco->bco_array[which]->free);

	thr_mtx_destroy(&handle->lock);
	thr_cnd_destroy(&handle->wait);
	
	do {
		handle->next=*pfree;
	} while (!casp((void **)pfree, (void *)handle->next, (void *)handle));

	return;
}

static tmpi_BCQueue *tmpi_bcq_create(int groupsize)
{
	tmpi_BCQueue *ret;
	int i;

	if (groupsize<2) return NULL; /* a group of one proc does not need BCQueue */

	/* have groupsize+1 cached handle */
	ret=(tmpi_BCQueue *)malloc(sizeof(tmpi_BCQueue)+sizeof(tmpi_BCHandle)*(groupsize));

	if (!ret) return NULL;
	if (ukey_create(&(ret->key_curhandle))) { /* key creation failed */
		free(ret);
		return NULL;
	}

	ret->first=ret->cache;
	ret->free=ret->cache+1;

	for (i=0; i<groupsize-1; i++)
		ret->free[i].next=&(ret->free[i+1]);

	ret->free[i].next=NULL;

	ret->first->next=NULL;
	ret->first->avail=0; /* guarding node */
	thr_mtx_init(&(ret->first->lock));
	thr_cnd_init(&(ret->first->wait));

	return ret;
}
	
/* there is minor memory leak problem */
static void tmpi_bcq_destroy(tmpi_BCQueue * bcq)
{
	tmpi_BCHandle *bch;

	bch=bcq->first;

	while (bch) {
		thr_mtx_destroy(&(bch->lock));
		thr_cnd_destroy(&(bch->wait));
		bch=bch->next;
	}

	free(bcq);

	return;
}

tmpi_BcastObj *tmpi_bco_create(int groupsize)
{
	tmpi_BcastObj *ret;
	int i,j;

	ret=(tmpi_BcastObj *)
		malloc(sizeof(tmpi_BcastObj)+sizeof(tmpi_BCQueue *)*(groupsize-1));

	if (!ret) return NULL;

	ret->group_size=groupsize;
	
	if (groupsize<2) {
		ret->bco_array[0]=NULL;
		return ret;
	}

	for (i=0;i<groupsize;i++) {
		if ((ret->bco_array[i]=tmpi_bcq_create(groupsize))==NULL) {
			for (j=0;j<i;j++)
				tmpi_bcq_destroy(ret->bco_array[j]);
			free(ret);
			return NULL;
		}
	}

	return ret;
}

int tmpi_bco_destroy(tmpi_BcastObj *bco)
{
	int i;

	if (bco) {
		for (i=0; i<bco->group_size; i++) {
			if (bco->bco_array[i]) {
				tmpi_bcq_destroy(bco->bco_array[i]);
			}
		}

		free(bco);
	}

	return 0;
}

int tmpi_bco_bcast(tmpi_BcastObj *bco, int myid, int root, char *data, unsigned int len) {
	register tmpi_BCHandle *curitem;
	tmpi_BCHandle *newitem;
	register tmpi_BCQueue *q;
	int ret;

	if (bco->group_size<2) return 0; /* don't do anything */ 

	q=bco->bco_array[root];

	curitem=(tmpi_BCHandle *)ukey_getval(q->key_curhandle);
	if (!curitem)
		curitem=q->first;

	if (curitem==(tmpi_BCHandle*)8L) {
		/* internal allocation error */
		return -1;
	}

	/* now curitem holds a handle whose avail is not -1 */

	if (myid==root) { /* now avail must be zero, functioning like a lock */
		/* expend the queue by one */
		newitem=tmpi_bch_alloc(bco, root);
		if (!newitem) {
			tmpi_error(DBG_INTERNAL, "bcast failed due to insufficient memory.");
			newitem=(tmpi_BCHandle*)8L;
		}
		else {
			newitem->avail=0;
			newitem->next=NULL;

		}

		curitem->next=newitem;

		/* if ((curitem->data=anl_malloc(len))==NULL) { */
		/* if ((curitem->data=mb_alloc(myid, len))==NULL) { */
		if ((curitem->data=cm_alloc(len))==NULL) { 
			curitem->len=0;
			ret=-2; /* no memory */
		}
		else {
			memcpy(curitem->data, data, len);
			curitem->len=len;
			ret=0;
		}

		thr_mtx_lock(&(curitem->lock));
		curitem->avail=1; /* the buffer is now available */
		thr_cnd_bcast(&(curitem->wait));
		thr_mtx_unlock(&(curitem->lock));

		/* same as curitem->next, but local data will ensure faster access */
		ukey_setval(q->key_curhandle, newitem);

 		/* better signal everybody, but don't do it now for easy debugging */
		/* signalAllFlag(&(curitem->avail), 1); */
		/* oh no, signalAll is too costly, why not just let recver wake by itself */
	}
	else { /* receivers */
		thr_mtx_lock(&(curitem->lock));
		while (!curitem->avail) {
			thr_cnd_wait(&(curitem->wait), &(curitem->lock));
		}
		thr_mtx_unlock(&(curitem->lock));

		if (curitem->data) {
			memcpy(data, curitem->data, MIN(len, curitem->len));
			ret=0;
		}
		else
			ret=-2;

		/* next item must be non-zero now */
		ukey_setval(q->key_curhandle, curitem->next);

		if (anf32(&(curitem->avail), 1)==bco->group_size) { /* last to leave */
			if (!ret)
				cm_free(curitem->data); 
			q->first=curitem->next;
			tmpi_bch_free(bco, root, curitem);
		}
	}

	return ret;
}
