# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <unistd.h>

#include "mpi.h"

#define BUFLEN 512

static void random_wait()
{
	usleep((rand()%100)*10000);
}

int main(int argc, char *argv[])
{
	int i, myid, numprocs;
	int io_rank=0;
	char buffer[BUFLEN];
	int report_error=1;
	MPI_Status status;

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);

	if (numprocs!=2) {
		if (myid==io_rank) {
			printf("ping pong test requires exactly 2 nodes.\n");
		}
		MPI_Finalize();
		return -1;
	}

	MPI_Barrier(MPI_COMM_WORLD);
	i=100;
	while (i--) {
	if (myid == 0) {
		char result[BUFLEN];

		sprintf(buffer, "Hello %d", i);
		sprintf(result, "%s%s",buffer, buffer);

		MPI_Send(buffer, BUFLEN, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
		random_wait();
		// MPI_Recv(buffer, BUFLEN, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
		MPI_Recv(buffer, BUFLEN/2, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &status);
		random_wait();
		// printf("Got message %s\n", buffer);
		if (strcmp(result, buffer)) {
			if (report_error) {
				printf("Error!\n");
				report_error=0;
			}
		}
		printf(".");
		fflush(stdout);
	} 
	else {
		int len;

		MPI_Recv(buffer, BUFLEN/2, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
		random_wait();
		len=strlen(buffer);
		strncpy(buffer+len, buffer, len);
		buffer[len*2]='\0';
		// MPI_Send(buffer, BUFLEN, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
		MPI_Send(buffer, BUFLEN, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
		random_wait();
	}
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	return 0;
}
