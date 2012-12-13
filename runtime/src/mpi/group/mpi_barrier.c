/*
 * barrier.c -- BARRIER OBJECT INTERFACE 
 */

# include "ukey.h"
# include "barrier.h"
# include "comm.h"
# include "tmpi_debug.h"
# include "mpi_struct.h"
# include "netq_comm.h"
# include "netq.h"

/* implementation for barrier */
int MPI_Barrier(MPI_Comm communicator) {
  	tmpi_Comm* comm;
	int myid=ukey_getid();

	if (communicator!=MPI_COMM_WORLD) {
		tmpi_error(DBG_INTERNAL, "only support comm_world barrier.");
		return MPI_ERR_INTERN;
	}

  	if ((comm=tmpi_comm_verify(communicator, myid))==NULL)
		return MPI_ERR_COMM;

	/* intra barrier */
  	bar_wait(comm->br);

	if (netq_getMacSize()>1) {
		/* not only local groups */
		/* inter barrier */
		if (myid==0) {
			netq_barrier();
		}

		/* intra barrier */
  		bar_wait(comm->br);
	}

  	return MPI_SUCCESS;
}
