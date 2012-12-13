# ifndef _NETQ_COMM_H_
# define _NETQ_COMM_H_

# include "netd_comm.h"
/**
 * Header file for network queueing
 * This is the interface seen by
 * the application.
 *
 * The difference between NETQ and
 * NETD is that NETD only knows about
 * cluster node address while NETQ
 * knows about thread node address.
 *
 * 		Application
 * ----------------------------
 *  Network Queuing Interface
 * ----------------------------
 * 	Net Device Interface
 * ----------------------------
 *  TCP/IP | P4 | ...
 */

/**
 * NETQ hanldes only inter-cluster communication,
 * it will refuse to do anything if you attempt to
 * send a message to the intra-cluster thread node.
 */

# define NETQ_BLK 1
# define NETQ_NBLK 2

/* successful return from netq_send doesn't mean that the message has been sent **/
int netq_send(int from, int to, char *header, int headerlen, char *payload, int payloadlen, int mode);

/** 
 * receive EXACTLY len bytes data from "from", "from" cannot be wildcard, if less
 * than requested data is available, the request will hang.
 *
 * The current implementation is quite dangerous, NETQ assumes whatever comes from
 * the proper channel are data. A safer way will require internal buffering in NETQ
 * which I don't want to waste time on. Currently NETQ will send payload (data) 
 * immediately following the header, so the application have to be able to detect
 * if there is a payload after the header and how large it is. Then issue netq_recvdata.
 *
 * netq_recvdata is only meant to be called by the message handler.
 */
int netq_recvdata(int from, char *data, int len);

int netq_bcast(char *data, int len, int root);

/* the data types and op codes are inherited from netd */
int netq_reduce(char *src, char *result, int nelem, int op, int data_type, int root);

/* the data types and op codes are inherited from netd */
int netq_allreduce(char *src, char *result, int nelem, int op, int data_type);

int netq_barrier();

/** 
 * return one of the following as netq status 
 * NETQ_OFF 	: not yet started (before netq_start())
 * NETQ_INIT 	: initializing the netq (during netq_start())
 * NETQ_ON		: netq is up and ready to take requests 
 * 					(after netq_start() but before netq_end())
 * NETQ_END		: netq is being taking down (during netq_end())
 * NETQ_DOWN	: netq is offline (after netq_end())
 **/

# define NETQ_OFF 1 
# define NETQ_INIT 2
# define NETQ_ON 3
# define NETQ_END 4
# define NETQ_DOWN 5
# define NETQ_ERROR_START 6
# define NETQ_ERROR_RUN 7

int netq_status();

/** receiving data are carried out asynchronously */

# define NETQ_OP_ABSMAX NETD_OP_ABSMAX
# define NETQ_OP_ABSMIN NETD_OP_ABSMIN
# define NETQ_OP_MAX NETD_OP_MAX
# define NETQ_OP_MIN NETD_OP_MIN
# define NETQ_OP_MULT NETD_OP_MULT
# define NETQ_OP_SUM NETD_OP_SUM

/** note that NETD_CHR is not valid reduce type */
# define NETQ_TYPE_INT NETD_TYPE_INT
# define NETQ_TYPE_FLT NETD_TYPE_FLT
# define NETQ_TYPE_DBL NETD_TYPE_DBL
# define NETQ_TYPE_LNG NETD_TYPE_LNG
# define NETQ_TYPE_CHR NETD_TYPE_CHR

/** if you wants to define new types, start from NETD_NXT **/
# define NETQ_TYPE_NXT NETD_TYPE_NXT

# endif
