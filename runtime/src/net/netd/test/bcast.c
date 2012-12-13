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
	Machine machines[6]={
		{"local", 1},
		{"node37", 1},
		{"node39", 1},
		{"node40", 1},
		{"node42", 1},
		{"", -1}
	};
    char msg[100];
	int done=0;

    netd_initEnv(&argc,&argv, NULL);
    netd_start(machines);

	do {
		if (netd_amIStartupMaster()) {
			printf("Enter a string:");
			fflush(stdout);
			if (!fgets(msg, 100, stdin)) {
				msg[0]='\1';
				msg[1]='\0';
				done=1;
			}
			else {
				if (msg[strlen(msg)-1]=='\n')
					msg[strlen(msg)-1]='\0';
			}
		}
		netd_bcast(msg, 100, 0);
		if (!netd_amIStartupMaster()) {
			if (msg[0]=='\1')
				done=1;
			else
				dumpMsg(msg);
		}
	} while (!done);

    netd_waitAll();

	return 0;
}
