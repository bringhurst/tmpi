#include <stdio.h>
#include "../netd.h"
#include "../netd_comm.h"
#include "usc_time.h"
    
# define FALSE 0
# define TRUE 1
static void dump_args(char *prompt, int argc, char *argv[])
{
	int i;

	printf("%s\n", prompt);

	for (i=0; i<argc; i++)
		printf("%d) %s\n", i+1, argv[i]);
}

static void check_master(char *prompt)
{
	printf("%s\n", prompt);

	printf("myid = %d, ", netd_getMyID());

	if (netd_amIStartupMaster()) {
		printf("I am the startup master.\n");
	}
	else {
		printf("I am not the startup master.\n");
	}
}

void master();
void worker();

int main(int argc, char *argv[])
{
	Machine machines[5]={
		{"local", 1},
		{"node11", 1},
		{"node14", 1},
		{"node13", 1},
		{"nothing", -1}
	};
	dump_args("Before initenv", argc, argv);
    netd_initEnv(&argc,&argv, NULL);
	dump_args("After initenv", argc, argv);
	check_master("Before create procgroup");
    netd_start(machines);
	check_master("After create procgroup");

    if (netd_getMyID() == 0)
    {
	master();
    }
    else
    {
	worker();
    }

    netd_waitAll();

	return 0;
}

void master()
{
    char msg[200];
    char msg2[200];
	char END[200]={'\1', '\0'};
    int i, size;
    char *cp;
    usc_time_t start_time,end_time;
    
    size = netd_getSize();
    
    printf("enter a string:\n");
    while (gets(msg) != NULL)
    {
        for(cp=msg, i=1; *cp; i++, cp++)
	    if(*cp == '\n')
	    {
		*cp = 0;
		break;
	    }
	start_time = usc_gettime();
	netd_senddat(1, msg, 200);
	netd_recvdat(size-1, msg2, 200);
	end_time = usc_gettime();
	printf("total time=%f seconds \n", usc_difftime(end_time,start_time)/1000000);
	printf("master received :%s: from %d\n", msg2, size-1);
	printf("enter a string:\n");
    }
    
    netd_senddat(1, END, 200);
    netd_recvdat(size-1, msg2, 200);
    
    printf("master exiting normally\n");
}

void crashme(int *c)
{
	*c=1;
}

void worker()	
{
    int done;
    int my_id, from, to, size;
    char msg[200];
    
    my_id = netd_getMyID();
    size = netd_getSize();
    if (my_id == (size-1))
        to = 0;
    else
		to = my_id + 1;
	from=my_id -1;

	crashme(NULL);

    done = FALSE;
    while (!done)
    {
		netd_recvdat(from, msg, 200);
		if (msg[0]=='\1')
	    	done = TRUE;
		netd_senddat(to, msg, 200);
    }
}

