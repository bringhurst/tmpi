#include <stdio.h>
#include "../netd.h"
#include "../netd_comm.h"
#include "usc_time.h"
    
# define FALSE 0
# define TRUE 1

void master();
void worker();

int main(int argc, char *argv[])
{
	Machine machines[5]={
		{"local", 1},
		{"node29", 1},
		{"node30", 1},
		{"node28", 1},
		{"nothing", -1}
	};
    netd_initEnv(&argc,&argv, NULL);
    netd_start(machines);
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

# define DONE -2000

int handlemsg_master(int from, char *msg, int len, void **result)
{
	int ret=0;

	if (len>0) {
		if (msg[0]=='\1') {
			printf("Successful termination.");
			ret=DONE;
		}
		else {
			printf("Got message from %d:%d: %s\n", from, len, msg);
		}
	}

	return ret;
}

int handlemsg_worker(int from, char *msg, int len, void **input)
{
	int *in=(int *)input;

	if (from!=in[0]) {
		printf("Got unexpected message from %d:%d: %s\n", from, len, msg);
	}
	else {
		netd_sendmsg(in[1], msg, len);
	}

	if (msg[0]=='\1')
		return DONE;

	return 0;
}

void master()
{
    char msg[1024];
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
		/* netd_sendmsg(1, msg, 200); */
		netd_sendmsg(1, msg, strlen(msg)+1);
		netd_recvmsg(200, handlemsg_master, NULL, NULL);
		end_time = usc_gettime();
		printf("total time=%f seconds \n", usc_difftime(end_time,start_time)/1000000);
		printf("enter a string:\n");
    }
    
    netd_sendmsg(1, END, 200);
    netd_recvmsg(200, handlemsg_master, NULL, NULL);
}

void worker()	
{
    int done=0;
    int my_id, fromto[2], size;
    
    my_id = netd_getMyID();
    size = netd_getSize();
    if (my_id == (size-1))
        fromto[1] = 0;
    else
		fromto[1] = my_id + 1;

	fromto[0]=my_id -1;

    while (done!=DONE)
    {
		done=netd_recvmsg(200, handlemsg_worker, (void **)fromto, NULL);
    }
}

