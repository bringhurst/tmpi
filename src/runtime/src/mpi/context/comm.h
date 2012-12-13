# ifndef _COMM_H_
# define _COMM_H_

# include "mpi_struct.h"
# include "tmpi_struct.h"

tmpi_Comm *tmpi_comm_verify(MPI_Comm comm, int myid);
int tmpi_comm_ginit(int size);
int tmpi_comm_gend();
int tmpi_comm_init(int myid);
int tmpi_comm_end(int myid);
int tmpi_pt2pt_context_by_comm(tmpi_Comm * comm);
int tmpi_coll_context_by_comm(tmpi_Comm * comm);
MPI_Comm tmpi_comm_by_context(int context);
int tmpi_comm_global_id(tmpi_Comm *comm, int comm_lrank);

# endif
