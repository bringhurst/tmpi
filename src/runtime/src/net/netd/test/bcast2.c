#include <stdio.h>
#include "../netd.h"
#include "../netd_comm.h"
#include "usc_time.h"
    
# define FALSE 0
# define TRUE 1

void dumpMsg(char *msg)
{
	printf("From %d, msg=%s\n", netd_getMyID(), msg);
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
    char msg[100];
	int i;

    netd_initEnv(&argc,&argv, NULL);
    netd_start(machines);

	for (i=0; i<netd_getSize(); i++) {
		if (i==netd_getMyID()) {
			sprintf(msg, "Hello from %d", netd_getMyID());
		}
		netd_bcast(msg, 100, i);
		dumpMsg(msg);
	}

    netd_waitAll();

	return 0;
}
