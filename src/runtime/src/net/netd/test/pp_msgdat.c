/**
 * This program tests the overhead
 * associated with the netd/netq header
 * in the current TMPI implementation.
 * Each send/receive of the pingpong program
 * works as follows:
 * 1) 4 bytes message header length (always 72)
 * 2) 72 bytes message header that contains the data length.
 * 3) variable length message data.
 */

#include <stdio.h>
#include <stdlib.h>
#include "../netd.h"
#include "../netd_comm.h"
#include "usc_time.h"
#include "tmpi_debug.h"
    
# define FALSE 0
# define TRUE 1

# define MSG_HEADER_LEN 72

void master();
void worker();

# define KILO 0x400
# define MEGA 0x100000
# define ITER 1000
# define ITER2 100
# define ITER3 20

static char *data;

int main(int argc, char *argv[])
{
	Machine machines[]={
		{"local", 1},
		{"node40", 1},
		/**
		**/
		{"nothing", -1}
	};

	data=(char *)malloc(MEGA);
	if (data) {
		memset(data, 0x55, MEGA);
	}
	else {
		tmpi_error(DBG_INFO, "Cannot allocate memory.");
		exit(-1);
	}

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

int short_size[][18]={
	{4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, -1},
	{72, 80, 88, 96, 104, 112, 120, 128, -1},
	{144, 160, 176, 192, 208, 224, 240, 256, -1},
	{288, 320, 352, 384, 416, 448, 480, 512, -1},
	{576, 640, 704, 768, 832, 896, 960, 1024, -1}
};

int handlemsg_master(int from, char *msg, int len, void **result)
{
	int size=*(int *)msg;

	if (size>0) {
		netd_recvdat(from, data, size);
	}

	return 0;
}

int handlemsg_worker(int from, char *msg, int len, void **result)
{
	int size=*(int *)msg;
    int msg2[MSG_HEADER_LEN/sizeof(int)];

	if (size>0) {
		netd_recvdat(from, data, size);
	}

	msg2[0]=size;

	netd_sendmsgdat(from, (char *)msg2, MSG_HEADER_LEN, data, size);

	return 0;
}

void master()
{
    int msg[MSG_HEADER_LEN/sizeof(int)];
    int size;
    usc_tmr_t timer;
	int i, j, k;
    
    
	usc_init();

	/* short */
	for (i=0; i<5; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;
			msg[0]=size;
			usc_tmr_reset(&timer);
			usc_tmr_start(&timer);
			for (k=0; k<ITER; k++) {
				if (netd_sendmsgdat(1, (char *)msg, MSG_HEADER_LEN, data, size)<0)
					tmpi_error(DBG_INFO, "sendmsg failed");
				if (netd_recvmsg(1024, handlemsg_master, NULL, NULL)<0)
					tmpi_error(DBG_INFO, "recvmsg failed");
			}
			usc_tmr_stop(&timer);
			printf("Ping Pong (short), size=%d, iteration=%d, single round trip=%fus, rate=%fMB.\n", 
					size, ITER, usc_tmr_read(&timer)/ITER, (float)size*2/(usc_tmr_read(&timer)/ITER));
		}
	}

	/* long */

	for (i=0; i<2; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;
			size*=KILO;
			msg[0]=size;
			usc_tmr_reset(&timer);
			usc_tmr_start(&timer);
			for (k=0; k<ITER2; k++) {
				if (netd_sendmsgdat(1, (char *)msg, MSG_HEADER_LEN, data, size)<0)
					tmpi_error(DBG_INFO, "sendmsg failed");
				if (netd_recvmsg(1024, handlemsg_master, NULL, NULL)<0)
					tmpi_error(DBG_INFO, "recvmsg failed");
			}
			usc_tmr_stop(&timer);
			printf("Ping Pong (long), size=%d, iteration=%d, transfer rate=%fMB/s.\n", 
					size, ITER2, (float)size*2/(usc_tmr_read(&timer)/ITER2));
		}
	}

	for (i=2; i<5; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;

			size*=KILO;
			msg[0]=size;
			usc_tmr_reset(&timer);
			usc_tmr_start(&timer);
			for (k=0; k<ITER3; k++) {
				if (netd_sendmsgdat(1, (char *)msg, MSG_HEADER_LEN, data, size)<0)
					tmpi_error(DBG_INFO, "sendmsg failed");
				if (netd_recvmsg(1024, handlemsg_master, NULL, NULL)<0)
					tmpi_error(DBG_INFO, "recvmsg failed");
			}
			usc_tmr_stop(&timer);
			printf("Ping Pong (very long), size=%d, iteration=%d, transfer rate=%fMB/s.\n", 
					size, ITER3, (float)size*2/(usc_tmr_read(&timer)/ITER3));
		}
	}
    printf("master exiting normally\n");
}

void worker()	
{
    int size;
	int i, j, k;
    
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

	/* long */

	for (i=0; i<2; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;

			for (k=0; k<ITER2; k++) {
				netd_recvmsg(1024, handlemsg_worker, NULL, NULL);
			}
		}
	}

	for (i=2; i<5; i++) {
		for (j=0; j<18; j++) {
			if ((size=short_size[i][j])<0)
				break;

			for (k=0; k<ITER3; k++) {
				netd_recvmsg(1024, handlemsg_worker, NULL, NULL);
			}
		}
	}

}
