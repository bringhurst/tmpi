# ifndef _INTRA_SR_H_
# define _INTRA_SR_H_

# include "mpi_struct.h"
# include "tmpi_struct.h"

int tmpi_grank_to_lrank(int grank);
tmpi_Channel *tmpi_intra_get_channel(int local_id);
int tmpi_intra_init(int np);
void tmpi_intra_end();
tmpi_MSG *tmpi_inter_new_msg();
void tmpi_inter_free_msg(tmpi_MSG *msg);
tmpi_RMSHandle *tmpi_intra_new_rmsh(tmpi_Channel *ch);
void tmpi_intra_free_rh(tmpi_Channel *ch, tmpi_RHandle *rh);
void tmpi_intra_free_sh(tmpi_Channel *ch, tmpi_SHandle *sh);
tmpi_RHandle *tmpi_rq_find(tmpi_Channel *ch, int context, int tag, int src);
void tmpi_sq_enq(tmpi_Channel *ch, tmpi_SHandle *sh);
int tmpi_intra_init_sh(MPI_Comm comm, char *buf, int from, int to, int tag, MPI_Datatype datatype, 
		int count, tmpi_Mode mode, int isPersistent, int isImmediate, MPI_Request *psh);
int tmpi_intra_init_rh(MPI_Comm comm, char *buf, int from, int to, int tag, MPI_Datatype datatype, 
		int count, int isPersistent, int isImmediate, MPI_Request *prh);
int tmpi_intra_start_sh(tmpi_SHandle **psh);
int tmpi_intra_start_rh(tmpi_RHandle **prh);
int tmpi_intra_wait_sh(tmpi_SHandle **psh, tmpi_Status *status);
int tmpi_intra_wait_rh(tmpi_RHandle **prh, tmpi_Status *status);
int tmpi_intra_cancel_sh(tmpi_SHandle **psh);
int tmpi_intra_cancel_rh(tmpi_RHandle **prh);
int tmpi_intra_test_sh(tmpi_SHandle **psh, int *flag, tmpi_Status *status);
int tmpi_intra_test_rh(tmpi_RHandle **prh, int *flag, tmpi_Status *status);

int tmpi_inter_msg_handle(int from, int to, tmpi_MSG *msg, int msglen, int payloadsize);
void tmpi_dump_handle(int dbg_level, void *handle);
/**
 * set in bound message size to be 100 times of the average package size
 * so that the extra round trip cost introduced by the long message can
 * be neglected from the total cost.
 */
/* 100K */
# define TMPI_MAX_SHORT_MSG (3000*0x400)

# endif
