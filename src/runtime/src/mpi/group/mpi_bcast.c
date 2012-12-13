# include "bcast.h"
# include "ukey.h"
# include "mpi_struct.h"
# include "tmpi_struct.h"
# include "comm.h"
# include "tmpi_util.h"
# include "intra_sr.h"
# include "tmpi_debug.h"
# include "netq_comm.h"
# include "netq.h"

int tmpi_Bcast( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
	int my_local_rank=ukey_getid();
	int local_root;
	int buflen=count*tmpi_Usize(datatype);
	int ret1=0, ret2;
	tmpi_Comm *tcomm=tmpi_comm_verify(comm, my_local_rank);

	if (comm!=MPI_COMM_WORLD) {
		tmpi_error(DBG_INTERNAL, "Only comm_world bcast supported.");
		return MPI_ERR_INTERN;
	}

	if (tcomm==NULL) 
		return MPI_ERR_COMM;

	if (netq_isLocalNode(root)) {
		local_root=tmpi_grank_to_lrank(root);
	}
	else {
		local_root=0;
	}

	if (my_local_rank==local_root)
		ret1=netq_bcast(buffer, buflen, root);

	ret2=tmpi_bco_bcast(tcomm->bc_ptr, my_local_rank, local_root, (char *)buffer, buflen);
	switch(ret2) {
		case 0  : 
			if (ret1==0) 
				return MPI_SUCCESS;
			else
				return MPI_ERR_INTERN;
		case -1 : return MPI_ERR_INTERN;
		case -2 : return MPI_ERR_BUFFER;
		default : return MPI_ERR_INTERN;
	}
}
