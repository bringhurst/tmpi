#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
    
# define FALSE 0
# define TRUE 1

void master();
void worker();

int main(int argc, char *argv[])
{
	int myid, numprocs;
	int io_rank=0;

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);

	MPI_Barrier(MPI_COMM_WORLD);

	if (numprocs!=2) {
		if (myid==io_rank) {
			printf("ping pong test requires exactly 2 nodes.\n");
		}
		MPI_Finalize();
		return -1;
	}

	if (myid==0) { 
		master(); 
	} else { 
		worker(); 
	}

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	return 0;
}

# define KILO 0x400
# define MEGA 0x100000
# define ITER 1000
# define ITER2 100
# define ITER3 20
/**
# define ITER 10
# define ITER2 10
# define ITER3 10
**/

int short_size[][18]={
	{4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, -1},
	{72, 80, 88, 96, 104, 112, 120, 128, -1},
	{144, 160, 176, 192, 208, 224, 240, 256, -1},
	{288, 320, 352, 384, 416, 448, 480, 512, -1},
	{576, 640, 704, 768, 832, 896, 960, 1024, -1}
};

void master()
{
    char *msg;
    int size;
	double t1, t2;
	int i, j, k;
	MPI_Status status;
	int report_error=1;
    
    
	msg=(char *)malloc(MEGA);
	if (msg) {
		memset(msg, 0x55, MEGA);
	}
	else {
		printf("master: Cannot allocate memory.\n");
		return;
	}

	/* short */
	for (i=0; i<5; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;
			t1=MPI_Wtime();
			for (k=0; k<ITER; k++) {
				if (MPI_Send(msg, size, MPI_CHAR, 1, 0, MPI_COMM_WORLD)!=MPI_SUCCESS) {
					if (report_error) {
						fprintf(stderr, "short MPI_Send failed\n");
						report_error=0;
					}
				}
				if (MPI_Recv(msg, size, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &status)!=MPI_SUCCESS) {
					if (report_error) {
						fprintf(stderr, "short MPI_Recv failed\n");
						report_error=0;
					}
				}
			}
			t2=MPI_Wtime();
			printf("Ping Pong (short), size=%d, iteration=%d, single round trip=%fus, rate=%fMB/s.\n", 
					size, ITER, (t2-t1)*MEGA/ITER, (float)size*2/((t2-t1)*MEGA/ITER));
		}
	}

	/* long */

	for (i=0; i<2; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;
			size*=KILO;
			t1=MPI_Wtime();
			for (k=0; k<ITER2; k++) {
				if (MPI_Send(msg, size, MPI_CHAR, 1, 0, MPI_COMM_WORLD)!=MPI_SUCCESS) {
					if (report_error) {
						fprintf(stderr, "long MPI_Send failed\n");
						report_error=0;
					}
				}
				if (MPI_Recv(msg, size, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &status)!=MPI_SUCCESS) {
					if (report_error) {
						fprintf(stderr, "long MPI_Recv failed\n");
						report_error=0;
					}
				}
			}
			t2=MPI_Wtime();
			printf("Ping Pong (long), size=%d, iteration=%d, transfer rate=%fMB/s.\n", 
					size, ITER2, (float)size*2/((t2-t1)*MEGA/ITER2));
		}
	}

	for (i=2; i<5; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;

			size*=KILO;
			t1=MPI_Wtime();
			for (k=0; k<ITER3; k++) {
				if (MPI_Send(msg, size, MPI_CHAR, 1, 0, MPI_COMM_WORLD)!=MPI_SUCCESS) {
					if (report_error) {
						fprintf(stderr, "very long MPI_Send failed\n");
						report_error=0;
					}
				}
				if (MPI_Recv(msg, size, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &status)!=MPI_SUCCESS) {
					if (report_error) {
						fprintf(stderr, "very long MPI_Recv failed\n");
						report_error=0;
					}
				}
			}
			t2=MPI_Wtime();
			printf("Ping Pong (very long), size=%d, iteration=%d, transfer rate=%fMB/s.\n", 
					size, ITER3, (float)size*2/((t2-t1)*MEGA/ITER3));
		}
	}

    printf("master exiting normally\n");
}

void worker()	
{
    char *msg;
    int size;
	int i, j, k;
	MPI_Status status;
    
    
	msg=(char *)malloc(MEGA);
	if (msg)  {
		memset(msg, 0x55, MEGA);
	}
	else {
		printf("worker: Cannot allocate memory.\n");
		return;
	}

	/* short */
	for (i=0; i<5; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;
			for (k=0; k<ITER; k++) {
				MPI_Recv(msg, size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
				MPI_Send(msg, size, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
			}
		}
	}

	/* long */

	for (i=0; i<2; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;

			size*=KILO;
			for (k=0; k<ITER2; k++) {
				MPI_Recv(msg, size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
				MPI_Send(msg, size, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
			}
		}
	}

	for (i=2; i<5; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;

			size*=KILO;
			for (k=0; k<ITER3; k++) {
				MPI_Recv(msg, size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
				MPI_Send(msg, size, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
			}
		}
	}
}
