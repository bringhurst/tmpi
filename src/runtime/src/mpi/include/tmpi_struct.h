#ifndef _TMPI_STRUCT_H_
#define _TMPI_STRUCT_H_

# include "mpi_def.h"
# include "mpi_struct.h"
# include "thread.h"
# include "netq.h"
# include "tqueue.h"

typedef unsigned char tmpi_Mode;
enum {STANDARD=1, READY=2, SYNC=4, BUFFERED=8};

typedef enum {INTER, INTRA} tmpi_Comm_type;

/***********************************************************************/
/* a group of processes                                                */
/***********************************************************************/

#define MPI_GROUP_COOKIE 0x87ef54a0

typedef struct tmpi_Group {
		unsigned long cookie;/* cookie to help detect valid item */
		int np;/* number of processes in group */
		int local_rank;/* my rank in the group (if I belong) */
		/* MPI_UNDEFINED if I am not a member */
		int ref_count;/* number of references to this group */
		int isPermanent;/* permanent group */
		int *lrank2grank;/* mapping from local to "global" ranks */  
} tmpi_Group;


/***********************************************************************/
/* Communicator is a group of processes involved in communication      */
/***********************************************************************/

# include "barrier.h"
# include "bcast.h"
# include "reduce.h"

#define MPI_COMM_COOKIE 0xbad2face

typedef struct tmpi_Comm {
		unsigned long cookie;/* cookie to help detect valid item */
		int np;/* size of group */
		int local_rank;/* rank in local_group for this process */
		int *lrank2grank;/* mapping for group */
		unsigned long pt2pt_context;/* context for point to point communication */
		unsigned long coll_context;/* context for collective communication */
		tmpi_Comm_type comm_type;/* inter or intra */
		tmpi_Group *group;/* group associated with this communicator */
		int ref_count;/* number of references to communicator */

		tmpi_BcastObj *bc_ptr;/* intra-bcast object, hook for broadcast */
		tmpi_ReduceObj *rdc_ptr;/* intra-reduce object, hook for reduce */
		tmpi_AllreduceObj *ardc_ptr;/* intra-allreduce object, hook for allreduce */

		void *comm_cache;/* hook for communicator cache */
		void *attr_cache;/* hook for attribute cache */
		int isPermanent;/* permanent communicator */

		tmpi_Bar *br;/* intra-barrier structure, one copy shared by all */
} tmpi_Comm;


/***********************************************************************/
/* Data structure for send/receive handle and request                  */
/***********************************************************************/

#define MPI_REQUEST_COOKIE 0xaddbcafe

enum {
  MPI_SEND=1, 
  MPI_RECV=2, 
  MPI_PERSISTENT_SEND=4,
  MPI_PERSISTENT_RECV=8,
  MPI_IMMEDIATE_SEND=16,
  MPI_IMMEDIATE_RECV=32,
  MPI_REMOTE_MSG_SEND=64,
  MPI_REMOTE_DAT_SEND=128,
  MPI_REMOTE_MSGDAT_SEND=MPI_REMOTE_MSG_SEND|MPI_REMOTE_DAT_SEND,
  MPI_REMOTE_MSG_RECV=256,
  MPI_REMOTE_DAT_RECV=512,
  MPI_REMOTE_MSGDAT_RECV=MPI_REMOTE_MSG_RECV|MPI_REMOTE_DAT_RECV,
  MPI_REMOTE_NRDY=1024,
  MPI_PROBE=2048 /* not used now */
};

typedef unsigned int tmpi_OPTYPE;

/* states of message handles */
enum {
  TMPI_CREATE=0x1, /* just created */
# define TMPI_PEND_STRT TMPI_PEND
  TMPI_PEND=0x2, /* to be matched */
  TMPI_WAIT=0x4, /* owner blocking */
  TMPI_PEND_WAIT=TMPI_PEND|TMPI_WAIT,
  TMPI_RELS=0x8, /* buffered send, sender already left */
  TMPI_PEND_RELS=TMPI_PEND|TMPI_RELS,
  TMPI_DATA=0x10, /* waiting to send data or to receive data */
  TMPI_PEND_DATA=TMPI_PEND|TMPI_DATA,
  TMPI_PEND_DWAT=TMPI_PEND|TMPI_DATA|TMPI_WAIT,
  TMPI_NRDY=0x20, /* receiver not ready */
  TMPI_PEND_NRDY=TMPI_PEND|TMPI_NRDY,
  TMPI_MATCH=0x40,
  TMPI_CANCEL=0x80, /* request has been cancelled. */
  TMPI_FREE=0x100, /* request done */
  TMPI_ERR=0x200 /* error happens */
};

union tmpi_Handle;

typedef struct tmpi_COMMON {
		tmpi_OPTYPE handle_type;
		unsigned long cookie;
		unsigned int stat; /* handle state */
		unsigned long context;/* context id */
		int tag;/* tag */
		int src;/* global source process for message */
		int dest;/* global destination process for message */
} tmpi_Common;

/* message send handle */
typedef struct tmpi_Shandle {
		tmpi_OPTYPE handle_type;
		unsigned long cookie;/* Cookie to help detect valid item */
		unsigned int stat; /* handle state */
		unsigned long context;/* context id */
		int tag;/* tag */
		int src;/* global source process for message */
		int dest;/* global destination process for message */
		MPI_Comm comm;/* communicator */
		int count;/* requested count in units of the data_type, never change */
		char *curdata;/* place where the good data can be found */
		int buflen;/* length of either user_buf or sys_buf */
		tmpi_Mode send_mode;/* mode: STANDARD, READY, SYNC, BUFFERED */
		char *user_buf;/* user's buffer space */
		int errval;/* Holds any error code; 0 for none */
		char *sys_buf;/* system buffer */
		// int isActive; /* relavent only persistent handle */
		int isRemote; /* inter or intra request? */
		thr_mtx_t lock; /* only useful if isRemote is 1 */
		thr_cnd_t wait;
		MPI_Datatype datatype;/* basic or derived datatype */
} tmpi_SHandle;

/**
 * remote message send handle.
 * If RMSHandle type is:
 * MSG: curdata is NULL, buflen is the length of the 
 * buffer, remote_handle is the remote send handle.
 * MSGDAT: curdata is the data, buflen is the length of the buffer,
 * remote_handle is the remote send handle so that receiver can
 * return feedback.
 * comm will be back filled by comm_lookup based on context.
 *
 * The structure layout is carefully designed so that we can do 
 * a copy for the size tmpi_RMSHandle, and just modify the
 * handle_type and remote_send_handle field.
 */

/* here is a remote send, with or without data */
typedef struct tmpi_RMShandle {
		tmpi_OPTYPE handle_type;/* should be REMOTE_MSG_SEND/REMOTE_MSGDAT_SEND */
		unsigned long cookie;/* Cookie to help detect valid item */
		unsigned int stat; /* handle state */
		unsigned long context;/* context id */
		int tag;/* tag */
		int src;/* global source process for message */
		int dest;/* global destination process for message */
		MPI_Comm comm;/* to be filled back */
		int count;/* requested count in units of the data type, never change */
		char *curdata;/* always allocated in internal buffer at the remote site */
		int buflen;/* length of either user_buf or sys_buf */
		tmpi_Mode send_mode;/* mode: STANDARD, READY, SYNC, BUFFERED */
		tmpi_SHandle *remote_send_handle; /* if curdata is NULL, need to fetch data from remote */
		int errval;/* Holds any error code; 0 for none */
} tmpi_RMSHandle;

/* message receive handle */
typedef struct tmpi_Rhandle {
		tmpi_OPTYPE handle_type;
		unsigned long cookie;/* Cookie to help detect valid item */
		unsigned int stat; /* handle state */
		unsigned long context;/* context id */
		int tag;/* tag */
		int src;/* global source process for message */
		int dest;/* global destination process for message */
		MPI_Comm comm;/* communicator */
		int count;/* requested count in units of the data type, never change */
		char *data;/* user data buffer */
		int buflen;/* length of buffer at bufadd */
		int errval;/* Holds any error code; 0 for none */
		// int isActive;/* relavent only persistent handle */
		thr_mtx_t lock;
		thr_cnd_t wait;
		/* For persistant receives, we may need to restore some of the fields
		   after the operation completes */
		/* the parameters set at the construction time of the request */
		MPI_Datatype datatype;/* basic or derived datatype */
		int perm_source, perm_tag, perm_len;
} tmpi_RHandle;

/* remote mesaage receive handle */
typedef struct tmpi_RMRhandle {
		tmpi_OPTYPE handle_type; /* REMOTE_MSG_RECV */
		tmpi_SHandle *local_send_handle;
		tmpi_RHandle *remote_recv_handle; 
		int buflen;
		int errval;
} tmpi_RMRHandle;

/**
 * remote data send handle.
 * DAT: buflen is the length of thebuffer, 
 * local_recv_handle is the receive handle on the local machine.
 */
typedef struct tmpi_RDShandle {
		tmpi_OPTYPE handle_type;/* should be REMOTE_DAT_SEND */
		tmpi_SHandle *remote_send_handle;
		tmpi_RHandle *local_recv_handle; 
		int buflen;
		int errval;
} tmpi_RDSHandle;

typedef struct tmpi_RDRhandle {
		/* should be REMOTE_MSGDAT_SEND, REMOTE_DAT_RECV, REOMTE_NRDY */
		tmpi_OPTYPE handle_type;
		tmpi_SHandle *local_send_handle;
		int buflen;
		int errval;
} tmpi_RDRHandle;

/* allocation unit for tqueue */
typedef union tmpi_Handle {
		tmpi_OPTYPE type;  
		tmpi_Common common;
		tmpi_SHandle shandle;
		tmpi_RMSHandle rmshandle;
		tmpi_RDSHandle rdshandle;
		tmpi_RHandle rhandle;
		tmpi_RMRHandle rmrhandle;
		tmpi_RDRHandle rdrhandle;
} tmpi_Handle;

/* around 108 bytes long header! */
/* allocation unit for fifo queue */
typedef struct tmpi_Msg {
	NETQ_HEADER hdr;
	union {
		tmpi_OPTYPE type;  
		tmpi_Common common;
		// tmpi_SHandle shandle;
		tmpi_RMSHandle rmshandle;
		tmpi_RDSHandle rdshandle;
		// tmpi_RHandle rhandle;
		tmpi_RMRHandle rmrhandle;
		tmpi_RDRHandle rdrhandle;
	} body;
} tmpi_MSG;

typedef struct tmpi_Channel {
	thr_mtx_t lock;
	TQueue *rq;
	TQueue *sq;
} tmpi_Channel;

#endif
