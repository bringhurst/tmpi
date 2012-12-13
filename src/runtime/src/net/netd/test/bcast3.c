#include <stdio.h>
#include "../netd.h"
#include "../netd_comm.h"
#include "usc_time.h"
    
# define FALSE 0
# define TRUE 1

# define ITER 1000

void dumpMsg(char *msg)
{
	printf("From %d, msg=%s\n", netd_getMyID(), msg);
}

int main(int argc, char *argv[])
{
	Machine machines[6]={
		{"local", 1},
		{"node37", 1},
		{"node39", 1},
		{"node40", 1},
		{"node42", 1},
		{"", -1}
	};
    char msg[100]="Hello, world!";
	int i, j;
	usc_tmr_t timer;

    netd_initEnv(&argc,&argv, NULL);
    netd_start(machines);
	usc_init();
	usc_tmr_reset(&timer);

	usc_tmr_start(&timer);
	for (j=0; j<ITER; j++) {
		netd_bcast(msg, 100, j%netd_getSize());
	}
	usc_tmr_stop(&timer);

	printf("skewed bcast performance, %fus per op.\n", usc_tmr_read(&timer)/ITER);

    netd_waitAll();

	return 0;
}
