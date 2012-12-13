# include <stdio.h>
# include <unistd.h>
# include "mpi.h"

int main(int argc, char **argv)
{
	int myid=-1;
	int size=0;
	double t1=0, t2=0;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	printf("myid=%d\n", myid);
	if (myid==0) {
		printf("comm size =%d\n", size);
	}

	if (myid==0)
		t1=MPI_Wtime();

	MPI_Barrier(MPI_COMM_WORLD);

	if (myid==0) 
		t2=MPI_Wtime();

	MPI_Finalize();
	return 0;
}
