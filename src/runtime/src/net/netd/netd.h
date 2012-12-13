# ifndef _NETD_H_
# define _NETD_H_

/**
 * Note, the hard limit for the maximum nodes is
 * about 2^24, which is due to the message type
 * space in the underlying P4 device. (I need
 * a different message type for broadcast calls
 * from a specific root.)
 */
# define NETD_MAX_HOST_NAME 255
# define NETD_MAX_HOST_NUM 256
# define NETD_MAX_NODE_NUM 1024

# define NETD_DBGOPT "-tmpidbg"

/* global id to local id */
typedef struct machine {
	char szHost[NETD_MAX_HOST_NAME+1]; /* host name */
	int nProcs; /* how many instances run on that machine? alwasy 1 for TMPI */
} Machine; 

/** 
 * spawn new processes on all remote machines
 * and set up a fully connected network with them.
 * The remote process will be created using ssh
 * and passed the same argument as what netd_initEnv
 * received.
 */
int netd_start(Machine *mac_list);

int netd_getMyID();

int netd_getSize();

int netd_amIStartupMaster();

/** 
 * parse command line arguments and return a machine list
 * If pmac_list is NULL, the upper layer will tell me
 * the machine list so I will just ignore the command line
 * options that deal with machine list.
 * All upper layer related options need to be processed before
 * calling netd_initEnv(). It will remember the argc and argv
 * after the adjustion it might make. Need to be called 
 * before all other netd functions can be called.
 * Machine list will be terminated with an entry with nProc=-1.
 */
int netd_initEnv(int *argc, char **(*argv), Machine **pmac_list);

void netd_waitAll();

char *netd_getProgPathName();

# endif
