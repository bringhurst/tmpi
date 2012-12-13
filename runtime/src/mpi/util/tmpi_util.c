# include "tmpi_util.h"
# include "mpi_struct.h"
# include "tmpi_debug.h"
# include "usc_time.h"
# include "mpi_def.h"
# include "thread.h"

int tmpi_Usize(MPI_Datatype unit)
{
  switch (unit) {
  case MPI_BYTE: return sizeof(char);
  case MPI_CHAR: return sizeof(char);
  case MPI_SHORT: return sizeof(short);
  case MPI_INT: return sizeof(int);
  case MPI_LONG: return sizeof(long);
  case MPI_UNSIGNED_LONG: return sizeof(unsigned long);
  case MPI_FLOAT: return sizeof(float);
  case MPI_DOUBLE: return sizeof(double);
  case MPI_LONG_DOUBLE: return sizeof(long double);

  /* deal with stupid FORTRAN */
  case MPI_DOUBLE_PRECISION: return sizeof(double);
  case MPI_INTEGER: return sizeof(int);
  case MPI_LOGICAL: return sizeof(int);
  default:
	tmpi_error(DBG_INTERNAL, "MPI_Usize: type not supported.");
    return sizeof(int);
  }
}

/* Wall lock time is used through gettimeofday */
double tmpi_Wtime() {
	return usc_wtime(usc_gettime())/1000000;
}

double tmpi_Wtick() {
	/* us resolution */
  return 0.000001;
}

int tmpi_Init()
{
	return MPI_SUCCESS;
}

int tmpi_Finalize()
{
	return MPI_SUCCESS;
}

int tmpi_Abort(MPI_Comm comm, int code)
{
	if (comm==MPI_COMM_WORLD) {
		MPI_Finalize();
		// tmpi_user_end();
		thr_exit(code);
	}

	/* should never reach here */
	return code;
}

int tmpi_Get_count(MPI_Status *status, MPI_Datatype datatype, int *count)
{
	if (!status)
		return MPI_ERR_ARG;

	if (status->buflen%tmpi_Usize(datatype)!=0)
		return MPI_ERR_ARG;

	*count=status->buflen/tmpi_Usize(datatype);

	return MPI_SUCCESS;
}
