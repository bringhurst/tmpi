# include <stdio.h>
# include <unistd.h>
# include "mpi.h"

# define NITER 1000

void dumpMsg(int *msg, int n)
{
	int i;
	char mesg[1024];
	int myid;

	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	sprintf(mesg, "From %d, (", myid);

	for (i=0; i<n-1; i++)
		sprintf(mesg+strlen(mesg), "%d, ", msg[i]);
	sprintf(mesg+strlen(mesg), "%d)\n", msg[n-1]);

	printf(mesg);
}

int main(int argc, char **argv)
{
	int myid=-1;
	int size=0;
	int i;
	double t1=0, t2=0;
	int msg[10];
	int result[10];

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	printf("myid=%d\n", myid);
	if (myid==0) {
		printf("comm size =%d\n", size);
	}

	for (i=0; i<10; i++)
		msg[i]=(i+1)*(i+1);

	MPI_Barrier(MPI_COMM_WORLD);
	if (myid==0)
		t1=MPI_Wtime();

	for (i=0; i<NITER; i++) {
		MPI_Reduce((char *)msg,(char *)result,10, MPI_INT, MPI_SUM, i%size, MPI_COMM_WORLD);
		MPI_Reduce((char *)msg,(char *)result,10, MPI_INT, MPI_SUM, i%size, MPI_COMM_WORLD);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	if (myid==0) 
		t2=MPI_Wtime();

	if (myid==0) {
		printf("partially skewed root reduce cost = %lf seconds.\n", (t2-t1)/NITER/2);
	}

	if (myid==0)
		dumpMsg(result, 10);

	MPI_Finalize();
}
