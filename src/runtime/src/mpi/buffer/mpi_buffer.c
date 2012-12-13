/** 
 * MPI wrapper for message buffer
 */

# include "mpi_def.h"
# include "mbuffer.h"
# include "ukey.h"

/* attach user defined buffer, implement MPI_Buffer_attach */
int tmpi_Buffer_attach(void *buf, int bsize) {
	int myid=ukey_getid();

	if (mb_attach_buffer(myid, buf, bsize)<0)
		return MPI_ERR_BUFFER;

	return MPI_SUCCESS;
}

/* detach user defined buffer, implement MPI_Buffer_detach */
int tmpi_Buffer_detach(void *buf, int *bsize) {
	int myid=ukey_getid();
  
	if (mb_detach_buffer(myid, buf, bsize)<0)
		return MPI_ERR_BUFFER;

	return MPI_SUCCESS;
}
