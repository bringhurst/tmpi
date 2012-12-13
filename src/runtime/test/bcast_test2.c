# include <stdio.h>
# include <unistd.h>
# include "mpi.h"

# define NITER 1000

int main(int argc, char **argv)
{
	int myid=-1;
	int size=0;
	int i;
	double t1=0, t2=0;
	char msg[100]="Hello!";

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	printf("myid=%d\n", myid);
	if (myid==0) {
		printf("comm size =%d\n", size);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	if (myid==0)
		t1=MPI_Wtime();

	for (i=0; i<NITER; i++)
		MPI_Bcast(msg, 100, MPI_CHAR, i%size, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);
	if (myid==0) 
		t2=MPI_Wtime();

	if (myid==0) {
		printf("skewed root bcast cost = %lf seconds.\n", (t2-t1)/NITER);
	}

	MPI_Finalize();
	return 0;
}
