# include "comm.h"
# include "reduce.h"
# include "ukey.h"
# include "mpi_struct.h"
# include "tmpi_struct.h"
# include "netq.h"
# include "netq_comm.h"
# include "intra_sr.h"
# include "tmpi_util.h"
# include "stdlib.h"
# include "tmpi_debug.h"

/* return 0 for non-default ops */
static int op_mpi2netq(int op)
{
	switch(op) {
		case MPI_MAX :
			return NETQ_OP_MAX;
		case MPI_MIN  :
			return NETQ_OP_MIN;
		case MPI_SUM  :
			return NETQ_OP_SUM;
		case MPI_PROD :
		case MPI_LAND :
		case MPI_BAND :
		case MPI_LOR  :
		case MPI_BOR  :
		case MPI_LXOR :
		case MPI_BXOR :
		case MPI_MINLOC :
		case MPI_MAXLOC :
			return 0;
		default :
			return -1;
	}
}

static int type_mpi2netq(int type)
{
	switch (type) {
		case MPI_INT :
		case MPI_INTEGER :
			return NETQ_TYPE_INT;

		case MPI_LONG :
			return NETQ_TYPE_LNG;

		case MPI_FLOAT :
		case MPI_REAL :
			return NETQ_TYPE_FLT;

		case MPI_DOUBLE :
		case MPI_DOUBLE_PRECISION :
			return NETQ_TYPE_DBL;

		case MPI_CHAR :
		case MPI_BYTE :
			return NETQ_TYPE_CHR;

		case MPI_SHORT :
		case MPI_LONG_LONG_INT :
		case MPI_UNSIGNED_SHORT :
		case MPI_UNSIGNED :
		case MPI_UNSIGNED_LONG :
		case MPI_LONG_DOUBLE :
		case MPI_UNSIGNED_CHAR :
		case MPI_PACKED :
		case MPI_LB :
		case MPI_UB :
		case MPI_FLOAT_INT :
		case MPI_DOUBLE_INT :
		case MPI_LONG_INT :
		case MPI_SHORT_INT :
		case MPI_2INT :
		case MPI_LONG_DOUBLE_INT :
		case MPI_COMPLEX :
		case MPI_DOUBLE_COMPLEX :
		case MPI_LOGICAL :
		case MPI_2INTEGER :
		case MPI_2COMPLEX :
		case MPI_2DOUBLE_COMPLEX :
		case MPI_2REAL :
		case MPI_2DOUBLE_PRECISION :
			return 0;

		default :
			return -1;
	}
}

int tmpi_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
		MPI_Op op, int root, MPI_Comm comm)
{
	int my_local_id=ukey_getid();
	int local_root;
	tmpi_Comm *tcomm=tmpi_comm_verify(comm, my_local_id);
	int ret1=0, ret2=0;
	int netq_type, netq_op;
	int buflen=count*tmpi_Usize(datatype);
	void *tmp_buf=malloc(buflen);

	if (comm!=MPI_COMM_WORLD) {
		tmpi_error(DBG_INTERNAL, "only comm_world reduce supported.");
		return MPI_ERR_INTERN;
	}

	if (tcomm==NULL) return MPI_ERR_COMM;

	if (!tmp_buf)
		return MPI_ERR_INTERN;

	if (netq_isLocalNode(root)) {
		local_root=tmpi_grank_to_lrank(root);
	}
	else {
		local_root=0;
	}

	if (netq_getMacSize()>1) {
		ret1=tmpi_rdco_reduce(tcomm->rdc_ptr, my_local_id, local_root, sendbuf, tmp_buf, count, datatype, op);

		if (my_local_id==local_root) {
			netq_op=op_mpi2netq(op);
			netq_type=type_mpi2netq(datatype);
			ret2=netq_reduce(tmp_buf, recvbuf, count, netq_op, netq_type, root);
		}
	}
	else {
		ret1=tmpi_rdco_reduce(tcomm->rdc_ptr, my_local_id, root, sendbuf, recvbuf, count, datatype, op);
		ret2=MPI_SUCCESS;
	}

	free(tmp_buf);

	switch (ret1) {
		case 0  : 
			if (ret2==0) 
				return MPI_SUCCESS;
			return MPI_ERR_INTERN;
		case -1 : return MPI_ERR_INTERN;
		case -2 : return MPI_ERR_BUFFER;
		default : return MPI_ERR_INTERN;
	}
}

/**
int tmpi_Bcast( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);

int tmpi_Allreduce(void *sendbuf, void *recvbuf, int count, 
		MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int ret1, ret2;

	ret1=tmpi_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
	ret2=tmpi_Bcast(recvbuf, count, datatype, 0, comm);
	if (ret1!=MPI_SUCCESS)
		return ret1;

	return ret2;
}
**/

int tmpi_Allreduce(void *sendbuf, void *recvbuf, int count, 
		MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int my_local_id=ukey_getid();
	tmpi_Comm *tcomm=tmpi_comm_verify(comm, my_local_id);
	int ret1=0, ret2=0, ret3=0;
	int netq_type, netq_op;
	int buflen=count*tmpi_Usize(datatype);
	void *tmp_buf=NULL;


	if (comm!=MPI_COMM_WORLD) {
		tmpi_error(DBG_INTERNAL, "allreduce only support comm_world.");
		return MPI_ERR_INTERN;
	}

   	if (tcomm==NULL) 
		return MPI_ERR_COMM;

	if (netq_getMacSize()>1) {
		tmp_buf=malloc(buflen);
		if (!tmp_buf)
			return MPI_ERR_INTERN;

		ret1=tmpi_rdco_reduce(tcomm->rdc_ptr, my_local_id, 0, sendbuf, tmp_buf, count, datatype, op);

		if (my_local_id==0) {
			netq_op=op_mpi2netq(op);
			netq_type=type_mpi2netq(datatype);
			ret2=netq_allreduce(tmp_buf, recvbuf, count, netq_op, netq_type);
		}

		ret3=tmpi_bco_bcast(tcomm->bc_ptr, my_local_id, 0, (char *)recvbuf, buflen);
		free(tmp_buf);
	}
	else {
		/* this does sound weird, but my specific allreduce function works poorer than
		 * a reduce + bcast
		 */
		ret1=tmpi_rdco_reduce(tcomm->rdc_ptr, my_local_id, 0, sendbuf, recvbuf, count, datatype, op);
		ret3=tmpi_bco_bcast(tcomm->bc_ptr, my_local_id, 0, (char *)recvbuf, buflen);
		/**
		ret1=tmpi_ardco_allreduce(tcomm->ardc_ptr, my_local_id, sendbuf, recvbuf, count, datatype, op);
		ret3=MPI_SUCCESS;
		**/
	}

	if ((!ret1)&&(!ret2)&&(!ret3))
		return MPI_SUCCESS;

	if (ret2!=0)
		return MPI_ERR_INTERN;

	if ( (ret1==-1)||(ret2==-1) )
		return MPI_ERR_INTERN;

	if ( (ret1==-2)||(ret2==-2) )
   		return MPI_ERR_BUFFER;

   	return MPI_ERR_INTERN;
}
