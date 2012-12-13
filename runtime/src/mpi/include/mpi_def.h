#ifndef _MPI_DEF_H_
#define _MPI_DEF_H_

#define MPI_SUCCESS          0      /* Successful return code */
#define MPI_ERR_BUFFER       1      /* Invalid buffer pointer or no enough buffer space */
#define MPI_ERR_COUNT        2      /* Invalid count argument */
#define MPI_ERR_TYPE         3      /* Invalid datatype argument */
#define MPI_ERR_TAG          4      /* Invalid tag argument */
#define MPI_ERR_COMM         5      /* Invalid communicator */
#define MPI_ERR_RANK         6      /* Invalid rank */
#define MPI_ERR_ROOT         7      /* Invalid root */
#define MPI_ERR_GROUP        8      /* Null group passed to function */
#define MPI_ERR_OP           9      /* Invalid operation */
#define MPI_ERR_TOPOLOGY    10      /* Invalid topology */
#define MPI_ERR_DIMS        11      /* Illegal dimension argument */
#define MPI_ERR_ARG         12      /* Invalid argument */
#define MPI_ERR_UNKNOWN     13      /* Unknown error */
#define MPI_ERR_TRUNCATE    14      /* message truncated on receive */
#define MPI_ERR_OTHER       15      /* Other error; use Error_string */
#define MPI_ERR_INTERN      16      /* internal error code    */
#define MPI_ERR_IN_STATUS   17      /* Look in status for error value */
#define MPI_ERR_PENDING     18      /* Pending request */
#define MPI_ERR_REQUEST     19      /* Illegal mpi_request handle */
#define MPI_ERR_PERM_GROUP  20	    /* Try to free a permanent group */
#define MPI_ERR_CANCEL  	21	    /* request has been cancelled by peer */
#define MPI_ERR_NRDY  		22	    /* ready send, peer not ready */
#define MPI_ERR_PARAM  		23	    /* parameter error */
#define MPI_ERR_LASTCODE    (256*16+18)      /* Last error code*/

/* These should stay ordered */
#define MPI_IDENT     0
#define MPI_CONGRUENT 1
#define MPI_SIMILAR   2
#define MPI_UNEQUAL   3

/* Pre-defined constants */
#define MPI_UNDEFINED      (-32766)
#define MPI_UNDEFINED_RANK MPI_UNDEFINED
#define MPI_KEYVAL_INVALID 0

/* Upper bound on the overhead in bsend for each message buffer */
#define MPI_BSEND_OVERHEAD 512

/* Topology types */
#define MPI_GRAPH  1
#define MPI_CART   2

#define MPI_BOTTOM      (void *)0

#define MPI_PROC_NULL   (-1)
#define MPI_ANY_SOURCE 	(-2)
#define MPI_ANY_TAG	(-1)

#define MPI_VERSION 1
#define MPI_SUBVERSION 1
#define MPI_TAG_UB 80
#define MPI_HOST 82
#define MPI_IO 84
#define MPI_MAX_PROCESSOR_NAME 1024
#endif
