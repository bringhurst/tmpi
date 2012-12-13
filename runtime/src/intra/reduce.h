# ifndef _REDUCE_H_
# define _REDUCE_H_

# include "ukey.h"
# include "thread.h"
# include "mpi_struct.h"

typedef struct tmpi_RDCHandle {
	struct tmpi_RDCHandle *next;
	int avail; 
	void *data;
	unsigned int count; /* in the unit of datatype */
	thr_mtx_t lock;
	thr_cnd_t wait;
} tmpi_RDCHandle;
	
typedef struct tmpi_RDCQueue {
	ukey_t key_curhandle; /* each process keeps the current handle pointer */
	struct tmpi_RDCHandle *free;
	struct tmpi_RDCHandle *first;
	struct tmpi_RDCHandle cache[1]; /* a variable length array */
} tmpi_RDCQueue;	

typedef struct tmpi_ReduceObj {
	int group_size;
	tmpi_RDCQueue *(rdco_array[1]); /* indexed by the root, variable length */
} tmpi_ReduceObj;
	
/* reduce all object */
typedef struct tmpi_ARDCObj {
  int group_size;
  struct _sub {
    int avail;
    int done;
    void *data;
    unsigned int count;
  } sub[2];
  struct _sub *subp;
	thr_mtx_t lock;
	thr_cnd_t wait;
} tmpi_AllreduceObj;

tmpi_ReduceObj *tmpi_rdco_create(int groupsize); /* constructor */
int tmpi_rdco_destroy(tmpi_ReduceObj *rdco); /* destructor */
int tmpi_rdco_reduce(tmpi_ReduceObj *rdco, int myid, int root, void *sendbuf, void *recvbuf, unsigned int count, MPI_Datatype datatype, MPI_Op operator);

tmpi_AllreduceObj *tmpi_ardco_create(int groupsize); /* constructor */
int tmpi_ardco_destroy(tmpi_AllreduceObj *ardco); /* destructor */
int tmpi_ardco_allreduce(tmpi_AllreduceObj *ardco, int myid, void *sendbuf, void *recvbuf, unsigned int count, MPI_Datatype datatype, MPI_Op operator);

# endif
