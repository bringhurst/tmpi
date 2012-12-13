#include <stdio.h>
#include "../netd.h"
#include "../netd_comm.h"
#include "usc_time.h"
    
# define FALSE 0
# define TRUE 1

void dumpMsg(int *msg, int n)
{
	int i;
	char mesg[1024];

	sprintf(mesg, "From %d, (", netd_getMyID());

	for (i=0; i<n-1; i++)
		sprintf(mesg+strlen(mesg), "%d, ", msg[i]);
	sprintf(mesg+strlen(mesg), "%d)\n", msg[n-1]);

	printf(mesg);
}

# define ITER 1000

int main(int argc, char *argv[])
{
	Machine machines[5]={
		{"local", 1},
		{"node37", 1},
		{"node39", 1},
		{"node40", 1},
		{"", -1}
	};
    int msg[10];
    int msg2[10];
	int i;
	usc_tmr_t timer;

    netd_initEnv(&argc,&argv, NULL);
    netd_start(machines);
	usc_init();
	usc_tmr_reset(&timer);

	for (i=0; i<10; i++)
		msg[i]=(i+1)*(i+1);

	usc_tmr_start(&timer);
	for (i=0; i<ITER; i++) {
		if (netd_reduce((char *)msg,(char *)msg2,10,NETD_OP_SUM,NETD_TYPE_INT,0)<0) {
			printf("error in reduce.");
			break;
		}
	}
	usc_tmr_stop(&timer);

	dumpMsg(msg2, 10);
	
	printf("reduce performance %fus per op\n", usc_tmr_read(&timer)/ITER);

    netd_waitAll();

	return 0;
}
