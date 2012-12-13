#include <stdio.h>
#include <stdlib.h>
#include "../netd.h"
#include "../netd_comm.h"
#include "usc_time.h"
#include "tmpi_debug.h"
    
# define FALSE 0
# define TRUE 1

void master();
void worker();

int main(int argc, char *argv[])
{
	Machine machines[]={
		{"local", 1},
		{"node40", 1},
		/**
		**/
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

# define KILO 0x400
# define MEGA 0x100000
# define ITER 1000
# define ITER2 100

int short_size[][18]={
	{4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, -1},
	{72, 80, 88, 96, 104, 112, 120, 128, -1},
	{144, 160, 176, 192, 208, 224, 240, 256, -1},
	{288, 320, 352, 384, 416, 448, 480, 512, -1},
	{576, 640, 704, 768, 832, 896, 960, 1024, -1}
};

int handlemsg_master(int from, char *msg, int len, void **result)
{
	return 0;
}

int handlemsg_worker(int from, char *msg, int len, void **result)
{
	netd_sendmsg(from, msg, len);

	return 0;
}

void master()
{
    char *msg;
    int size;
    usc_tmr_t timer;
	int i, j, k;
    
    
	msg=(char *)malloc(MEGA);
	memset(msg, 0x55, MEGA);

	usc_init();
	/* short */
	for (i=0; i<5; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;
			usc_tmr_reset(&timer);
			usc_tmr_start(&timer);
			for (k=0; k<ITER; k++) {
				if (netd_sendmsg(1, msg, size)<0)
					tmpi_error(DBG_INFO, "sendmsg failed");
				if (netd_recvmsg(1024, handlemsg_master, NULL, NULL)<0)
					tmpi_error(DBG_INFO, "recvmsg failed");
			}
			usc_tmr_stop(&timer);
			printf("Ping Pong (short), size=%d, iteration=%d, single round trip=%fus.\n", 
					size, ITER, usc_tmr_read(&timer)/ITER);
		}
	}

    printf("master exiting normally\n");
}

void worker()	
{
    char *msg;
    int size;
	int i, j, k;
    
    
	msg=(char *)malloc(MEGA);
	memset(msg, 0x55, MEGA);

	/* short */
	for (i=0; i<5; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;
			for (k=0; k<ITER; k++) {
				netd_recvmsg(1024, handlemsg_worker, NULL, NULL);
			}
		}
	}
}
