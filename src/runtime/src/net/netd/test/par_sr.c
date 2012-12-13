/**
 * Check if NETD send/recv is multi-thread safe
 */
# include <stdio.h>
# include <stdlib.h>
# include "thread.h"
# include "atomic.h"
# include "netd.h"
# include "netd_comm.h"

volatile int b1=0, b2=0;
# define NWORKER 2

# define NITER1 1000000
# define NITER2 10000

int peer_id;
int niter1, niter2;

void *worker(void *arg)
{
	int i;
	int tid=(int)arg;
	char msg1[100]="Hello!";
	char msg2[100];

	inc(&b1);
	while (b1!=(NWORKER));
	inc(&b2);
	while (b2!=(NWORKER));

	if (tid==0) {
		for (i=0; i<niter1; i++) {
			netd_senddat(peer_id, msg1, 100);
		}
	}
	else {
		for (i=0; i<niter2; i++) {
			netd_recvdat(peer_id, msg2, 100);
		}
	}

	dec(&b1);
	while (b1!=0);
	dec(&b2);
	while (b2!=0);

	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t thrs[NWORKER];
	int i;
	int my_id;
	Machine machines[5]={
		{"local", 1},
		{"node11", 1},
		{"nothing", -1}
	};
	char prompt[1024];

    netd_initEnv(&argc,&argv, NULL);

	netd_start(machines);

	/* I know there will be only two people */
	my_id=netd_getMyID();
	peer_id=1-my_id;

	if (my_id==0) {
		niter1=NITER1;
		niter2=NITER2;
	}
	else {
		niter1=NITER2;
		niter2=NITER1;
	}

	if (my_id<2) {
		for (i=0; i<NWORKER; i++)
			pthread_create(thrs+i, NULL, worker, (void *)i);

		for (i=0; i<NWORKER; i++)
			pthread_join(thrs[i], NULL);
	}

    netd_waitAll();

	return 0;
}
