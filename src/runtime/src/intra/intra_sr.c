/**
 * Even though this file deals mostly with intra-node pt2pt comm,
 * major work for inter-node pt2pt is also included.
 * Particularly from a receivers point of view, it really doesn't
 * know whether it is gonna to be a intra-node operation or inter-node
 * operation.
 */

# include <stdlib.h>
# include "mpi_struct.h"
# include "tmpi_struct.h"
# include "tmpi_debug.h"
# include "thread.h"
# include "tqueue.h"
# include "stackallocator.h"
# include "netq.h"
# include "netq_comm.h"
# include "mbuffer.h"
# include "intra_sr.h"
# include "tmpi_util.h"
# include "comm.h"

static tmpi_Channel **tmpi_channel=NULL;
static int tmpi_channel_size=0;
static StackAllocator tmpi_inter_msg;
# define TMPI_GARBAGE_DATA_SIZE 20
static char tmpi_garbage_data[TMPI_GARBAGE_DATA_SIZE]="Cancelled!";

# ifndef MAX
# define MAX(a,b) (((a)>(b))?(a):(b))
# endif

# ifndef MIN
# define MIN(a,b) (((a)<(b))?(a):(b))
# endif

int tmpi_grank_to_lrank(int grank)
{
	ID_ENTRY id_entry;

	if (netq_transGlobalRank(grank, &id_entry)<0)
		return -1;

	if (id_entry.macID!=netq_getLocalMacID())
		return -1;

	return id_entry.localID;
}

tmpi_Channel *tmpi_intra_get_channel(int local_id)
{
	if ((local_id<0) || (local_id>=tmpi_channel_size) || (!tmpi_channel) )
		return NULL;

	return tmpi_channel[local_id];
}

/* receiver matching, tag src can be wildcard */
static int sq_cmp(tmpi_Common *handle, int context, int tag, int src)
{
	if ( ((handle->src!=src) && (src!=MPI_ANY_SOURCE))  || 
			((handle->tag!=tag)&&(tag!=MPI_ANY_TAG)) || 
			(handle->context!=context))
		return 1;

	/* double check if it is a legal handle */
	if (!(handle->stat & TMPI_PEND)) {
		tmpi_error(DBG_INTERNAL, "sq_cmp: found none pending handle in send queue.");
		return 1;
	}

	return 0;
}

static int rq_cmp(tmpi_Common *handle, int context, int tag, int src)
{
	if ( ((handle->src!=src) && (handle->src!=MPI_ANY_SOURCE))  || 
			((handle->tag!=tag)&&(handle->tag!=MPI_ANY_TAG)) || 
			(handle->context!=context))
		return 1;

	/* double check if it is a legal handle */
	if (!(handle->stat & TMPI_PEND)) {
		tmpi_error(DBG_INTERNAL, "rq_cmp: found none pending handle in receive queue.");
		return 1;
	}

	return 0;
}

static tmpi_Channel *tmpi_intra_new_channel()
{
	tmpi_Channel *ch;
	int np=netq_getGlobalSize();

	ch=(tmpi_Channel *)malloc(sizeof(tmpi_Channel));

	if (!ch)
		return NULL;

	thr_mtx_init(&(ch->lock));
	ch->sq=Q_create((int (*)(void *, int, int, int))&sq_cmp, MAX(sizeof(tmpi_SHandle), 
				sizeof(tmpi_RMSHandle)), np+1);
	ch->rq=Q_create((int (*)(void *, int, int, int))&rq_cmp, sizeof(tmpi_RHandle), np+1);

	if ((!ch->sq) || (!ch->rq)) {
		if (ch->sq)
			Q_free(ch->sq);
		if (ch->rq)
			Q_free(ch->rq);
		thr_mtx_destroy(&(ch->lock));
		free(ch);
		return NULL;
	}

	return ch;
}

static int tmpi_intra_del_channel(tmpi_Channel *ch)
{
	if (!ch)
		return 0;

	if (ch->sq)
		Q_free(ch->sq);
	if (ch->rq)
		Q_free(ch->rq);

	thr_mtx_destroy(&(ch->lock));
	free(ch);

	return 0;
}

int tmpi_intra_init(int np)
{
	int i, j;

	if (np<=0)  {
		tmpi_error(DBG_CALLER, "tmpi_intra_init: np<=0.");
		return -1;
	}
	
	tmpi_channel_size=np;
	tmpi_channel=(tmpi_Channel **)calloc(np, sizeof(tmpi_Channel *));

	if (!tmpi_channel) {
		tmpi_error(DBG_INTERNAL, "tmpi_intra_init: cannot allocate memory.");
		return -1;
	}

	for (i=0; i<np; i++) {
		tmpi_channel[i]=tmpi_intra_new_channel();
		if (!tmpi_channel[i]) {
			for (j=0; j<i; j++)
				tmpi_intra_del_channel(tmpi_channel[j]);
			free(tmpi_channel);
			tmpi_error(DBG_INTERNAL, "tmpi_intra_init: cannot allocate memory.");
			return -1;
		}
	}

	sa_init(&(tmpi_inter_msg), sizeof(tmpi_MSG), 65535);

	return 0;
}

void tmpi_intra_end()
{
	int i;

	sa_destroy(&(tmpi_inter_msg));

	for (i=0; i<tmpi_channel_size; i++) {
		if (tmpi_channel[i]) {
			tmpi_intra_del_channel(tmpi_channel[i]);
		}
	}

	free(tmpi_channel);

	tmpi_channel=NULL;
	tmpi_channel_size=0;
}

tmpi_MSG *tmpi_inter_new_msg()
{
	return (tmpi_MSG *)sa_alloc(&tmpi_inter_msg);
}

void tmpi_inter_free_msg(tmpi_MSG *msg)
{
	sa_free(&tmpi_inter_msg, msg);
}

static tmpi_SHandle *tmpi_intra_new_sh(tmpi_Channel *ch)
{
	tmpi_SHandle *sh;

	sh=(tmpi_SHandle *)Q_newElem(ch->sq);

	if (sh) {
		thr_mtx_init(&(sh->lock));
		thr_cnd_init(&(sh->wait));
	}

	return sh;
}
	
static tmpi_RHandle *tmpi_intra_new_rh(tmpi_Channel *ch)
{
	tmpi_RHandle *rh;

	rh=(tmpi_RHandle *)Q_newElem(ch->rq);
	if (rh) {
		thr_mtx_init(&(rh->lock));
		thr_cnd_init(&(rh->wait));
	}

	return rh;
}

void tmpi_intra_free_sh(tmpi_Channel *ch, tmpi_SHandle *sh)
{
	if (sh->sys_buf) {
		int local_id=tmpi_grank_to_lrank(sh->src);

		if (local_id>=0) {
			mb_free(local_id, sh->sys_buf);
		}
	}
	thr_mtx_destroy(&(sh->lock));
	thr_cnd_destroy(&(sh->wait));
	Q_freeElem(ch->sq, (void *)sh);
}

void tmpi_intra_free_rh(tmpi_Channel *ch, tmpi_RHandle *rh)
{
	thr_mtx_destroy(&(rh->lock));
	thr_cnd_destroy(&(rh->wait));
	Q_freeElem(ch->rq, (void *)rh);
}
	
tmpi_RMSHandle *tmpi_intra_new_rmsh(tmpi_Channel *ch)
{
	return (tmpi_RMSHandle *)Q_newElem(ch->sq);
}

static void tmpi_intra_free_rmsh(tmpi_Channel *ch, tmpi_RMSHandle *rmsh)
{
	if (rmsh->curdata) {
		int local_id=tmpi_grank_to_lrank(rmsh->dest);

		if (local_id>=0) {
			mb_free(local_id, rmsh->curdata);
		}
	}

	Q_freeElem(ch->sq, (void *)rmsh);
}

/* the found handle is unlinked from the queue */
tmpi_RHandle *tmpi_rq_find(tmpi_Channel *ch, int context, int tag, int src)
{
	return (tmpi_RHandle *)Q_find(ch->rq, context, tag, src);
}

static tmpi_SHandle *tmpi_sq_find(tmpi_Channel *ch, int context, int tag, int src)
{
	return (tmpi_SHandle *)Q_find(ch->sq, context, tag, src);
}

/* actually the return handle could be tmpi_RMSHandle */
void tmpi_sq_enq(tmpi_Channel *ch, tmpi_SHandle *sh)
{

	enQ(ch->sq, (void *)sh);
}

static void tmpi_rq_enq(tmpi_Channel *ch, tmpi_RHandle *rh)
{

	enQ(ch->rq, (void *)rh);
}

int tmpi_intra_init_sh
(
		MPI_Comm comm,
		char *buf,
		int from,
		int to,
		int tag,
		MPI_Datatype datatype,
		int count,
		tmpi_Mode mode,
		int isPersistent,
		int isImmediate,
		MPI_Request *psh
)
{
	int local_id=tmpi_grank_to_lrank(from);
	tmpi_Channel *ch=tmpi_intra_get_channel(local_id);
	tmpi_SHandle *sh;
	tmpi_Comm *tcomm=tmpi_comm_verify(comm, local_id);
	int len=tmpi_Usize(datatype)*count;

	if (psh)
		*psh=MPI_REQUEST_NULL;
	else {
		tmpi_error(DBG_CALLER, "pointer to shandle is <nul>.");
		return MPI_ERR_INTERN;
	}

	if (!ch) {
		tmpi_error(DBG_INTERNAL, "cannot get channel object.");
		return MPI_ERR_INTERN;
	}

	if (len<0) {
		tmpi_error(DBG_CALLER, "parameter error, datatype or count.");
		return MPI_ERR_INTERN;
	}

	if (!tcomm) {
		tmpi_error(DBG_INTERNAL, "cannot get tmpi_comm object.");
		return MPI_ERR_COMM;
	}

	if ((sh=tmpi_intra_new_sh(ch))==NULL) {
		tmpi_error(DBG_INTERNAL, "cannot create new shandle.");
		return MPI_ERR_INTERN;
	}

	if (isPersistent)
		sh->handle_type=MPI_PERSISTENT_SEND;
	else if (isImmediate)
		sh->handle_type=MPI_IMMEDIATE_SEND;
	else
		sh->handle_type=MPI_SEND;

	sh->cookie=MPI_REQUEST_COOKIE;
	sh->stat=TMPI_CREATE;
	sh->context=tcomm->pt2pt_context;
	sh->tag=tag;
	sh->src=from;
	sh->dest=to;
	sh->comm=comm;

	sh->curdata=sh->user_buf=buf;
	sh->sys_buf=NULL;
	sh->buflen=len;
	sh->count=count;
	sh->datatype=datatype;
	sh->send_mode=mode;

	if (netq_isLocalNode(to))
		sh->isRemote=0;
	else
		sh->isRemote=1;

	*psh=(MPI_Request)sh;

	/* correctly handle 0 bytes message */
	if (mode&(STANDARD|BUFFERED)) {
		if (len>0)
			sh->sys_buf=mb_alloc(local_id, len);
	}

	if ((mode==BUFFERED)&&(sh->sys_buf==NULL)&&(len>0)) {
		tmpi_error(DBG_INTERNAL, "sh_init: mb_alloc failed for buffered send.");
		sh->errval=MPI_ERR_BUFFER;
		return MPI_ERR_BUFFER;
	}

	sh->errval=MPI_SUCCESS;
	return MPI_SUCCESS;
}

int tmpi_intra_init_rh
(
		MPI_Comm comm,
		char *buf,
		int from,
		int to,
		int tag,
		MPI_Datatype datatype,
		int count,
		int isPersistent,
		int isImmediate,
		MPI_Request *prh
)
{
	int local_id=tmpi_grank_to_lrank(to);
	tmpi_Channel *ch=tmpi_intra_get_channel(local_id);
	tmpi_RHandle *rh;
	tmpi_Comm *tcomm=tmpi_comm_verify(comm, local_id);
	int len=tmpi_Usize(datatype)*count;

	if (prh)
		*prh=MPI_REQUEST_NULL;
	else {
		tmpi_error(DBG_CALLER, "pointer to rhandle is <nul>.");
		return MPI_ERR_INTERN;
	}

	if (!ch) {
		tmpi_error(DBG_INTERNAL, "cannot get channel object.");
		return MPI_ERR_INTERN;
	}

	if (len<0) {
		tmpi_error(DBG_CALLER, "parameter error, datatype or count.");
		return MPI_ERR_INTERN;
	}

	if (!tcomm) {
		tmpi_error(DBG_INTERNAL, "cannot get tmpi_comm object.");
		return MPI_ERR_COMM;
	}

	if ((rh=tmpi_intra_new_rh(ch))==NULL) {
		tmpi_error(DBG_INTERNAL, "cannot create new rhandle.");
		return MPI_ERR_INTERN;
	}

	if (isPersistent) {
		rh->handle_type=MPI_PERSISTENT_RECV;
		rh->perm_source=from;
		rh->perm_tag=tag;
		rh->perm_len=len;
	}
	else if (isImmediate)
		rh->handle_type=MPI_IMMEDIATE_RECV;
	else
		rh->handle_type=MPI_RECV;

	rh->cookie=MPI_REQUEST_COOKIE;
	rh->stat=TMPI_CREATE;
	rh->context=tcomm->pt2pt_context;
	rh->tag=tag;
	rh->src=from;
	rh->dest=to;
	rh->comm=comm;

	rh->data=buf;
	rh->buflen=len;
	rh->count=count;
	rh->datatype=datatype;

	*prh=(MPI_Request)rh;

	rh->errval=MPI_SUCCESS;
	return MPI_SUCCESS;
}

static int tmpi_inter_start_sh(tmpi_SHandle **psh)
{
	/* ???? sh can be in CANCEL state */
	/**
	 * first if psh is in CANCEL and is a persistent handle, create a new handle and put it into
	 * psh.
	 */

	tmpi_SHandle *sh=(*psh);
	tmpi_MSG *request;

	if ( (sh->stat!=TMPI_CREATE) && ((sh->stat!=TMPI_FREE)||(sh->handle_type!=MPI_PERSISTENT_SEND))) {
		tmpi_error(DBG_CALLER, "request handle is not in a startable state.");
		return MPI_ERR_INTERN;
	}

	request=tmpi_inter_new_msg();
	if (!request) {
		tmpi_error(DBG_INTERNAL, "cannot create inter-node comm message.");
		return MPI_ERR_INTERN;
	}

	sh->stat=TMPI_PEND_STRT;

	memcpy((char *)&(request->body.rmshandle), (char *)sh, sizeof(tmpi_RMSHandle));
	request->body.rmshandle.remote_send_handle=sh;
	request->body.rmshandle.curdata=NULL;

	if (sh->buflen<TMPI_MAX_SHORT_MSG) {
		/* do short message protocol */
		request->body.rmshandle.handle_type=MPI_REMOTE_MSGDAT_SEND;
		netq_send(sh->src, sh->dest, (char *)request, sizeof(tmpi_MSG), 
				sh->curdata, sh->buflen, NETQ_BLK);
	}
	else {
		/* do long message protocol */
		request->body.rmshandle.handle_type=MPI_REMOTE_MSG_SEND;
		netq_send(sh->src, sh->dest, (char *)request, sizeof(tmpi_MSG), NULL, 0, NETQ_BLK);
	}

	/**
	if (sh->src==0) {
		tmpi_dump_handle(DBG_INTERNAL, &(request->body));
	}
	**/

	tmpi_inter_free_msg(request);

	return MPI_SUCCESS;
}

int tmpi_intra_start_sh(tmpi_SHandle **psh)
{
	tmpi_Channel *ch;
	int local_recv_id;
	tmpi_RHandle *rh;
	tmpi_SHandle *sh=*psh;

	if (!sh)
		return MPI_ERR_REQUEST;

	if (sh->errval!=MPI_SUCCESS) {
		tmpi_error(DBG_INTERNAL, "cannot start request due to internal errors, errval=%d.", sh->errval);
		return MPI_ERR_INTERN;
	}

	if (sh->isRemote) {
		return tmpi_inter_start_sh(psh);
	}

	/* intra sh start */
	if ( (sh->stat!=TMPI_CREATE) && ((sh->stat!=TMPI_FREE)||(sh->handle_type!=MPI_PERSISTENT_SEND))) {
		tmpi_error(DBG_CALLER, "request handle is not in a startable state.");
		return MPI_ERR_INTERN;
	}

	local_recv_id=tmpi_grank_to_lrank(sh->dest);
	if ((ch=tmpi_intra_get_channel(local_recv_id))==NULL) {
		tmpi_error(DBG_INTERNAL, "sh intra start, cannot get channel object.");
		return MPI_ERR_INTERN;
	}

	thr_mtx_lock(&(ch->lock));

	rh=tmpi_rq_find(ch, sh->context, sh->tag, sh->src);

	if (!rh) {
		sh->stat=TMPI_PEND_STRT;
		tmpi_sq_enq(ch, sh);
	}
	else {
		sh->stat=TMPI_MATCH;
		thr_mtx_lock(&(rh->lock));
		rh->src=sh->src;
		rh->tag=sh->tag;
		rh->buflen=MIN(sh->buflen, rh->buflen);
		if (rh->buflen)
			memcpy(rh->data, sh->curdata, rh->buflen);
		sh->buflen=rh->buflen;
		sh->errval=MPI_SUCCESS;
		if (rh->stat==TMPI_PEND_WAIT) {
			rh->stat=TMPI_MATCH;
			thr_cnd_signal(&(rh->wait));
		}
		else if (rh->stat!=TMPI_PEND_STRT) {
			tmpi_error(DBG_INTERNAL, "protocol error, rh stat=%d in queue.", rh->stat);
		}
		else {
			rh->stat=TMPI_MATCH;
		}
		thr_mtx_unlock(&(rh->lock));
	}
	thr_mtx_unlock(&(ch->lock));

	return MPI_SUCCESS;
}

int tmpi_intra_start_rh(tmpi_RHandle **prh)
{
	tmpi_Channel *ch;
	int local_id;
	tmpi_SHandle *sh;
	int unlockit=1;
	tmpi_RHandle *rh=(*prh);

	/* might need to substitute old handle */
	/* ... */
	if (!rh)
		return MPI_ERR_REQUEST;

	if (rh->errval!=MPI_SUCCESS) {
		tmpi_error(DBG_INTERNAL, "cannot start request due to internal errors, errval=%d.", rh->errval);
		return MPI_ERR_INTERN;
	}

	if ( (rh->stat!=TMPI_CREATE) && ((rh->stat!=TMPI_FREE)||(rh->handle_type!=MPI_PERSISTENT_SEND))) {
		tmpi_error(DBG_CALLER, "request handle is not in a startable state.");
		return MPI_ERR_INTERN;
	}

	local_id=tmpi_grank_to_lrank(rh->dest);
	if ((ch=tmpi_intra_get_channel(local_id))==NULL) {
		tmpi_error(DBG_INTERNAL, "rh intra start, cannot get channel object.");
		return MPI_ERR_INTERN;
	}

	thr_mtx_lock(&(ch->lock));
	sh=tmpi_sq_find(ch, rh->context, rh->tag, rh->src);
	if (!sh) {
		rh->stat=TMPI_PEND_STRT;
		tmpi_rq_enq(ch, rh);
	}
	else {
		if (sh->handle_type&(MPI_SEND|MPI_PERSISTENT_SEND|MPI_IMMEDIATE_SEND)) {
			thr_mtx_lock(&(sh->lock));
			rh->src=sh->src;
			rh->tag=sh->tag;
			rh->buflen=MIN(sh->buflen, rh->buflen);
			sh->buflen=rh->buflen;
			/* intra send handle */
			rh->stat=TMPI_MATCH;
			if (rh->buflen)
				memcpy(rh->data, sh->curdata, rh->buflen);
			if (sh->stat==TMPI_PEND_WAIT) {
				sh->stat=TMPI_MATCH;
				thr_cnd_signal(&(sh->wait));
			}
			else if (sh->stat==TMPI_PEND_STRT) {
				sh->stat=TMPI_MATCH;
			}
			else if (sh->stat==TMPI_PEND_RELS) {
				sh->stat=TMPI_FREE;
				sh->errval=MPI_SUCCESS;
				if (sh->handle_type==MPI_PERSISTENT_SEND) {
					sh->curdata=sh->user_buf;
					sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
					thr_mtx_unlock(&(sh->lock));
				}
				else {
					tmpi_Channel *peer_ch;

					peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
					thr_mtx_unlock(&(sh->lock));
					if (peer_ch)
						tmpi_intra_free_sh(peer_ch, sh);
					else 
						tmpi_error(DBG_INTERNAL, "cannot get channel object.");

				}
				unlockit=0;
			}
			else {
				tmpi_error(DBG_INTERNAL, "protocol error, sh stat=%d in queue.", sh->stat);
			}

			if (unlockit)
				thr_mtx_unlock(&(sh->lock));
		}
		else {
			tmpi_RMSHandle *rmsh=(tmpi_RMSHandle *)sh;
			if (!(rmsh->handle_type&(MPI_REMOTE_MSGDAT_SEND|MPI_REMOTE_MSG_SEND))) {
				tmpi_error(DBG_INTERNAL, "protocol error, rmsh stat=%d in queue.", rmsh->stat);
			}
			else {
				tmpi_MSG *response=tmpi_inter_new_msg();
				if (!response) {
					tmpi_error(DBG_FATAL, "cannot create inter node message.");
				}
				rh->src=rmsh->src;
				rh->tag=rmsh->tag;
				rh->buflen=MIN(rmsh->buflen, rh->buflen);
				if (rmsh->handle_type==MPI_REMOTE_MSGDAT_SEND) {
					if (rh->buflen)
						memcpy(rh->data, rmsh->curdata, rh->buflen);
					rh->stat=TMPI_MATCH;
					if (response) {
						response->body.rdrhandle.handle_type=MPI_REMOTE_MSGDAT_RECV;
						response->body.rdrhandle.local_send_handle=rmsh->remote_send_handle;
						response->body.rdrhandle.errval=MPI_SUCCESS;
						response->body.rdrhandle.buflen=rh->buflen;
						netq_send(rh->dest, rh->src, (char *)response, 
								sizeof(tmpi_MSG), NULL, 0, NETQ_BLK);
						tmpi_inter_free_msg(response);
					}
				} 
				else /* MPI_REMOTE_MSG_SEND */ {
					rh->stat=TMPI_PEND_DATA;
					if (response) {
						response->body.rmrhandle.handle_type=MPI_REMOTE_MSG_RECV;
						response->body.rmrhandle.local_send_handle=rmsh->remote_send_handle;
						response->body.rmrhandle.remote_recv_handle=rh;
						response->body.rmrhandle.errval=MPI_SUCCESS;
						response->body.rmrhandle.buflen=rh->buflen;
						netq_send(rh->dest, rh->src, (char *)response, 
								sizeof(tmpi_MSG), NULL, 0, NETQ_BLK);
						tmpi_inter_free_msg(response);
					}
				}
			}
			rmsh->stat=TMPI_FREE;
			tmpi_intra_free_rmsh(ch, rmsh);
		}
	}
	thr_mtx_unlock(&(ch->lock));

	return MPI_SUCCESS;
}

static int tmpi_inter_wait_sh(tmpi_SHandle **psh, tmpi_Status *status)
{
	tmpi_SHandle *sh=*psh;
	int ret=MPI_SUCCESS;

	if (!(sh->stat&(TMPI_PEND_STRT|TMPI_MATCH|TMPI_PEND_NRDY|TMPI_PEND_DATA))) {
		tmpi_error(DBG_INTERNAL, "protocol error, sh stat=%d in inter_sh_wait.", sh->stat);
		return MPI_ERR_INTERN;
	}

	thr_mtx_lock(&(sh->lock));
	if ((sh->stat==TMPI_PEND_STRT)&&(sh->send_mode==READY)) {
		do {
			thr_cnd_wait(&(sh->wait), &(sh->lock));
		} while (sh->stat==TMPI_PEND_STRT);
	}

	if (sh->stat==TMPI_MATCH) {
		if (status) {
			status->buflen=sh->buflen;
			status->MPI_SOURCE=sh->src;
			status->MPI_TAG=sh->tag;
			status->MPI_ERROR=sh->errval;
		}
		sh->stat=TMPI_FREE;
		sh->errval=MPI_SUCCESS;
		if (sh->handle_type!=MPI_PERSISTENT_SEND) {
			tmpi_Channel *peer_ch;

			peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
			thr_mtx_unlock(&(sh->lock));
			if (peer_ch)
				tmpi_intra_free_sh(peer_ch, sh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
		}
		else {
			sh->curdata=sh->user_buf;
			sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
			thr_mtx_unlock(&(sh->lock));
		}
	}
	else if (sh->stat==TMPI_PEND_NRDY) {
		if (sh->send_mode!=READY) {
			tmpi_error(DBG_INTERNAL, "protocol error, non-ready handle in NRDY state.");
		}

		/* repeat the above MATCH block */
		if (status) {
			status->buflen=sh->buflen;
			status->MPI_SOURCE=sh->src;
			status->MPI_TAG=sh->tag;
			status->MPI_ERROR=MPI_ERR_NRDY;
		}

		ret=MPI_ERR_NRDY;

		sh->stat=TMPI_FREE;
		sh->errval=MPI_SUCCESS;
		if (sh->handle_type!=MPI_PERSISTENT_SEND) {
			tmpi_Channel *peer_ch;

			peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
			thr_mtx_unlock(&(sh->lock));
			if (peer_ch)
				tmpi_intra_free_sh(peer_ch, sh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
		}
		else {
			sh->curdata=sh->user_buf;
			sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
			thr_mtx_unlock(&(sh->lock));
		}
	}
	else {
		switch (sh->send_mode) {
			case (SYNC) :
			case (READY) :
_INTER_SYNC:	
				if (sh->stat==TMPI_PEND_STRT) {
					sh->stat=TMPI_PEND_WAIT;
				}
				else {
					sh->stat=TMPI_PEND_DWAT;
				}

				do {
					thr_cnd_wait(&(sh->wait), &(sh->lock));
				} while (sh->stat!=TMPI_MATCH);

				if (status) {
					status->buflen=sh->buflen;
					status->MPI_SOURCE=sh->src;
					status->MPI_TAG=sh->tag;
					status->MPI_ERROR=sh->errval;
				}

				sh->stat=TMPI_FREE;
				sh->errval=MPI_SUCCESS;

				if (sh->handle_type!=MPI_PERSISTENT_SEND) {
					tmpi_Channel *peer_ch;

					peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
					thr_mtx_unlock(&(sh->lock));
					if (peer_ch)
						tmpi_intra_free_sh(peer_ch, sh);
					else 
						tmpi_error(DBG_INTERNAL, "cannot get channel object.");

					*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
				}
				else {
					sh->curdata=sh->user_buf;
					sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
					thr_mtx_unlock(&(sh->lock));
				}

				break;

			case (BUFFERED) :
_INTER_BUFFER:	if (status) {
					status->buflen=sh->buflen;
					status->MPI_SOURCE=sh->src;
					status->MPI_TAG=sh->tag;
					status->MPI_ERROR=sh->errval;
				}

				if (sh->curdata==sh->user_buf) {
					if (sh->sys_buf) {
						memcpy((char *)(sh->sys_buf), (char  *)(sh->user_buf), sh->buflen);
						sh->curdata=sh->sys_buf;
					}
					else {
						ret=MPI_ERR_REQUEST;
						if (status)
							status->MPI_ERROR=MPI_ERR_BUFFER;
					}
				}

				sh->stat=TMPI_PEND_RELS;

				if (sh->handle_type!=MPI_PERSISTENT_SEND)
					*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;

				thr_mtx_unlock(&(sh->lock));
				break;

			case (STANDARD) :
				if (sh->sys_buf)
					goto _INTER_BUFFER;
				else
					goto _INTER_SYNC;
			default :
				// nothing
				tmpi_error(DBG_INTERNAL, "unknow send mode=%d.", sh->send_mode);
				thr_mtx_unlock(&(sh->lock));
		}
	}

	return ret;
}

int tmpi_intra_wait_sh(tmpi_SHandle **psh, tmpi_Status *status)
{
	tmpi_SHandle *sh=*psh;
	int ret=MPI_SUCCESS;

	if (!sh)
		return MPI_ERR_REQUEST;

	if (sh->isRemote) {
		return tmpi_inter_wait_sh(psh, status);
	}

	if (!(sh->stat&(TMPI_PEND_STRT|TMPI_MATCH))) {
		tmpi_error(DBG_INTERNAL, "protocol error, sh stat=%d in intra_sh_wait.", sh->stat);
		return MPI_ERR_INTERN;
	}

	thr_mtx_lock(&(sh->lock));
	if (sh->stat==TMPI_MATCH) {
		if (status) {
			status->buflen=sh->buflen;
			status->MPI_SOURCE=sh->src;
			status->MPI_TAG=sh->tag;
			status->MPI_ERROR=sh->errval;
		}
		sh->stat=TMPI_FREE;
		sh->errval=MPI_SUCCESS;
		if (sh->handle_type!=MPI_PERSISTENT_SEND) {
			tmpi_Channel *peer_ch;

			peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
			thr_mtx_unlock(&(sh->lock));
			if (peer_ch)
				tmpi_intra_free_sh(peer_ch, sh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
		}
		else {
			sh->curdata=sh->user_buf;
			sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
			thr_mtx_unlock(&(sh->lock));
		}
	}
	else {
		switch (sh->send_mode) {
			case (SYNC) :
_INTRA_SYNC:	sh->stat=TMPI_PEND_WAIT;
				do {
					thr_cnd_wait(&(sh->wait), &(sh->lock));
				} while (sh->stat==TMPI_PEND_WAIT);

				if (sh->stat!=TMPI_MATCH) {
					tmpi_error(DBG_INTERNAL, "protocol error, sender wake up in stat %d.", sh->stat);
				}

				if (status) {
					status->buflen=sh->buflen;
					status->MPI_SOURCE=sh->src;
					status->MPI_TAG=sh->tag;
					status->MPI_ERROR=sh->errval;
				}

				sh->stat=TMPI_FREE;
				sh->errval=MPI_SUCCESS;

				if (sh->handle_type!=MPI_PERSISTENT_SEND) {
					tmpi_Channel *peer_ch;

					peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
					thr_mtx_unlock(&(sh->lock));
					if (peer_ch)
						tmpi_intra_free_sh(peer_ch, sh);
					else 
						tmpi_error(DBG_INTERNAL, "cannot get channel object.");

					*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
				}
				else {
					sh->curdata=sh->user_buf;
					sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
					thr_mtx_unlock(&(sh->lock));
				}

				break;

			case (BUFFERED) :
_INTRA_BUFFER:	if (status) {
					status->buflen=sh->buflen;
					status->MPI_SOURCE=sh->src;
					status->MPI_TAG=sh->tag;
					status->MPI_ERROR=sh->errval;
				}

				if (sh->curdata==sh->user_buf) {
					if (sh->sys_buf) {
						memcpy((char *)(sh->sys_buf), (char  *)(sh->user_buf), sh->buflen);
						sh->curdata=sh->sys_buf;
					}
					else {
						ret=MPI_ERR_REQUEST;
						if (status)
							status->MPI_ERROR=MPI_ERR_BUFFER;
					}
				}

				sh->stat=TMPI_PEND_RELS;

				if (sh->handle_type!=MPI_PERSISTENT_SEND)
					*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;

				thr_mtx_unlock(&(sh->lock));
				break;

			case (READY) :

				if (status) {
					status->buflen=sh->buflen;
					status->MPI_SOURCE=sh->src;
					status->MPI_TAG=sh->tag;
					status->MPI_ERROR=MPI_ERR_NRDY;
				}

				ret=MPI_ERR_NRDY;

				sh->stat=TMPI_FREE;
				sh->errval=MPI_SUCCESS;

				if (sh->handle_type!=MPI_PERSISTENT_SEND) {
					tmpi_Channel *peer_ch;

					peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
					thr_mtx_unlock(&(sh->lock));
					if (peer_ch)
						tmpi_intra_free_sh(peer_ch, sh);
					else 
						tmpi_error(DBG_INTERNAL, "cannot get channel object.");

					*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
				}
				else {
					sh->curdata=sh->user_buf;
					sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
					thr_mtx_unlock(&(sh->lock));
				}

				break;

			case (STANDARD) :
				if (sh->sys_buf)
					goto _INTRA_BUFFER;
				else
					goto _INTRA_SYNC;
			default :
				// nothing
				tmpi_error(DBG_INTERNAL, "unknow send mode=%d.", sh->send_mode);
				thr_mtx_unlock(&(sh->lock));
		}
	}

	return ret;
}

int tmpi_intra_wait_rh(tmpi_RHandle **prh, tmpi_Status *status)
{
	tmpi_RHandle *rh=*prh;
	int ret=MPI_SUCCESS;

	if (!rh)
		return MPI_ERR_REQUEST;

	if (!(rh->stat&(TMPI_PEND_STRT|TMPI_MATCH|TMPI_PEND_DATA))) {
		tmpi_error(DBG_INTERNAL, "protocol error, rh stat=%d in intra_rh_wait.", rh->stat);
		return MPI_ERR_INTERN;
	}

	thr_mtx_lock(&(rh->lock));
	if (rh->stat==TMPI_MATCH) {
		if (status) {
			status->buflen=rh->buflen;
			status->MPI_SOURCE=rh->src;
			status->MPI_TAG=rh->tag;
			status->MPI_ERROR=rh->errval;
		}
		rh->stat=TMPI_FREE;
		rh->errval=MPI_SUCCESS;
		if (rh->handle_type!=MPI_PERSISTENT_RECV) {
			tmpi_Channel *ch;

			ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(rh->dest));
			thr_mtx_unlock(&(rh->lock));
			if (ch)
				tmpi_intra_free_rh(ch, rh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*prh=(tmpi_RHandle *)MPI_REQUEST_NULL;
		}
		else {
			rh->tag=rh->perm_tag;
			rh->src=rh->perm_source;
			rh->buflen=rh->perm_len;
			thr_mtx_unlock(&(rh->lock));
		}
	}
	else {
		if (rh->stat==TMPI_PEND_STRT) {
			rh->stat=TMPI_PEND_WAIT;
		}
		else {
			rh->stat=TMPI_PEND_DWAT;
		}
		do {
			thr_cnd_wait(&(rh->wait), &(rh->lock));
		} while (rh->stat!=TMPI_MATCH);

		if (status) {
			status->buflen=rh->buflen;
			status->MPI_SOURCE=rh->src;
			status->MPI_TAG=rh->tag;
			status->MPI_ERROR=rh->errval;
		}

		rh->stat=TMPI_FREE;
		rh->errval=MPI_SUCCESS;
		if (rh->handle_type!=MPI_PERSISTENT_RECV) {
			tmpi_Channel *ch;

			ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(rh->dest));
			thr_mtx_unlock(&(rh->lock));
			if (ch)
				tmpi_intra_free_rh(ch, rh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*prh=(tmpi_RHandle *)MPI_REQUEST_NULL;
		}
		else {
			rh->tag=rh->perm_tag;
			rh->src=rh->perm_source;
			rh->buflen=rh->perm_len;
			thr_mtx_unlock(&(rh->lock));
		}
	}

	return ret;
}

static int tmpi_inter_cancel_sh(tmpi_SHandle **psh)
{
	tmpi_SHandle *sh=*psh;
	int ret=MPI_SUCCESS;

	if (!(sh->stat&(TMPI_PEND_STRT|TMPI_MATCH|TMPI_PEND_NRDY|TMPI_PEND_DATA))) {
		tmpi_error(DBG_INTERNAL, "protocol error, sh stat=%d in inter_sh_cancel.", sh->stat);
		return MPI_ERR_INTERN;
	}

	thr_mtx_lock(&(sh->lock));

	if (sh->stat==TMPI_MATCH) {
		sh->stat=TMPI_FREE;
		sh->errval=MPI_SUCCESS;
		if (sh->handle_type!=MPI_PERSISTENT_SEND) {
			tmpi_Channel *peer_ch;

			peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
			thr_mtx_unlock(&(sh->lock));
			if (peer_ch)
				tmpi_intra_free_sh(peer_ch, sh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
		}
		else {
			sh->curdata=sh->user_buf;
			sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
			thr_mtx_unlock(&(sh->lock));
		}
	}
	else if (sh->stat==TMPI_PEND_NRDY) {
		if (sh->send_mode!=READY) {
			tmpi_error(DBG_INTERNAL, "protocol error, non-ready handle in NRDY state.");
		}

		/* repeat the above MATCH block */
		sh->stat=TMPI_FREE;
		sh->errval=MPI_SUCCESS;
		if (sh->handle_type!=MPI_PERSISTENT_SEND) {
			tmpi_Channel *peer_ch;

			peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
			thr_mtx_unlock(&(sh->lock));
			if (peer_ch)
				tmpi_intra_free_sh(peer_ch, sh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
		}
		else {
			sh->curdata=sh->user_buf;
			sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
			thr_mtx_unlock(&(sh->lock));
		}
	}
	else /* TMPI_PEND_STRT or TMPI_PEND_DATA */ {
		sh->stat=TMPI_CANCEL;
		sh->errval=MPI_ERR_CANCEL;
		sh->curdata=tmpi_garbage_data;
		sh->buflen=MIN(sh->buflen, TMPI_GARBAGE_DATA_SIZE);
		thr_mtx_unlock(&(sh->lock));
	}

	return ret;
}

int tmpi_intra_cancel_sh(tmpi_SHandle **psh)
{
	tmpi_SHandle *sh=*psh;
	int ret=MPI_SUCCESS;

	if (!sh)
		return MPI_ERR_REQUEST;

	if (sh->isRemote)
		return tmpi_inter_cancel_sh(psh);

	if (!(sh->stat&(TMPI_PEND_STRT|TMPI_MATCH))) {
		tmpi_error(DBG_INTERNAL, "protocol error, sh stat=%d in intra_sh_cancel.", sh->stat);
		return MPI_ERR_INTERN;
	}

	thr_mtx_lock(&(sh->lock));

	sh->stat=TMPI_FREE;
	sh->errval=MPI_SUCCESS;
	if (sh->handle_type!=MPI_PERSISTENT_SEND) {
		tmpi_Channel *peer_ch;

		peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
		thr_mtx_unlock(&(sh->lock));
		if (peer_ch)
			tmpi_intra_free_sh(peer_ch, sh);
		else 
			tmpi_error(DBG_INTERNAL, "cannot get channel object.");

		*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
	}
	else {
		sh->curdata=sh->user_buf;
		sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
		thr_mtx_unlock(&(sh->lock));
	}

	return ret;
}

int tmpi_intra_cancel_rh(tmpi_RHandle **prh)
{
	tmpi_RHandle *rh=*prh;
	int ret=MPI_SUCCESS;

	if (!rh)
		return MPI_ERR_REQUEST;

	if (!(rh->stat&(TMPI_PEND_STRT|TMPI_MATCH|TMPI_PEND_DATA))) {
		tmpi_error(DBG_INTERNAL, "protocol error, rh stat=%d in intra_rh_cancel.", rh->stat);
		return MPI_ERR_INTERN;
	}

	thr_mtx_lock(&(rh->lock));

	if (rh->stat&(TMPI_MATCH|TMPI_PEND_STRT)) {
		rh->stat=TMPI_FREE;
		rh->errval=MPI_SUCCESS;
		if (rh->handle_type!=MPI_PERSISTENT_RECV) {
			tmpi_Channel *ch;

			ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(rh->dest));
			thr_mtx_unlock(&(rh->lock));
			if (ch)
				tmpi_intra_free_rh(ch, rh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*prh=(tmpi_RHandle *)MPI_REQUEST_NULL;
		}
		else {
			rh->tag=rh->perm_tag;
			rh->src=rh->perm_source;
			rh->buflen=rh->perm_len;
			thr_mtx_unlock(&(rh->lock));
		}
	}
	else /* TMPI_PEND_DATA */ {
		rh->stat=TMPI_CANCEL;
		thr_mtx_unlock(&(rh->lock));
	}

	return ret;
}

int tmpi_intra_test_sh(tmpi_SHandle **psh, int *flag, tmpi_Status *status)
{
	tmpi_SHandle *sh=*psh;
	int ret=MPI_SUCCESS;

	if (!sh) {
		*flag=0;
		return MPI_SUCCESS;
	}

	if (!(sh->stat&(TMPI_PEND_STRT|TMPI_MATCH|TMPI_PEND_NRDY|TMPI_PEND_DATA))) {
		tmpi_error(DBG_INTERNAL, "protocol error, sh stat=%d in intra_sh_test.", sh->stat);
		return MPI_ERR_INTERN;
	}

	thr_mtx_lock(&(sh->lock));

	if (sh->stat==TMPI_MATCH) {
		if (status) {
			status->buflen=sh->buflen;
			status->MPI_SOURCE=sh->src;
			status->MPI_TAG=sh->tag;
			status->MPI_ERROR=sh->errval;
		}
		sh->stat=TMPI_FREE;
		sh->errval=MPI_SUCCESS;
		if (sh->handle_type!=MPI_PERSISTENT_SEND) {
			tmpi_Channel *peer_ch;

			peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
			thr_mtx_unlock(&(sh->lock));
			if (peer_ch)
				tmpi_intra_free_sh(peer_ch, sh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
		}
		else {
			sh->curdata=sh->user_buf;
			sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
			thr_mtx_unlock(&(sh->lock));
		}

		*flag=1;
	}
	else if (sh->stat==TMPI_PEND_NRDY) {
		if (sh->send_mode!=READY) {
			tmpi_error(DBG_INTERNAL, "protocol error, non-ready handle in NRDY state.");
		}

		/* repeat the above MATCH block */
		if (status) {
			status->buflen=sh->buflen;
			status->MPI_SOURCE=sh->src;
			status->MPI_TAG=sh->tag;
			status->MPI_ERROR=sh->errval;
		}
		sh->stat=TMPI_FREE;
		sh->errval=MPI_SUCCESS;
		if (sh->handle_type!=MPI_PERSISTENT_SEND) {
			tmpi_Channel *peer_ch;

			peer_ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
			thr_mtx_unlock(&(sh->lock));
			if (peer_ch)
				tmpi_intra_free_sh(peer_ch, sh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
		}
		else {
			sh->curdata=sh->user_buf;
			sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
			thr_mtx_unlock(&(sh->lock));
		}

		*flag=1;
	}
	else /* TMPI_PEND_DATA or TMPI_PEND_STRT */ {
		switch (sh->send_mode) {
			case (SYNC) :
			case (READY) :
_TEST_SYNC:	
				*flag=0;
				break;

			case (BUFFERED) :
_TEST_BUFFER:	if (status) {
					status->buflen=sh->buflen;
					status->MPI_SOURCE=sh->src;
					status->MPI_TAG=sh->tag;
					status->MPI_ERROR=sh->errval;
				}

				if (sh->curdata==sh->user_buf) {
					if ( (sh->sys_buf) && (sh->buflen) ) {
						memcpy((char *)(sh->sys_buf), (char  *)(sh->user_buf), sh->buflen);
						sh->curdata=sh->sys_buf;
					}
					else {
						ret=MPI_ERR_REQUEST;
						if (status)
							status->MPI_ERROR=MPI_ERR_BUFFER;
					}
				}

				sh->stat=TMPI_PEND_RELS;

				if (sh->handle_type!=MPI_PERSISTENT_SEND)
					*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;

				thr_mtx_unlock(&(sh->lock));
				*flag=1;
				break;

			case (STANDARD) :
				if (sh->sys_buf)
					goto _TEST_BUFFER;
				else
					goto _TEST_SYNC;
			default :
				// nothing
				tmpi_error(DBG_INTERNAL, "unknow send mode=%d.", sh->send_mode);
				*flag=0;
				thr_mtx_unlock(&(sh->lock));
		}
	}

	return ret;
}

int tmpi_intra_test_rh(tmpi_RHandle **prh, int *flag, tmpi_Status *status)
{
	tmpi_RHandle *rh=*prh;
	int ret=MPI_SUCCESS;

	if (!rh) {
		*flag=0;
		return MPI_ERR_REQUEST;
	}

	if (!(rh->stat&(TMPI_PEND_STRT|TMPI_MATCH|TMPI_PEND_DATA))) {
		tmpi_error(DBG_INTERNAL, "protocol error, rh stat=%d in intra_rh_rest.", rh->stat);
		return MPI_ERR_INTERN;
	}

	thr_mtx_lock(&(rh->lock));
	if (rh->stat==TMPI_MATCH) {
		if (status) {
			status->buflen=rh->buflen;
			status->MPI_SOURCE=rh->src;
			status->MPI_TAG=rh->tag;
			status->MPI_ERROR=rh->errval;
		}
		rh->stat=TMPI_FREE;
		rh->errval=MPI_SUCCESS;
		if (rh->handle_type!=MPI_PERSISTENT_RECV) {
			tmpi_Channel *ch;

			ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(rh->dest));
			thr_mtx_unlock(&(rh->lock));
			if (ch)
				tmpi_intra_free_rh(ch, rh);
			else 
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");

			*prh=(tmpi_RHandle *)MPI_REQUEST_NULL;
		}
		else {
			rh->tag=rh->perm_tag;
			rh->src=rh->perm_source;
			rh->buflen=rh->perm_len;
			thr_mtx_unlock(&(rh->lock));
		}

		*flag=1;
	}
	else {
		thr_mtx_unlock(&(rh->lock));
		*flag=0;
	}

	return ret;
}

/**
 * probe and iprobe hasn't be put into the system yet.
 * I need to add one more handle type: PROBE.
 * Both probe and iprobe acts like recv and irecv from
 * the end user's point of view. The difference is
 * that xprobe doesn't take away the message while xrecv
 * does.
 *
 * However, the implementation is completely different.
 * For iprobe, things are easy, lock the queue, do a probe,
 * if found matching handle, return true, else, return false.
 * For probe request, the first checking is the same, but
 * if no matching send handle found, it will enqueue a 
 * PENDING PROBE handle in the receive queue and then the
 * caller will be blocked on that handle.
 * When a matching sending request comes and finds the PROBE
 * handle, it moves the handle out of the queue, and change
 * its state to FREE. At the same time, the sender acts as if
 * it fails the probe and link the send handle in the queue.
 *
 * The reason they can be simplified is because for probe,
 * comparing with recv, you always have wait after start
 * and you cannot cancel a probe request.
 *
 * To implement it, I need to change inter_sr.c in the
 * RMS, RMDS handling function (instead of find, I need to
 * do a separate probe and removeElem)i and in intra_sr.c,
 * change the intra send message handling in a similar way.
 *
 * The existing receive code doesn't need any change.
 */

/**
 * The current cancel code doesn't work the way as MPI specified.
 * I thought one can not do wait after a cancel. But actually
 * in MPI, one needs to do a completion call after cancel to
 * deallocate the resource.
 *
 * To support that, I need to add two more states: CANCEL_DONE and
 * CANCEL_FAIL. All cancel will eventually go to either state 
 * instead of FREE state. The new state graph reflects these changes,
 * there should be several places in inter_sr.c where I should
 * not free the handle, and move the state to CANCEL_DONE or CANCEL_FAIL.
 */
