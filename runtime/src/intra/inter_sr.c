# include <stdio.h>
# include "mbuffer.h"
# include "mpi_def.h"
# include "tmpi_struct.h"
# include "tmpi_debug.h"
# include "intra_sr.h"
# include "netq.h"
# include "netq_comm.h"
# include "comm.h"
# include "tmpi_util.h"
# include "thread.h"

# ifndef MIN
# define MIN(a,b) (((a)>(b))?(b):(a))
# endif

static char * dump_send_mode(int smode)
{
	switch (smode) {
		case STANDARD:
			return "STANDARD";
		case READY:
			return "READY";
		case SYNC:
			return "SYNC";
		case BUFFERED:
			return "BUFFERED";
		default:
			return "<invalid>";
	}
}

static char * dump_handle_stat(int stat)
{
	switch (stat) {
		case TMPI_CREATE:
			return "TMPI_CREATE";
		case TMPI_PEND:
			return "TMPI_PEND/TMPI_PEND_STRT";
		case TMPI_WAIT:
			return "TMPI_WAIT";
		case TMPI_PEND_WAIT:
			return "TMPI_PEND_WAIT";
		case TMPI_RELS:
			return "TMPI_RELS";
		case TMPI_PEND_RELS:
			return "TMPI_PEND_RELS";
		case TMPI_DATA:
			return "TMPI_DATA";
		case TMPI_PEND_DATA:
			return "TMPI_PEND_DATA";
		case TMPI_PEND_DWAT:
			return "TMPI_PEND_DWAT";
		case TMPI_NRDY:
			return "TMPI_NRDY";
		case TMPI_MATCH:
			return "TMPI_MATCH";
		case TMPI_CANCEL:
			return "TMPI_CANCEL";
		case TMPI_FREE:
			return "TMPI_FREE";
		case TMPI_ERR:
			return "TMPI_ERR";
		default:
			return "<unknown>";
	}
}

static char * dump_handle_type(type)
{
	switch (type) {
		case MPI_SEND:
			return "MPI_SEND";
		case MPI_RECV:
			return "MPI_RECV";
		case MPI_PERSISTENT_SEND:
			return "MPI_PERSISTENT_SEND";
		case MPI_PERSISTENT_RECV:
			return "MPI_PERSISTENT_RECV";
		case MPI_IMMEDIATE_SEND:
			return "MPI_IMMEDIATE_SEND";
		case MPI_IMMEDIATE_RECV:
			return "MPI_IMMEDIATE_RECV";
		case MPI_REMOTE_MSG_SEND:
			return "MPI_REMOTE_MSG_SEND";
		case MPI_REMOTE_DAT_SEND:
			return "MPI_REMOTE_DAT_SEND";
		case MPI_REMOTE_MSGDAT_SEND:
			return "MPI_REMOTE_MSGDAT_SEND";
		case MPI_REMOTE_MSG_RECV:
			return "MPI_REMOTE_MSG_RECV";
		case MPI_REMOTE_DAT_RECV:
			return "MPI_REMOTE_DAT_RECV";
		case MPI_REMOTE_MSGDAT_RECV:
			return "MPI_REMOTE_MSGDAT_RECV";
		case MPI_REMOTE_NRDY:
			return "MPI_REMOTE_NRDY";
		case MPI_PROBE:
			return "MPI_PROBE";
		default:
			return "<unknown>";
	}
}

static void dump_send_handle(tmpi_SHandle*handle)
{
	printf("cookie\t:%08lx\n", handle->cookie);
	printf("stat\t:%s\n", dump_handle_stat(handle->stat));
	printf("context\t:%08lx\n", handle->context);
	printf("tag\t:%d\n", handle->tag);
	printf("source\t:%d\n", handle->src);
	printf("destiny\t:%d\n", handle->dest);
	printf("count\t:%d\n", handle->count);
	printf("buflen\t:%d\n", handle->buflen);
	printf("mode\t:%s\n", dump_send_mode(handle->send_mode));
	printf("curbuf\t:%p\n", handle->curdata);
	printf("usrbuf\t:%p\n", handle->user_buf);
	printf("sysbuf\t:%p\n", handle->sys_buf);
	printf("errval\t:%d\n", handle->errval);
}

static void dump_recv_handle(tmpi_RHandle*handle)
{
	printf("cookie\t:%08lx\n", handle->cookie);
	printf("stat\t:%s\n", dump_handle_stat(handle->stat));
	printf("context\t:%08lx\n", handle->context);
	printf("tag\t:%d\n", handle->tag);
	printf("source\t:%d\n", handle->src);
	printf("destiny\t:%d\n", handle->dest);
	printf("count\t:%d\n", handle->count);
	printf("buflen\t:%d\n", handle->buflen);
	printf("datbuf\t:%p\n", handle->data);
	printf("errval\t:%d\n", handle->errval);
}

static void dump_rms_handle(tmpi_RMSHandle*handle)
{
	printf("cookie\t:%08lx\n", handle->cookie);
	printf("stat\t:%s\n", dump_handle_stat(handle->stat));
	printf("context\t:%08lx\n", handle->context);
	printf("tag\t:%d\n", handle->tag);
	printf("source\t:%d\n", handle->src);
	printf("destiny\t:%d\n", handle->dest);
	printf("count\t:%d\n", handle->count);
	printf("buflen\t:%d\n", handle->buflen);
	printf("mode\t:%s\n", dump_send_mode(handle->send_mode));
	printf("curbuf\t:%p\n", handle->curdata);
	printf("rem_sh\t:%p\n", handle->remote_send_handle);
	printf("errval\t:%d\n", handle->errval);
}

static void dump_rds_handle(tmpi_RDSHandle*handle)
{
	printf("rem_sh\t:%p\n", handle->remote_send_handle);
	printf("loc_rh\t:%p\n", handle->local_recv_handle);
	printf("buflen\t:%d\n", handle->buflen);
	printf("errval\t:%d\n", handle->errval);
}

static void dump_rmr_handle(tmpi_RMRHandle*handle)
{
	printf("rem_sh\t:%p\n", handle->local_send_handle);
	printf("loc_rh\t:%p\n", handle->remote_recv_handle);
	printf("buflen\t:%d\n", handle->buflen);
	printf("errval\t:%d\n", handle->errval);
}

static void dump_rdr_handle(tmpi_RDRHandle*handle)
{
	printf("rem_sh\t:%p\n", handle->local_send_handle);
	printf("buflen\t:%d\n", handle->buflen);
	printf("errval\t:%d\n", handle->errval);
}

void tmpi_dump_handle(int dbg_level, void *h)
{
	static thr_mtx_t thr;
	tmpi_Handle *handle=(tmpi_Handle *)h;

	thr_mtx_lock(&thr);

	printf("Handle type = %s\n", dump_handle_type(handle->type));
	switch (handle->type) {
		case MPI_SEND:
		case MPI_PERSISTENT_SEND:
		case MPI_IMMEDIATE_SEND:
			dump_send_handle((tmpi_SHandle*)handle);
			break;
		case MPI_RECV:
		case MPI_PERSISTENT_RECV:
		case MPI_IMMEDIATE_RECV:
			dump_recv_handle((tmpi_RHandle*)handle);
			break;
		case MPI_REMOTE_MSG_SEND:
		case MPI_REMOTE_MSGDAT_SEND:
			dump_rms_handle((tmpi_RMSHandle*)handle);
			break;
		case MPI_REMOTE_DAT_SEND:
			dump_rds_handle((tmpi_RDSHandle*)handle);
			break;
		case MPI_REMOTE_MSG_RECV:
			dump_rmr_handle((tmpi_RMRHandle*)handle);
			break;
		case MPI_REMOTE_DAT_RECV:
		case MPI_REMOTE_MSGDAT_RECV:
		case MPI_REMOTE_NRDY:
			dump_rdr_handle((tmpi_RDRHandle*)handle);
			break;
		case MPI_PROBE:
		default:
			printf("<unknown/unused>");
			break;
	}
	thr_mtx_unlock(&thr);
}

static void tmpi_dump_data(int from, int len)
{
	char dump[1024];
	int m;

	while (len>0) {
		m=MIN(len, 1024);
		if (netq_recvdata(from, dump, m)==0)
			len-=m;
		else
			// problem happens
			break;
	}
}

static int tmpi_rms_handle(tmpi_RMSHandle *handle)
{
	tmpi_Channel *ch;
	tmpi_RHandle *rh=NULL;
	tmpi_RMSHandle *rmsh=NULL;
	tmpi_MSG *response=NULL;

	if ((ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(handle->dest)))==NULL)
		return -1;

	response=tmpi_inter_new_msg();
	if (!response)
		return -1;

	thr_mtx_lock(&(ch->lock));
	rh=tmpi_rq_find(ch, handle->context, handle->tag, handle->src);
	if (rh) {
		/* found a matching rh */
		thr_mtx_lock(&(rh->lock));
		rh->tag=handle->tag;
		rh->src=handle->src;
		rh->buflen=MIN(rh->buflen, handle->buflen);
		if (rh->stat==TMPI_PEND_STRT) {
			rh->stat=TMPI_PEND_DATA;
		}
		else if (rh->stat==TMPI_PEND_WAIT) {
			rh->stat=TMPI_PEND_DWAT;
		}
		else {
			tmpi_error(DBG_INTERNAL, "protocol error, rh state=%d in queue got RMS.", rh->stat);
			tmpi_dump_handle(DBG_INTERNAL, rh);
		}
		thr_mtx_unlock(&(rh->lock));

		thr_mtx_unlock(&(ch->lock));

		response->body.rmrhandle.handle_type=MPI_REMOTE_MSG_RECV;
		response->body.rmrhandle.local_send_handle=handle->remote_send_handle;
		response->body.rmrhandle.remote_recv_handle=rh;
		response->body.rmrhandle.errval=MPI_SUCCESS;
		response->body.rmrhandle.buflen=rh->buflen;

		netq_send(handle->dest, handle->src, (char *)response, sizeof(tmpi_MSG), NULL, 0, NETQ_BLK);
	}
	else {
		if (handle->send_mode!=READY) {
			rmsh=tmpi_intra_new_rmsh(ch);
			memcpy((char *)rmsh, (char *)handle, sizeof(tmpi_RMSHandle));
			rmsh->stat=TMPI_PEND;
			rmsh->curdata=NULL;
			// lookup the comm object by context
			rmsh->comm=tmpi_comm_by_context(rmsh->context);
			tmpi_sq_enq(ch, (tmpi_SHandle *)rmsh);
		}

		thr_mtx_unlock(&(ch->lock));

		if (handle->send_mode==READY) {
			/* ready send, but the receiver is not ready */
			response->body.rdrhandle.handle_type=MPI_REMOTE_NRDY;
			response->body.rdrhandle.local_send_handle=handle->remote_send_handle;
			response->body.rdrhandle.errval=MPI_SUCCESS;
			response->body.rdrhandle.buflen=handle->buflen;

			netq_send(handle->dest, handle->src, (char *)response, sizeof(tmpi_MSG), NULL, 0, NETQ_BLK);
		}
	}

	tmpi_inter_free_msg(response);

	return 0;
}

static int tmpi_rmds_handle(tmpi_RMSHandle *handle)
{
	tmpi_Channel *ch;
	tmpi_RHandle *rh=NULL;
	int local_id;
	tmpi_RMSHandle *rmsh=NULL;
	tmpi_MSG *response=NULL;

	local_id=tmpi_grank_to_lrank(handle->dest);
	if ((ch=tmpi_intra_get_channel(local_id))==NULL)
		return -1;

	response=tmpi_inter_new_msg();
	if (!response)
		return -1;

	thr_mtx_lock(&(ch->lock));
	rh=tmpi_rq_find(ch, handle->context, handle->tag, handle->src);
	if (rh) {
		/* found a matching rh */
		thr_mtx_lock(&(rh->lock));
		rh->tag=handle->tag;
		rh->src=handle->src;
		rh->buflen=MIN(rh->buflen, handle->buflen);

		netq_recvdata(rh->src, rh->data, rh->buflen);
		if (rh->buflen<handle->buflen)
			tmpi_dump_data(rh->src, handle->buflen-rh->buflen);

		if (rh->stat==TMPI_PEND_STRT) {
			rh->stat=TMPI_MATCH;
		}
		else if (rh->stat==TMPI_PEND_WAIT) {
			rh->stat=TMPI_MATCH;
			thr_cnd_signal(&(rh->wait));
		}
		else {
			tmpi_error(DBG_INTERNAL, "protocol error, rh state=%d in queue got RMDS.", rh->stat);
			tmpi_dump_handle(DBG_INTERNAL, rh);
		}
		thr_mtx_unlock(&(rh->lock));

		thr_mtx_unlock(&(ch->lock));

		response->body.rdrhandle.handle_type=MPI_REMOTE_MSGDAT_RECV;
		response->body.rdrhandle.local_send_handle=handle->remote_send_handle;
		response->body.rdrhandle.errval=MPI_SUCCESS;
		response->body.rdrhandle.buflen=rh->buflen;

		netq_send(handle->dest, handle->src, (char *)response, sizeof(tmpi_MSG), NULL, 0, NETQ_BLK);
	}
	else {
		if (handle->send_mode!=READY) {
			rmsh=tmpi_intra_new_rmsh(ch);
			memcpy((char *)rmsh, (char *)handle, sizeof(tmpi_RMSHandle));
			rmsh->stat=TMPI_PEND;
			// lookup the comm object by context
			rmsh->comm=tmpi_comm_by_context(rmsh->context);
			rmsh->curdata=mb_alloc(local_id, rmsh->buflen);
			if (!rmsh->curdata) {
				/* downgrade to REMOTE_MSG_SEND */
				rmsh->handle_type=MPI_REMOTE_MSG_SEND;
				tmpi_dump_data(rmsh->src, rmsh->buflen);
			}
			else {
				// rmsh->handle_type=MPI_REMOTE_MSGDAT_SEND;
				netq_recvdata(rmsh->src, rmsh->curdata, rmsh->buflen);
			}
			tmpi_sq_enq(ch, (tmpi_SHandle *)rmsh);
		}

		thr_mtx_unlock(&(ch->lock));

		if (handle->send_mode==READY) {
			/* ready send, but the receiver is not ready */
			tmpi_dump_data(handle->src, handle->buflen);
			response->body.rdrhandle.handle_type=MPI_REMOTE_NRDY;
			response->body.rdrhandle.local_send_handle=handle->remote_send_handle;
			response->body.rdrhandle.errval=MPI_SUCCESS;
			response->body.rdrhandle.buflen=handle->buflen;

			netq_send(handle->dest, handle->src, (char *)response, sizeof(tmpi_MSG), NULL, 0, NETQ_BLK);
		}
	}

	tmpi_inter_free_msg(response);

	return 0;
}

static int tmpi_rds_handle(tmpi_RDSHandle *handle)
{
	tmpi_Channel *ch;
	tmpi_RHandle *rh=NULL;
	tmpi_MSG *response=NULL;
	int iscancelled=0;
	int src, dest;

	response=tmpi_inter_new_msg();
	if (!response)
		return -1;

	rh=handle->local_recv_handle;

	if (!rh)
		return -1;

	src=rh->src;
	dest=rh->dest;

	thr_mtx_lock(&(rh->lock));
	rh->buflen=MIN(rh->buflen, handle->buflen);

	if (rh->stat==TMPI_PEND_DATA) {
		netq_recvdata(src, rh->data, rh->buflen);
		if (rh->buflen<handle->buflen)
			tmpi_dump_data(src, handle->buflen-rh->buflen);

		rh->stat=TMPI_MATCH;
		thr_mtx_unlock(&(rh->lock));
	}
	else if (rh->stat==TMPI_PEND_DWAT) {
		netq_recvdata(src, rh->data, rh->buflen);
		if (rh->buflen<handle->buflen)
			tmpi_dump_data(src, handle->buflen-rh->buflen);

		rh->stat=TMPI_MATCH;
		thr_cnd_signal(&(rh->wait));
		thr_mtx_unlock(&(rh->lock));
	}
	else if (rh->stat==TMPI_CANCEL) {
		iscancelled=1;
		tmpi_dump_data(src, handle->buflen);
		rh->stat=TMPI_FREE;
		rh->errval=MPI_SUCCESS;
		if (rh->handle_type==MPI_PERSISTENT_RECV) {
			rh->tag=rh->perm_tag;
			rh->src=rh->perm_source;
			rh->buflen=rh->perm_len;
			thr_mtx_unlock(&(rh->lock));
		} else {
			ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(dest));
			thr_mtx_unlock(&(rh->lock));
			if (ch) {
				tmpi_intra_free_rh(ch, rh);
			}
			else {
				// sounds weird, just drop the space
			}
		}
	}
	else {
		thr_mtx_unlock(&(rh->lock));
		tmpi_error(DBG_INTERNAL, "protocol error, rh state=%d, got RDS.", rh->stat);
		tmpi_dump_handle(DBG_INTERNAL, rh);
	}

	response->body.rdrhandle.handle_type=MPI_REMOTE_DAT_RECV;
	response->body.rdrhandle.local_send_handle=handle->remote_send_handle;
	if (!iscancelled)
		response->body.rdrhandle.errval=MPI_SUCCESS;
	else
		response->body.rdrhandle.errval=MPI_ERR_CANCEL;

	response->body.rdrhandle.buflen=rh->buflen;

	netq_send(dest, src, (char *)response, sizeof(tmpi_MSG), NULL, 0, NETQ_BLK);

	return 0;
}

static int tmpi_rmr_handle(tmpi_RMRHandle *handle)
{
	tmpi_SHandle *sh=NULL;
	tmpi_MSG *response=NULL;

	response=tmpi_inter_new_msg();
	if (!response)
		return -1;

	sh=handle->local_send_handle;

	if (!sh)
		return -1;

	thr_mtx_lock(&(sh->lock));
	response->body.rdshandle.handle_type=MPI_REMOTE_DAT_SEND;
	response->body.rdshandle.remote_send_handle=sh;
	response->body.rdshandle.local_recv_handle=handle->remote_recv_handle;
	response->body.rdshandle.buflen=MIN(sh->buflen, handle->buflen);
	response->body.rdshandle.errval=MPI_SUCCESS;

	netq_send(sh->src, sh->dest, (char *)response, sizeof(tmpi_MSG), sh->curdata, 
			response->body.rdshandle.buflen, NETQ_BLK);

	if (sh->stat==TMPI_PEND_STRT) {
		sh->stat=TMPI_PEND_DATA;
		if (sh->send_mode==READY)
			thr_cnd_signal(&(sh->wait));
	}
	else if (sh->stat==TMPI_PEND_WAIT) {
		sh->stat=TMPI_PEND_DWAT;
	}
	else if ((sh->stat!=TMPI_CANCEL) && (sh->stat!=TMPI_PEND_RELS)) {
		tmpi_error(DBG_INTERNAL, "protocol error, sh(%p) state=%d got RMR.", sh, sh->stat);
		tmpi_dump_handle(DBG_INTERNAL, sh);
	}
	else {
		// do nothing for TMPI_CANCEL or TMPI_PEND_RELS
	}

	thr_mtx_unlock(&(sh->lock));

	return 0;
}

static int tmpi_rdr_handle(tmpi_RDRHandle *handle)
{
	tmpi_SHandle *sh=NULL;
	int unlockit=1;

	sh=handle->local_send_handle;

	if (!sh)
		return -1;

	thr_mtx_lock(&(sh->lock));
	sh->buflen=handle->buflen;
	sh->errval=handle->errval;

	if (sh->stat==TMPI_PEND_DATA) {
		sh->stat=TMPI_MATCH;
	}
	else if (sh->stat==TMPI_PEND_DWAT) {
		sh->stat=TMPI_MATCH;
		thr_cnd_signal(&(sh->wait));
	}
	else if ((sh->stat!=TMPI_CANCEL) && (sh->stat!=TMPI_PEND_RELS)) {
		tmpi_error(DBG_INTERNAL, "protocol error, sh(%p) state=%d got RDR.", sh, sh->stat);
		tmpi_dump_handle(DBG_INTERNAL, sh);
	}
	else {
		sh->stat=TMPI_FREE;
		sh->errval=MPI_SUCCESS;
		// free the handle for TMPI_CANCEL or TMPI_PEND_RELS
		if (sh->handle_type==MPI_PERSISTENT_SEND) {
			sh->curdata=sh->user_buf;
			sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
			thr_mtx_unlock(&(sh->lock));
		}
		else {
			tmpi_Channel *ch;

			ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
			thr_mtx_unlock(&(sh->lock));
			if (ch)
				tmpi_intra_free_sh(ch, sh);
		}
		unlockit=0;
	}

	if (unlockit)
		thr_mtx_unlock(&(sh->lock));

	return 0;
}

static int tmpi_rmdr_handle(tmpi_RDRHandle *handle)
{
	tmpi_SHandle *sh=NULL;
	int unlockit=1;

	sh=handle->local_send_handle;

	if (!sh)
		return -1;

	thr_mtx_lock(&(sh->lock));
	sh->buflen=handle->buflen;
	sh->errval=handle->errval;

	if (sh->stat==TMPI_PEND_STRT) {
		sh->stat=TMPI_MATCH;
		if (sh->send_mode==READY)
			thr_cnd_signal(&(sh->wait));
	}
	else if (sh->stat==TMPI_PEND_WAIT) {
		sh->stat=TMPI_MATCH;
		thr_cnd_signal(&(sh->wait));
	}
	else if ((sh->stat!=TMPI_CANCEL) && (sh->stat!=TMPI_PEND_RELS)) {
		tmpi_error(DBG_INTERNAL, "protocol error, sh(%p) state=%d got RMDR.", sh, sh->stat);
		tmpi_dump_handle(DBG_INTERNAL, sh);
	}
	else {
		sh->stat=TMPI_FREE;
		sh->errval=MPI_SUCCESS;
		// free the handle for TMPI_CANCEL or TMPI_PEND_RELS
		if (sh->handle_type==MPI_PERSISTENT_SEND) {
			sh->curdata=sh->user_buf;
			sh->buflen=tmpi_Usize(sh->datatype)*sh->count;
			thr_mtx_unlock(&(sh->lock));
		}
		else {
			tmpi_Channel *ch;

			ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
			thr_mtx_unlock(&(sh->lock));
			if (ch)
				tmpi_intra_free_sh(ch, sh);
		}
		unlockit=0;
	}

	if (unlockit)
		thr_mtx_unlock(&(sh->lock));

	return 0;
}

static int tmpi_rndy_handle(tmpi_RDRHandle *handle)
{
	tmpi_SHandle *sh=NULL;

	sh=handle->local_send_handle;

	if (!sh)
		return -1;

	thr_mtx_lock(&(sh->lock));
	sh->errval=handle->errval;

	if (sh->stat==TMPI_PEND_STRT) {
		sh->stat=TMPI_PEND_NRDY;
		thr_cnd_signal(&(sh->wait));
	}
	else {
		tmpi_error(DBG_INTERNAL, "protocol error, sh(%p) state=%d got RNRDY.", sh, sh->stat);
		tmpi_dump_handle(DBG_INTERNAL, sh);
	}

	thr_mtx_unlock(&(sh->lock));

	return 0;
}


int tmpi_inter_msg_handle(int from, int to, tmpi_MSG *msg, int msglen, int payloadsize)
{
	/**
	 * from, to, msglen, payloadsize are completely ignored, 
	 * because they should also be stored in the actual handle 
	 */
	if (tmpi_debug(DBG_INFO)) {
		if ( (msg->body.type&MPI_REMOTE_MSG_SEND) 
				&& ( (msg->body.common.src!=from) || (msg->body.common.dest!=to) )
		) {
			tmpi_error(DBG_INTERNAL, "from/to parameter doesn't match message content.");
			return -1;
		}

		if (msglen!=sizeof(tmpi_MSG)) {
			tmpi_error(DBG_INTERNAL, "wrong message size."); 
			return -1;
		}

		switch(msg->body.type) {
			case MPI_REMOTE_MSGDAT_SEND :
				if (msg->body.rmshandle.buflen!=payloadsize) {
					tmpi_error(DBG_INTERNAL, "payloadsize parameter doesn't match content.");
					return -1;
				}
				break;
			case MPI_REMOTE_DAT_SEND :
				if (msg->body.rdshandle.buflen!=payloadsize) {
					tmpi_error(DBG_INTERNAL, "payloadsize parameter doesn't match content.");
					return -1;
				}
				break;
			default :
				if (payloadsize!=0) {
					tmpi_error(DBG_INTERNAL, "payloadsize parameter doesn't match content.");
					return -1;
				}
		}
	}

	switch(msg->body.type) {
		case MPI_REMOTE_MSG_SEND : 
			return tmpi_rms_handle(&(msg->body.rmshandle));
		case MPI_REMOTE_MSGDAT_SEND :
			return tmpi_rmds_handle(&(msg->body.rmshandle));
		case MPI_REMOTE_DAT_SEND :
			return tmpi_rds_handle(&(msg->body.rdshandle));
		case MPI_REMOTE_MSG_RECV :
			return tmpi_rmr_handle(&(msg->body.rmrhandle));
		case MPI_REMOTE_DAT_RECV :
			return tmpi_rdr_handle(&(msg->body.rdrhandle));
		case MPI_REMOTE_MSGDAT_RECV :
			/* tmpi_dump_handle(DBG_INTERNAL, &(msg->body)); */
			return tmpi_rmdr_handle(&(msg->body.rdrhandle));
		case MPI_REMOTE_NRDY :
			return tmpi_rndy_handle(&(msg->body.rdrhandle));
		default :
			tmpi_error(DBG_INTERNAL, 
					"inter msg handle, protocol error, type=%d.", 
					msg->body.type);
			return -1;
	}
}
