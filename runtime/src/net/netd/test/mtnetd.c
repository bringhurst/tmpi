# include <stdio.h>
# include <pthread.h>
# include "../netd.h"
# include "../netd_comm.h"
# include "usc_time.h"
# include "atomic.h"
    
# define FALSE 0
# define TRUE 1

# define NITER 1000

# define NWORKER 1
volatile int b1=0, b2=0;

int nRunners=0;
int myRank=-1;

int handlemsg_master(int from, char *msg, int len, void **result)
{
	if (strcmp(msg, "POINT_TO_POINT")!=0) {
		fprintf(stderr, "netd_msgrecv is not mt safe: MSG=%s.\n", msg);
	}

	return 0;
}

int handlemsg_slave(int from, char *msg, int len, void **input)
{
	int *in=(int *)input;

	if (from!=in[0]) {
		printf("Got unexpected message from %d:%d: %s\n", from, len, msg);
	}

	if (strcmp(msg, "POINT_TO_POINT")!=0) {
		fprintf(stderr, "netd_msgrecv is not mt safe: MSG=%s.\n", msg);
	}

	netd_sendmsg(in[1], msg, len);

	return 0;
}

void *ring_master(void *arg)
{
	char msg[200]="POINT_TO_POINT";
	int i=0;

	/* entry barrier */
	inc(&b1);
	while (b1!=(NWORKER));
	inc(&b2);
	while (b2!=(NWORKER));

	for (; i<NITER; i++) {
		netd_sendmsg(1, msg, 200);
		netd_recvmsg(200, handlemsg_master, NULL, NULL);
	}
	/* exit barrier */
	dec(&b1);
	while (b1!=0);
	dec(&b2);
	while (b2!=0);

	return NULL;
}

void *ring_slave(void *arg)
{
    int fromto[2];
	int i=0;
    
    if (myRank == (nRunners-1))
        fromto[1] = 0;
    else
		fromto[1] = myRank + 1;

	fromto[0]=myRank -1;

	/* entry barrier */
	inc(&b1);
	while (b1!=(NWORKER));
	inc(&b2);
	while (b2!=(NWORKER));

	for (; i<NITER; i++) {
		netd_recvmsg(200, handlemsg_slave, (void **)fromto, NULL);
	}
	/* exit barrier */
	dec(&b1);
	while (b1!=0);
	dec(&b2);
	while (b2!=0);

	return NULL;
}

void *bcast_worker(void *arg)
{
	char msg[200]="BROADCAST";
	int i=0;

	/* entry barrier */
	inc(&b1);
	while (b1!=(NWORKER));
	inc(&b2);
	while (b2!=(NWORKER));

	for (; i<NITER; i++) {
		netd_bcast(msg, 200, 0);
		if (strcmp(msg, "BROADCAST")!=0) {
			fprintf(stderr, "netd_bcast is not mt safe: MSG=%s.\n", msg);
		}
	}

	/* exit barrier */

	dec(&b1);
	while (b1!=0);
	dec(&b2);
	while (b2!=0);
	
	return NULL;
}

int main(int argc, char *argv[])
{
	Machine machines[5]={
		{"local", 1},
		{"node28", 1},
		{"node29", 1},
		{"node30", 1},
		{"", -1}
	};
	pthread_t thr[2];

    netd_initEnv(&argc,&argv, NULL);
    netd_start(machines);

	nRunners=netd_getSize();
	myRank=netd_getMyID();

	if (netd_amIStartupMaster()) {
		pthread_create(&(thr[0]), NULL, ring_master, NULL);
	}
	else {
		pthread_create(&(thr[0]), NULL, ring_slave, NULL);
	}

	/**
	pthread_create(&(thr[1]), NULL, bcast_worker, NULL);
	**/

	pthread_join(thr[0], NULL);
	/**
	pthread_join(thr[1], NULL);
	**/

    netd_waitAll();

	return 0;
}
