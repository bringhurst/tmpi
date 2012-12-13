/*
 * comm.c -- operations on communicator
 */

# include <stdlib.h>
# include "mpi_struct.h"
# include "tmpi_struct.h"
# include "ukey.h"
# include "comm.h"
# include "netq.h"
# include "mpi.h"
# include "bcast.h"
# include "reduce.h"

#define MPI_COMM_WORLD_PT2PT_CONTEXT 0x02020202
#define MPI_COMM_WORLD_COLL_CONTEXT 0x04040404

/* the length of the array is netq_getLocalSize() */
static tmpi_Comm **tmpi_COMM_WORLD=NULL; 
static tmpi_Bar *tmpi_COMM_WORLD_Bar=NULL; 
static tmpi_BcastObj *tmpi_COMM_WORLD_Bcast=NULL; 
static tmpi_ReduceObj *tmpi_COMM_WORLD_Reduce=NULL; 
static tmpi_AllreduceObj *tmpi_COMM_WORLD_Allreduce=NULL; 

/**
static tmpi_Bar *MPI_COMM_WORLD_BAR;
static tmpi_BcastObj *MPI_COMM_WORLD_BC;
static tmpi_ReduceObj *MPI_COMM_WORLD_RDC;
static tmpi_AllreduceObj *MPI_COMM_WORLD_ARDC;
**/

tmpi_Comm *tmpi_comm_verify(MPI_Comm comm, int myid)
{
	if (comm==MPI_COMM_WORLD) {
		return tmpi_COMM_WORLD[myid];
	}

	if (comm->cookie==MPI_COMM_COOKIE)
		return comm;

	return NULL;
}

int tmpi_comm_ginit(int size)
{
	tmpi_COMM_WORLD=(tmpi_Comm **)calloc(size, sizeof(tmpi_Comm *));
	if (!tmpi_COMM_WORLD) {
		goto _error_out1;
	}

	tmpi_COMM_WORLD_Bar=(tmpi_Bar *)malloc(sizeof(tmpi_Bar)); 
	if (!tmpi_COMM_WORLD_Bar) {
		goto _error_out1;
	}

	if (bar_init(tmpi_COMM_WORLD_Bar, size)<0) {
		goto _error_out1;
	}

	tmpi_COMM_WORLD_Bcast=tmpi_bco_create(size);
	tmpi_COMM_WORLD_Reduce=tmpi_rdco_create(size);
	tmpi_COMM_WORLD_Allreduce=tmpi_ardco_create(size);

  	if (
			  (!tmpi_COMM_WORLD_Bar)
			||(!tmpi_COMM_WORLD_Reduce)
			||(!tmpi_COMM_WORLD_Allreduce)
		)
		goto _error_out1;

	return MPI_SUCCESS;

_error_out1 :
	tmpi_comm_gend();
	return MPI_ERR_INTERN;
}

int tmpi_comm_gend()
{
	if (tmpi_COMM_WORLD) {
		free(tmpi_COMM_WORLD);
		tmpi_COMM_WORLD=NULL;
	}

	if (tmpi_COMM_WORLD_Bar) {
		bar_destroy(tmpi_COMM_WORLD_Bar);
		free(tmpi_COMM_WORLD_Bar);
		tmpi_COMM_WORLD_Bar=NULL;
	}

	if (tmpi_COMM_WORLD_Bcast) {
		tmpi_bco_destroy(tmpi_COMM_WORLD_Bcast);
		tmpi_COMM_WORLD_Bcast=NULL;
	}

	if (tmpi_COMM_WORLD_Bcast) {
		tmpi_rdco_destroy(tmpi_COMM_WORLD_Reduce);
		tmpi_COMM_WORLD_Reduce=NULL;
	}

	if (tmpi_COMM_WORLD_Allreduce) {
		tmpi_ardco_destroy(tmpi_COMM_WORLD_Allreduce);
		tmpi_COMM_WORLD_Allreduce=NULL;
	}

	return MPI_SUCCESS;
}

/* initialize the MPI_COMM_WORLD communicator */
int tmpi_comm_init(int myid) {
  int i;
  int nproc=netq_getGlobalSize();
  int grank;

  if (!tmpi_COMM_WORLD)
	  goto _error_out;

  tmpi_COMM_WORLD[myid] = (tmpi_Comm*)calloc(1, sizeof(tmpi_Comm));
  if (!tmpi_COMM_WORLD[myid])
	  goto _error_out;

  tmpi_COMM_WORLD[myid]->cookie = MPI_COMM_COOKIE;
  tmpi_COMM_WORLD[myid]->np = nproc;
  netq_transLocalRank(myid, &grank);
  tmpi_COMM_WORLD[myid]->local_rank=grank;
  tmpi_COMM_WORLD[myid]->lrank2grank = (int*) malloc(nproc*sizeof(int));

  if (!tmpi_COMM_WORLD[myid]->lrank2grank)
	  goto _error_out;

  for (i=0; i<nproc; i++)
    tmpi_COMM_WORLD[myid]->lrank2grank[i] = i;

  tmpi_COMM_WORLD[myid]->pt2pt_context = MPI_COMM_WORLD_PT2PT_CONTEXT;
  tmpi_COMM_WORLD[myid]->coll_context = MPI_COMM_WORLD_COLL_CONTEXT;
  tmpi_COMM_WORLD[myid]->comm_type = INTRA;
  tmpi_COMM_WORLD[myid]->group = (tmpi_Group*) malloc(sizeof(tmpi_Group));
  if (!tmpi_COMM_WORLD[myid]->group)
	  goto _error_out;

  tmpi_COMM_WORLD[myid]->group->cookie = MPI_GROUP_COOKIE;
  tmpi_COMM_WORLD[myid]->group->np = nproc;
  tmpi_COMM_WORLD[myid]->group->local_rank = grank;
  tmpi_COMM_WORLD[myid]->group->ref_count = 1;
  tmpi_COMM_WORLD[myid]->group->isPermanent = 1;
  tmpi_COMM_WORLD[myid]->group->lrank2grank = tmpi_COMM_WORLD[myid]->lrank2grank;
  tmpi_COMM_WORLD[myid]->ref_count = 1;
  tmpi_COMM_WORLD[myid]->bc_ptr = tmpi_COMM_WORLD_Bcast;
  tmpi_COMM_WORLD[myid]->rdc_ptr = tmpi_COMM_WORLD_Reduce;
  tmpi_COMM_WORLD[myid]->ardc_ptr = tmpi_COMM_WORLD_Allreduce;
  tmpi_COMM_WORLD[myid]->comm_cache = NULL;
  tmpi_COMM_WORLD[myid]->attr_cache = NULL;
  tmpi_COMM_WORLD[myid]->isPermanent = 1;
  tmpi_COMM_WORLD[myid]->br = tmpi_COMM_WORLD_Bar;

  return MPI_SUCCESS;

_error_out :
  tmpi_comm_end(myid);
  return MPI_ERR_INTERN;
}

int tmpi_comm_end(int myid)
{
	if (tmpi_COMM_WORLD[myid]) {
		if (tmpi_COMM_WORLD[myid]->group) {
			free(tmpi_COMM_WORLD[myid]->group);
			tmpi_COMM_WORLD[myid]->group=NULL;
		}
		if (tmpi_COMM_WORLD[myid]->lrank2grank) {
			free(tmpi_COMM_WORLD[myid]->lrank2grank);
			tmpi_COMM_WORLD[myid]->lrank2grank=NULL;
		}

		free(tmpi_COMM_WORLD[myid]);
		tmpi_COMM_WORLD[myid]=NULL;
	}

	return 0;
}

/* get the number of processes in the communicator */
int MPI_Comm_size(MPI_Comm communicator, int *size) {
  	tmpi_Comm* comm;
	int myid=ukey_getid();

  	if ((comm=tmpi_comm_verify(communicator, myid))==NULL) 
		return MPI_ERR_COMM;

  	*size=comm->np;
	
  	return MPI_SUCCESS;
}

/* get the rank of calling process in this communicator */
int MPI_Comm_rank(MPI_Comm communicator, int *rank) {
  	tmpi_Comm *comm;
	int myid=ukey_getid();

  	if ((comm=tmpi_comm_verify(communicator, myid))==NULL) 
		return MPI_ERR_COMM;

  	*rank=comm->local_rank;

  	return MPI_SUCCESS;
}

/* compare two MPI communicators */
int MPI_Comm_compare(MPI_Comm communicator1, MPI_Comm communicator2, int *result) {
  	tmpi_Comm *comm1, *comm2;
  	int group_result;
	int myid=ukey_getid();

  	if ((comm1=tmpi_comm_verify(communicator1, myid))==NULL) 	
		return MPI_ERR_COMM;
  	if ((comm2=tmpi_comm_verify(communicator2, myid))==NULL) 
		return MPI_ERR_COMM;

  	MPI_Group_compare(comm1->group, comm2->group, &group_result);
  	switch (group_result) {
  	case MPI_IDENT:
    		if (comm1->pt2pt_context==comm2->pt2pt_context && \
		    comm1->coll_context==comm2->coll_context)
      			*result=MPI_IDENT;
    		else
      			*result=MPI_CONGRUENT;
    		break;
  	case MPI_SIMILAR:
    		*result=MPI_SIMILAR;
    		break;
  	case MPI_UNEQUAL:
    		*result=MPI_UNEQUAL;
    		break;
  	}

  	return MPI_SUCCESS;
}

int tmpi_pt2pt_context_by_comm(tmpi_Comm *comm)
{
	return comm->pt2pt_context;
}

int tmpi_coll_context_by_comm(tmpi_Comm *comm)
{
	return comm->pt2pt_context;
}

MPI_Comm tmpi_comm_by_context(int context)
{
	if ( (context==MPI_COMM_WORLD_PT2PT_CONTEXT) ||
		(context==MPI_COMM_WORLD_COLL_CONTEXT) )
		return MPI_COMM_WORLD;

	return MPI_COMM_NULL;
}

/* translate the rank inside a comm to the global id */
int tmpi_comm_global_id(tmpi_Comm *comm, int comm_lrank)
{
	if (comm)
		return (comm->lrank2grank[comm_lrank]);

	return -1;
}

