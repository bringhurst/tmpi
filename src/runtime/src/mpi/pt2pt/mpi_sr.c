/*
 * Send/Recv family of functions.
 * [I][bsr]send, [bsr]send_init, [I]recv, recv_init. 15 functions.
 * Sendrecv[_replace], two functions.
 */
# include "intra_sr.h"
# include "mpi_struct.h"
# include "tmpi_struct.h"
# include "tmpi_debug.h"
# include "ukey.h"
# include "thread.h"
# include "netq.h"

# ifndef FALSE
# define FALSE 0
# endif

# ifndef TRUE
# define TRUE 1
# endif

# define SEND_MASK (MPI_SEND|MPI_PERSISTENT_SEND|MPI_IMMEDIATE_SEND)
# define RECV_MASK (MPI_RECV|MPI_PERSISTENT_RECV|MPI_IMMEDIATE_RECV)

static int isSendRequest(MPI_Request *req)
{
	if ((*req)->type&SEND_MASK)
		return TRUE;
	return FALSE;
}

static int isRecvRequest(MPI_Request *req)
{
	if ((*req)->type&RECV_MASK)
		return TRUE;
	return FALSE;
}

static int tmpi_Request_free_sh(tmpi_SHandle **psh)
{
	tmpi_SHandle *sh=*psh;

	if (!sh)
		return MPI_SUCCESS;

	thr_mtx_lock(&(sh->lock));
	if (sh->stat&(TMPI_FREE|TMPI_CREATE)) {
		tmpi_Channel *ch;

		thr_mtx_unlock(&(sh->lock));
		ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
		if (ch)
			tmpi_intra_free_sh(ch, sh);
		else
			tmpi_error(DBG_INTERNAL, "cannot get channel object.");
		*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;

		return MPI_SUCCESS;
	}
	else {
		int flag;

		thr_mtx_unlock(&(sh->lock));
		tmpi_intra_test_sh(psh, &flag, NULL);
		if (flag) {
			tmpi_Channel *ch;

			sh=*psh;

			if (!sh)
				return MPI_SUCCESS;

			ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(sh->src));
			if (ch)
				tmpi_intra_free_sh(ch, sh);
			else
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");
			*psh=(tmpi_SHandle *)MPI_REQUEST_NULL;
		}
		else
			return MPI_ERR_REQUEST; /* attempting to free an ongoing request */
	}

	return MPI_SUCCESS;
}

static int tmpi_Request_free_rh(tmpi_RHandle **prh)
{
	tmpi_RHandle *rh=*prh;

	if (!rh)
		return MPI_SUCCESS;

	thr_mtx_lock(&(rh->lock));
	if (rh->stat&(TMPI_FREE|TMPI_CREATE)) {
		tmpi_Channel *ch;

		thr_mtx_unlock(&(rh->lock));
		ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(rh->dest));
		if (ch)
			tmpi_intra_free_rh(ch, rh);
		else
			tmpi_error(DBG_INTERNAL, "cannot get channel object.");
		*prh=(tmpi_RHandle *)MPI_REQUEST_NULL;

		return MPI_SUCCESS;
	}
	else {
		int flag;

		thr_mtx_unlock(&(rh->lock));
		tmpi_intra_test_rh(prh, &flag, NULL);
		if (flag) {
			tmpi_Channel *ch;

			rh=*prh;

			if (!rh)
				return MPI_SUCCESS;

			ch=tmpi_intra_get_channel(tmpi_grank_to_lrank(rh->dest));
			if (ch)
				tmpi_intra_free_rh(ch, rh);
			else
				tmpi_error(DBG_INTERNAL, "cannot get channel object.");
			*prh=(tmpi_RHandle *)MPI_REQUEST_NULL;
		}
		else
			return MPI_ERR_REQUEST; /* attempting to free an ongoing request */
	}

	return MPI_SUCCESS;
}

int tmpi_Request_free(MPI_Request *req)
{
	if (*req==MPI_REQUEST_NULL)
		return MPI_SUCCESS;

	if (isSendRequest(req))
		return tmpi_Request_free_sh((tmpi_SHandle **)req);

	return tmpi_Request_free_rh((tmpi_RHandle **)req);
}

int tmpi_Start(MPI_Request *req)
{
	if (*req==MPI_REQUEST_NULL)
		return MPI_SUCCESS;

	if (isSendRequest(req))
		return tmpi_intra_start_sh((tmpi_SHandle **)req);

	return tmpi_intra_start_rh((tmpi_RHandle **)req);
}

int tmpi_Wait(MPI_Request *req, MPI_Status *status)
{
	if (*req==MPI_REQUEST_NULL)
		return MPI_SUCCESS;

	if (isSendRequest(req))
		return tmpi_intra_wait_sh((tmpi_SHandle **)req, status);

	return tmpi_intra_wait_rh((tmpi_RHandle **)req, status);
}

int tmpi_Cancel(MPI_Request *req)
{
	if (*req==MPI_REQUEST_NULL)
		return MPI_SUCCESS;

	if (isSendRequest(req))
		return tmpi_intra_cancel_sh((tmpi_SHandle **)req);

	return tmpi_intra_cancel_rh((tmpi_RHandle **)req);
}

int tmpi_Test(MPI_Request *req, int *flag, MPI_Status *status)
{
	if (*req==MPI_REQUEST_NULL)
		return MPI_SUCCESS;

	if (isSendRequest(req))
		return tmpi_intra_test_sh((tmpi_SHandle **)req, flag, status);

	return tmpi_intra_test_rh((tmpi_RHandle **)req, flag, status);
}

int tmpi_Send(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, STANDARD, FALSE, FALSE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		return ret;
	}
	
	ret=tmpi_intra_start_sh((tmpi_SHandle **)&req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		return ret;
	}

	return tmpi_intra_wait_sh((tmpi_SHandle **)&req, NULL);
}

int tmpi_Isend(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm, MPI_Request *request)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, STANDARD, FALSE, TRUE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
		return ret;
	}
	
	ret=tmpi_intra_start_sh((tmpi_SHandle **)&req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
		return ret;
	}
	
	*request=req; 
	
	return MPI_SUCCESS;
}

int tmpi_Send_init(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm, MPI_Request *request)
{
	MPI_Request req;
	int my_global_id;
	int ret;


	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, STANDARD, TRUE, FALSE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
	}
	else {
		(*request)=req;
	}
	
	return ret;
}

int tmpi_Bsend(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, BUFFERED, FALSE, FALSE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		return ret;
	}
	
	ret=tmpi_intra_start_sh((tmpi_SHandle **)&req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		return ret;
	}

	return tmpi_intra_wait_sh((tmpi_SHandle **)&req, NULL);
}

int tmpi_Ibsend(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm, MPI_Request *request)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, BUFFERED, FALSE, TRUE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
		return ret;
	}
	
	ret=tmpi_intra_start_sh((tmpi_SHandle **)&req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
		return ret;
	}
	
	*request=req; 
	
	return MPI_SUCCESS;
}

int tmpi_Bsend_init(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm, MPI_Request *request)
{
	MPI_Request req;
	int my_global_id;
	int ret;


	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, BUFFERED, TRUE, FALSE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
	}
	else {
		(*request)=req;
	}
	
	return ret;
}

int tmpi_Rsend(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, READY, FALSE, FALSE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		return ret;
	}
	
	ret=tmpi_intra_start_sh((tmpi_SHandle **)&req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		return ret;
	}

	return tmpi_intra_wait_sh((tmpi_SHandle **)&req, NULL);
}

int tmpi_Irsend(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm, MPI_Request *request)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, READY, FALSE, TRUE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
		return ret;
	}
	
	ret=tmpi_intra_start_sh((tmpi_SHandle **)&req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
		return ret;
	}
	
	*request=req; 
	
	return MPI_SUCCESS;
}

int tmpi_Rsend_init(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm, MPI_Request *request)
{
	MPI_Request req;
	int my_global_id;
	int ret;


	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, READY, TRUE, FALSE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
	}
	else {
		(*request)=req;
	}
	
	return ret;
}

int tmpi_Ssend(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, SYNC, FALSE, FALSE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		return ret;
	}
	
	ret=tmpi_intra_start_sh((tmpi_SHandle **)&req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		return ret;
	}

	return tmpi_intra_wait_sh((tmpi_SHandle **)&req, NULL);
}

int tmpi_Issend(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm, MPI_Request *request)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, SYNC, FALSE, TRUE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
		return ret;
	}
	
	ret=tmpi_intra_start_sh((tmpi_SHandle **)&req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
		return ret;
	}
	
	*request=req; 
	
	return MPI_SUCCESS;
}

int tmpi_Ssend_init(void *message, int count, MPI_Datatype datatype, int dest,
	int tag, MPI_Comm comm, MPI_Request *request)
{
	MPI_Request req;
	int my_global_id;
	int ret;


	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_sh(comm, (char *)message, my_global_id, dest, 
			tag, datatype, count, SYNC, TRUE, FALSE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_sh((tmpi_SHandle **)&req);
		(*request)=MPI_REQUEST_NULL;
	}
	else {
		(*request)=req;
	}
	
	return ret;
}

int tmpi_Recv(void *message, int count, MPI_Datatype datatype, int source,
	int tag, MPI_Comm comm, MPI_Status *status)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_rh(comm, (char *)message, source, my_global_id, tag, 
			datatype, count, FALSE, FALSE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_rh((tmpi_RHandle **)&req);
		return ret;
	}
	
	ret=tmpi_intra_start_rh((tmpi_RHandle **)&req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_rh((tmpi_RHandle **)&req);
		return ret;
	}

	ret=tmpi_intra_wait_rh((tmpi_RHandle **)&req, status);

	return ret;
}

int tmpi_Irecv(void *message, int count, MPI_Datatype datatype, int source,
	int tag, MPI_Comm comm, MPI_Request *request)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_rh(comm, (char *)message, source, my_global_id, tag, 
			datatype, count, FALSE, TRUE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_rh((tmpi_RHandle **)&req);
		*request=MPI_REQUEST_NULL;
		return ret;
	}
	
	ret=tmpi_intra_start_rh((tmpi_RHandle **)&req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_rh((tmpi_RHandle **)&req);
		*request=MPI_REQUEST_NULL;
		return ret;
	}

	(*request)=req;
	
	return MPI_SUCCESS;
}

int tmpi_Recv_init(void *message, int count, MPI_Datatype datatype, int source,
	int tag, MPI_Comm comm, MPI_Request *request)
{
	MPI_Request req;
	int my_global_id;
	int ret;

	if (netq_transLocalRank(ukey_getid(), &my_global_id)<0) {
		tmpi_error(DBG_INTERNAL, "cannot get my global id.");
		return MPI_ERR_INTERN;
	}

	ret=tmpi_intra_init_rh(comm, (char *)message, source, my_global_id, tag, 
			datatype, count, FALSE, TRUE, &req);

	if (ret!=MPI_SUCCESS) {
		tmpi_Request_free_rh((tmpi_RHandle **)&req);
		*request=MPI_REQUEST_NULL;
	}
	else {
		*request=req;
	}
	
	return ret;
}

int tmpi_Sendrecv(void *send_buf, int send_count, MPI_Datatype send_type,
	int dest, int send_tag, void *recv_buf, int recv_count, MPI_Datatype recv_type,
	int source, int recv_tag, MPI_Comm comm, MPI_Status *status)
{
	MPI_Request req1, req2;
	int ret1, ret2;

	tmpi_Isend(send_buf, send_count, send_type, dest, send_tag, comm, &req1);
	tmpi_Irecv(recv_buf, recv_count, recv_type, source, recv_tag, comm, &req2);
	ret1=tmpi_Wait(&req1, NULL);
	ret2=tmpi_Wait(&req2, status);
	if (ret1!=MPI_SUCCESS)
		return ret1;
	if (ret2!=MPI_SUCCESS)
		return ret2;
	return MPI_SUCCESS;
}

int tmpi_Sendrecv_replace(void *buffer, int count, MPI_Datatype datatype,
	int dest, int send_tag, int src, int recv_tag, MPI_Comm comm, MPI_Status *status)
{
	MPI_Request req2;
	int ret1, ret2;
	
	if (netq_isLocalNode(dest)) {
		ret1=tmpi_Rsend(buffer, count, datatype, dest, send_tag, comm);
	}
	else {
		ret1=MPI_ERR_NRDY;
	}

	if (ret1!=MPI_SUCCESS)
		ret1=tmpi_Bsend(buffer, count, datatype, dest, send_tag, comm);

	tmpi_Irecv(buffer, count, datatype, src, recv_tag, comm, &req2);
	ret2=MPI_Wait(&req2, status);

	if (ret1!=MPI_SUCCESS)
		return ret1;

	if (ret2!=MPI_SUCCESS)
		return ret2;

	return MPI_SUCCESS;
}

int tmpi_Startall(int count, MPI_Request *reqs)
{
	int i;
	int ret=MPI_SUCCESS;
	int ret0;

	for (i=0; i<count; i++) {
		ret0=tmpi_Start(reqs+i);
		if (ret0!=MPI_SUCCESS)
			ret=ret0;
	}

	return ret;
}

/**
 * a bit different from the spec, in the sense that if some of the 
 * requests are completed, then they will be deallocated.
 */
int tmpi_Testall(int count, MPI_Request *reqs, int *flag, MPI_Status *stats)
{
	int i;
	int ret=MPI_SUCCESS;
	int ret0;
	int flag0;

	*flag=1;

	for (i=0; i<count; i++) {
		flag0=1;

		ret0=tmpi_Test(reqs+i, &flag0, stats+i);

		if (ret0!=MPI_SUCCESS)
			ret=ret0;

		if (!flag0)
			*flag=0;
	}

	return ret;
}

int tmpi_Testany(int count, MPI_Request *reqs, int *index, int *flag, MPI_Status *status)
{
	int i;
	int ret=MPI_SUCCESS;
	int ret0;

	*flag=0;
	for (i=0; i<count; i++) {
		ret0=tmpi_Test(reqs+i, flag, status);

		if (ret0!=MPI_SUCCESS)
			ret=ret0;

		if (*flag) {
			*index=i;
			return ret0;
		}
	}

	return ret;
}

int tmpi_Testsome(int count, MPI_Request *reqs, int *outcnt, int *indice, MPI_Status *stats)
{
	int i;
	int ret=MPI_SUCCESS;
	int ret0;
	int flag;
	int cnt=0;

	for (i=0; i<count; i++) {
		flag=0;
		ret0=tmpi_Test(reqs+i, &flag, stats+i);

		if (ret0!=MPI_SUCCESS)
			ret=ret0;

		if (flag) {
			indice[cnt++]=i;
		}
	}

	*outcnt=cnt;

	return ret;
}

int tmpi_Waitall(int count, MPI_Request *reqs, MPI_Status *stats)
{
	int i;
	int ret=MPI_SUCCESS;
	int ret0;

	for (i=0; i<count; i++) {
		ret0=tmpi_Wait(reqs+i, stats+i);

		if (ret0!=MPI_SUCCESS)
			ret=ret0;
	}

	return ret;
}

int tmpi_Waitany(int count, MPI_Request *reqs, int *index, MPI_Status *status)
{
	int i;
	int ret=MPI_SUCCESS;
	int ret0;
	int flag=0;

	for (i=0; i<count; i++) {
		ret0=tmpi_Test(reqs+i, &flag, status);
		if (ret0!=MPI_SUCCESS)
			ret=ret0;

		if (flag)
			break;
	}

	if (flag) {
		*index=i;
		return ret;
	}
	else {
		*index=0;
		return tmpi_Wait(reqs, status);
	}
}

int tmpi_Waitsome(int count, MPI_Request *reqs, int *outcnt, int *indice, MPI_Status *stats)
{
	int i;
	int ret=MPI_SUCCESS;
	int ret0;
	int flag;
	int cnt=0;

	for (i=0; i<count; i++) {
		flag=0;
		ret0=tmpi_Test(reqs+i, &flag, stats+i);
		if (ret0!=MPI_SUCCESS)
			ret=ret0;

		if (flag) {
			indice[cnt++]=i;
		}
	}

	if (cnt) {
		*outcnt=cnt;
		return ret;
	}

	ret0=tmpi_Wait(reqs, stats);

	if (ret0!=MPI_SUCCESS)
		ret=ret0;

	indice[cnt++]=0;

	for (i=1; i<count; i++) {
		flag=0;
		ret0=tmpi_Test(reqs+i, &flag, stats+i);
		if (ret0!=MPI_SUCCESS)
			ret=ret0;

		if (flag) {
			indice[cnt++]=i;
		}
	}

	*outcnt=cnt;
	return ret;
}
