# include <stdlib.h>
# include "ukey.h"
# include "reduce.h"
# include "rdc_op.h"
# include "thread.h"
# include "tmpi_debug.h"
# include "mpi_struct.h"
# include "tmpi_util.h"
# include "atomic.h"

# define cm_alloc(b) malloc(b)
# define cm_free(b) free(b)

# ifndef MIN
# define MIN(a,b) (((a)<(b))?(a):(b))
# endif

static void (*(optable[][5]))(void *, void *, int) =
{
	/* unused op, max, min, sum */
	/* unused */ {NULL, NULL, NULL, NULL, NULL},
	/* int or long */ {NULL, rdc_int_max, rdc_int_min, rdc_int_sum, NULL},
	/* float */ {NULL, rdc_flt_max, rdc_flt_min, rdc_flt_sum, NULL},
	/* double */ {NULL, rdc_dbl_max, rdc_dbl_min, rdc_dbl_sum, NULL},
	/* char */ {NULL, NULL, NULL, NULL, NULL},
	/* short */ {NULL, NULL, NULL, NULL, NULL},
	/* long long */ {NULL, NULL, NULL, NULL, NULL},
	/* unsigned short */ {NULL, NULL, NULL, NULL, NULL},
	/* unsigned */ {NULL, NULL, NULL, NULL, NULL}
};

/* return 0 for non-default types */
static int rdc_type_idx(MPI_Datatype type)
{
	switch (type) {
		case MPI_INT :
		case MPI_INTEGER :
		case MPI_LONG :
			return 1;

		case MPI_FLOAT :
		case MPI_REAL :
			return 2;

		case MPI_DOUBLE :
		case MPI_DOUBLE_PRECISION :
			return 3;

		case MPI_CHAR :
		case MPI_BYTE :
			return 4;

		case MPI_SHORT :
			return 5;

		case MPI_LONG_LONG_INT :
			return 6;

		case MPI_UNSIGNED_SHORT :
			return 7;

		case MPI_UNSIGNED :
		case MPI_UNSIGNED_LONG :
			return 8;

		case MPI_LONG_DOUBLE :
		case MPI_UNSIGNED_CHAR :
		case MPI_PACKED :
		case MPI_LB :
		case MPI_UB :
		case MPI_FLOAT_INT :
		case MPI_DOUBLE_INT :
		case MPI_LONG_INT :
		case MPI_SHORT_INT :
		case MPI_2INT :
		case MPI_LONG_DOUBLE_INT :
		case MPI_COMPLEX :
		case MPI_DOUBLE_COMPLEX :
		case MPI_LOGICAL :
		case MPI_2INTEGER :
		case MPI_2COMPLEX :
		case MPI_2DOUBLE_COMPLEX :
		case MPI_2REAL :
		case MPI_2DOUBLE_PRECISION :
			return 0;

		default :
			return -1;
	}
}

/* return 0 for non-default ops */
static int rdc_op_idx(MPI_Op op)
{
	switch(op) {
		case MPI_MAX :
			return 1;
		case MPI_MIN  :
			return 2;
		case MPI_SUM  :
			return 3;
		case MPI_PROD :
		case MPI_LAND :
		case MPI_BAND :
		case MPI_LOR  :
		case MPI_BOR  :
		case MPI_LXOR :
		case MPI_BXOR :
		case MPI_MINLOC :
		case MPI_MAXLOC :
			return 0;
		default :
			return -1;
	}
}

static void simple_reduce(void *result, void *operand, int count, MPI_Datatype type, MPI_Op op)
{
	int type_idx=rdc_type_idx(type), op_idx=rdc_op_idx(op);
	void (*opfunc)(void *, void *, int);

	if ( (type_idx==-1) || (op_idx==-1) ) {
		tmpi_error(DBG_INTERNAL, "extended reduce op or data type not supported.");
		return;
	}
	opfunc=optable[type_idx][op_idx];
	if (!opfunc) {
		tmpi_error(DBG_INTERNAL, "(op=%d, type=%d) to be implemented as needed.", op, type);
		return;
	}

	(*opfunc)(result, operand, count);
}

/* a lock free allocator */
/* allocate a RDCHandle for rdco_array[which] */
static tmpi_RDCHandle *tmpi_rdch_alloc(tmpi_ReduceObj *rdco, int which)
{
	register tmpi_RDCHandle **pfree;
	tmpi_RDCHandle *ret;
	register tmpi_RDCHandle *localfree;

	if (rdco->group_size<2) return NULL; /* a group of one proc does not need handle */

	pfree=&(rdco->rdco_array[which]->free);
	ret=*pfree;

	while (ret) {
		localfree=ret->next;
		if (casp((void **)pfree, (void *)ret, (void *)localfree))
			break;
		ret=*pfree;
	}

	if (!ret) { /* no more cached handles */
		int i;

		ret=localfree=(tmpi_RDCHandle *)
		    calloc(rdco->group_size, sizeof(tmpi_RDCHandle));
		if (!localfree)
			return NULL;
		localfree++;

		/* link the chunk of handles */
		for (i=0; i<rdco->group_size-2; i++)
			localfree[i].next=&(localfree[i+1]);

		/* now localfree[i] is the last one */
		do {
			localfree[i].next=*pfree;
		}		while (!casp((void **)pfree, (void *)localfree[i].next, (void *)localfree));
	}

	thr_mtx_init(&(ret->lock));
	thr_cnd_init(&(ret->wait));
	return ret;
}

/* a lock free deallocator */
static void tmpi_rdch_free(tmpi_ReduceObj *rdco, int which, tmpi_RDCHandle *handle)
{
	register tmpi_RDCHandle **pfree=&(rdco->rdco_array[which]->free);

	thr_mtx_destroy(&(handle->lock));
	thr_cnd_destroy(&(handle->wait));

	do {
		handle->next=*pfree;
	} while (!casp((void **)pfree, (void *)handle->next, (void *)handle));

	return;
}

static tmpi_RDCQueue *tmpi_rdcq_create(int groupsize)
{
	tmpi_RDCQueue *ret;
	int i;

	if (groupsize<2) return NULL; /* a group of one proc does not need RDCQueue */

	/* groupsize+1 cached handles */
	ret=(tmpi_RDCQueue *)malloc(sizeof(tmpi_RDCQueue)+sizeof(tmpi_RDCHandle)*(groupsize));

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
	ret->first->avail=-1; /* guarding node */
	thr_mtx_init(&(ret->first->lock));
	thr_cnd_init(&(ret->first->wait));

	return ret;
}

static void tmpi_rdcq_destroy(tmpi_RDCQueue *rdcq)
{
	tmpi_RDCHandle *rdch;

	rdch=rdcq->first;

	while (rdch) {
		thr_mtx_destroy(&(rdch->lock));
		thr_cnd_destroy(&(rdch->wait));
		rdch=rdch->next;
	}

	free(rdcq);
}

tmpi_ReduceObj *tmpi_rdco_create(int groupsize)
{
	tmpi_ReduceObj *ret;
	int i,j;

	ret=(tmpi_ReduceObj *)malloc(sizeof(tmpi_ReduceObj)+sizeof(tmpi_RDCQueue *)*(groupsize-1));

	if (!ret) return NULL;

	ret->group_size=groupsize;

	if (groupsize<2) {
		ret->rdco_array[0]=NULL;
		return ret;
	}

	for (i=0;i<groupsize;i++) {
		if ((ret->rdco_array[i]=tmpi_rdcq_create(groupsize))==NULL) {
			for (j=0;j<i;j++)
				tmpi_rdcq_destroy(ret->rdco_array[j]);
			free(ret);
			return NULL;
		}
	}

	return ret;
}

int tmpi_rdco_destroy(tmpi_ReduceObj *rdco)
{
	int i;

	if (rdco) {
		for (i=0; i<rdco->group_size; i++) {
			if (rdco->rdco_array[i]) {
				tmpi_rdcq_destroy(rdco->rdco_array[i]);
			}
		}

		free(rdco);
	}

	return 0;
}

int tmpi_rdco_reduce(tmpi_ReduceObj *rdco, int myid, int root, void *sendbuf, void *recvbuf, 
		unsigned int count, MPI_Datatype type, MPI_Op op)
{
	register tmpi_RDCHandle *curitem;
	tmpi_RDCHandle *newitem;
	register tmpi_RDCQueue *q;
	int ret;
	int len;

	if (rdco->group_size<2) {
		int len=tmpi_Usize(type)*count;

		memcpy(recvbuf, sendbuf, len);
		/* may not be the case if there are some ABSMAX alike operations */

		return 0; /* don't do anything */
	}

	q=rdco->rdco_array[root];

	curitem=(tmpi_RDCHandle *)ukey_getval(q->key_curhandle);

	if (!curitem)
		curitem=q->first;

	if (curitem==(tmpi_RDCHandle*)8L) /* internal allocation error */
		return -1;

	if (curitem->avail==-1) { /* the first to reach the reduce */
		newitem=tmpi_rdch_alloc(rdco, root);

		if (!newitem) {
			tmpi_error(DBG_INTERNAL, "reduce failed due to insufficient memory.");
			newitem=(tmpi_RDCHandle*)8L;
		}
		else {
			newitem->avail=-1;
			newitem->next=NULL;
		}

		if (cas32(&(curitem->avail), -1, 0)) {
			len=tmpi_Usize(type)*count;
			curitem->next=newitem;
			if (myid!=root)
				curitem->data=cm_alloc(len); 
			else 
				curitem->data=recvbuf;

			if (curitem->data==NULL) {
				tmpi_error(DBG_INTERNAL, "reduce failed due to insufficient memory.");
				curitem->count=0;
				thr_mtx_lock(&(curitem->lock));
				curitem->avail=1;
				thr_cnd_bcast(&(curitem->wait));
				thr_mtx_unlock(&(curitem->lock));
				ukey_setval(q->key_curhandle, newitem);
				return -2; /* no memory */
			}
			else {
				memcpy(curitem->data, sendbuf, len);
				curitem->count=count;
				thr_mtx_lock(&(curitem->lock));
				curitem->avail=1;
				thr_cnd_bcast(&(curitem->wait));
				thr_mtx_unlock(&(curitem->lock));

				if (myid==root) { /* wait for result to come */
					thr_mtx_lock(&(curitem->lock));
					while (curitem->avail!=rdco->group_size)
						thr_cnd_wait(&(curitem->wait), &(curitem->lock));
					thr_mtx_unlock(&(curitem->lock));
					// memcpy(recvbuf, curitem->data, len);
					/* unlink the handle */
					// cm_free(curitem->data);
					/* free(curitem->data); */
					q->first=curitem->next;
					tmpi_rdch_free(rdco, root, curitem);
				}

				ukey_setval(q->key_curhandle, newitem);
				return 0;
			}
		}
		else { /* has been done by somebody else */
			if (newitem!=(tmpi_RDCHandle*)8L)
				tmpi_rdch_free(rdco, root, newitem);
		}
	}

	/* do reduction */
	thr_mtx_lock(&(curitem->lock));
	while (curitem->avail==0) {
		thr_cnd_wait(&(curitem->wait), &(curitem->lock));
	}
	thr_mtx_unlock(&(curitem->lock));

	count=MIN(count, curitem->count);
	len=tmpi_Usize(type)*count;

	if (curitem->data) {
		simple_reduce(curitem->data, sendbuf, count, type, op);
		ret=0;
	}
	else
		ret=-2;

	ukey_setval(q->key_curhandle, curitem->next);

	thr_mtx_lock(&(curitem->lock));
	curitem->avail++;
	if ((curitem->avail==rdco->group_size) && (myid!=root))
		thr_cnd_signal(&(curitem->wait));
	thr_mtx_unlock(&(curitem->lock));

	if (myid==root) {
		if (curitem->avail!=rdco->group_size) {
			thr_mtx_lock(&(curitem->lock));
			while (curitem->avail!=rdco->group_size)
				thr_cnd_wait(&(curitem->wait), &(curitem->lock));
			thr_mtx_unlock(&(curitem->lock));
		}

		if (!ret) {
			memcpy(recvbuf, curitem->data, len);
			/* unlink the handle */
			cm_free(curitem->data);
			/* free(myid, curitem->data); */
		}
		q->first=curitem->next;
		tmpi_rdch_free(rdco, root, curitem);
	}

	return ret;
}

/* constructor for all reduce object */
tmpi_AllreduceObj *tmpi_ardco_create(int groupsize) {
	tmpi_AllreduceObj *ret;
	int i;

	ret = (tmpi_AllreduceObj *) calloc(1, sizeof(tmpi_AllreduceObj));
	if (!ret) return NULL;

	ret->group_size = groupsize;
	ret->subp = &(ret->sub[0]);

	for (i=0; i<2; i++) {
		ret->sub[i].avail = -1;
		ret->sub[i].done = 0;
	}

	thr_mtx_init(&(ret->lock));
	thr_cnd_init(&(ret->wait));
	return ret;
}

/* destructor for all reduce object */
int tmpi_ardco_destroy(tmpi_AllreduceObj *ardco) {
	thr_mtx_destroy(&(ardco->lock));
	thr_cnd_destroy(&(ardco->wait));
	free(ardco);

	return 0;
}

int tmpi_ardco_allreduce(tmpi_AllreduceObj *ardco, int myid, void *sendbuf, 
		void *recvbuf, unsigned int count, MPI_Datatype type, MPI_Op op) 
{
	register struct _sub *subp = ardco->subp;
	int len;
	int ret=0;

	if (ardco->group_size<2) {
		int len=tmpi_Usize(type)*count;

		memcpy(recvbuf, sendbuf, len);
		/* may not be the case if there are some ABSMAX alike operations */

		return 0;	/* don't do anything */
	}

	/* the first to reach the allreduce */
	if ((subp->avail==-1) && (cas32(&(subp->avail), -1, 0)) ) {
 		len = tmpi_Usize(type)*count;
		subp->data = cm_alloc(len);
		if (subp->data==NULL) {
			tmpi_error(DBG_INTERNAL, "allreduce failed due to insufficient memory.");
			subp->count = 0;
			thr_mtx_lock(&(ardco->lock));
			subp->avail = 1;
			thr_cnd_bcast(&(ardco->wait));
			thr_mtx_unlock(&(ardco->lock));
			ret=-2;	/* no enough memory */
		}
		else {
			memcpy(subp->data, sendbuf, len);
			subp->count = count;
			thr_mtx_lock(&(ardco->lock));
			subp->avail = 1;
			thr_cnd_bcast(&(ardco->wait));
			thr_mtx_unlock(&(ardco->lock));
		}
	}
	else {
		/* do reduction */
		thr_mtx_lock(&(ardco->lock));
		while (subp->avail==0)
			thr_cnd_wait(&(ardco->wait), &(ardco->lock));
		thr_mtx_unlock(&(ardco->lock));

		count=MIN(count, subp->count);
		len=tmpi_Usize(type)*count;
		if (subp->data) {
			simple_reduce(subp->data, sendbuf, count, type, op);
		}
		else
			ret=-2;

		thr_mtx_lock(&(ardco->lock));
		subp->avail++;
		if (subp->avail==ardco->group_size)
			thr_cnd_bcast(&(ardco->wait));
		thr_mtx_unlock(&(ardco->lock));

	}

	/* wait for result to come */
	if (subp->avail!=ardco->group_size) {
		thr_mtx_lock(&(ardco->lock));
		while (subp->avail!=ardco->group_size)
			thr_cnd_wait(&(ardco->wait), &(ardco->lock));
		thr_mtx_unlock(&(ardco->lock));
	}

	if (subp->data)
		memcpy(recvbuf, subp->data, len);

	/* last one done with it */
	if (anf32(&(subp->done), 1)==ardco->group_size) {
		if (subp->data)
			cm_free(subp->data);
		subp->done = 0;
		subp->avail = -1;
	}

	/* switch the sub object */
	if (ardco->subp==subp)
		casp((void **)&(ardco->subp), (void *)subp, 
				(void *)( ( subp==&(ardco->sub[0]) )
						 ? &(ardco->sub[1]) : &(ardco->sub[0]) )
				);

	return ret;
}
