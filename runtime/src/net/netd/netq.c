# include <stdio.h>
# include <stdlib.h>
# include <ctype.h>
# include <string.h>
# include <unistd.h>
# include "thread.h"
# include "fifo.h"
# include "stackallocator.h"
# include "netq.h"
# include "netq_comm.h"
# include "netd.h"
# include "netd_comm.h"
# include "tmpi_debug.h"
# include "atomic.h"

/** 
 * Important: not all routines are multithread safe.
 * 		initEnv, start, end, can only be called
 * 		sequentially.
 */


# define MAX_MSG_SIZE 1024

# define MAX_FILENAME_LENGTH 1023

typedef struct netq_control {
	int type;
	int payload;
} NETQ_CONTROL;

# define NETQ_PEER_DOWN 1

typedef struct netq_info {
	/* frequent read, seldom modify */
	/* global rank to local rank translation */
	ID_ENTRY * gTable;
	int gTableSize;
	/* local rank to global rank translation */
	int * lTable;
	int lTableSize;
	FIFO *outq;
	NETQ_HANDLE nh;
	int stat;
	int shutdown;
	int movin_down;
	int movout_down;
	StackAllocator *qalloc;
	char *lsend_modes;
	char *peer_stat;
	/* seldom read */
	netq_msg_handler mh;
	thr_t movin;
	thr_t movout;
	char grpFileName[MAX_FILENAME_LENGTH+1];
	/* frequent modify */
	int isSingleMode;
	/* seldom read */
	char resFileName[MAX_FILENAME_LENGTH+1];
	/* frequent modify */
	thr_mtx_t **send_mtx;
} NETQ_INFO;	
	
static NETQ_INFO netq_info =
{
	NULL,  	/* gtable */
	-1,		/* gTableSize */ 
	NULL,	/* lTable */ 
	-1,		/* lTableSize */ 
	NULL,	/* moveout queue */
	{
		netq_fillFromAddr,
		netq_fillToAddr,
		netq_fillHeaderSize,
		netq_fillPayloadSize,
		netq_getFromAddr,
		netq_getToAddr,
		netq_getHeaderSize,
		netq_getPayloadSize
	},		
	NETQ_OFF, /* stat */
	0,		/* shutting down flag */
	0, 		/* movin down */
	0,		/* movout down */
	NULL,  /* stack allocator */
	NULL,  /* lsend_modes */
	NULL, /* peer states */
	NULL, /* netq message handler */
	NULL,	/* movin thread */
	NULL,	/* moveout thread */
	"",
	0,   /* is single mode */
	"",
	NULL
};

volatile static int movin_up=0;
volatile static int movout_up=0;

typedef struct sendreq {
	int from;
	int to;
	char *hdr;
	int hdrlen;
	char *payload;
	int pldlen;
} SENDREQ;

int netq_status()
{
	return netq_info.stat;
}

static void netq_dbg_pstat(int dbgl)
{
	switch (netq_info.stat) {
		case NETQ_OFF : 
			tmpi_error(dbgl, "netq is not started.");
			break;
		case NETQ_INIT : 
			tmpi_error(dbgl, "netq is being started.");
			break;
		case NETQ_ON : 
			tmpi_error(dbgl, "netq is already started.");
			break;
		case NETQ_END : 
			tmpi_error(dbgl, "netq is being shutting down.");
			break;
		case NETQ_DOWN : 
			tmpi_error(dbgl, "netq is already down.");
			break;
		case NETQ_ERROR_START : 
			tmpi_error(dbgl, "netq is cannot be started due to errors.");
			break;
		case NETQ_ERROR_RUN : 
			tmpi_error(dbgl, "netq is stopped due to runtime errors.");
			break;
		default: 
			tmpi_error(DBG_INTERNAL, "netq state code unknown.");
	}
	return;
}

/* once called, no functions can be called **/
int netq_end()
{
	SENDREQ *req;
	int i;

	/** first statment **/
	if (netq_info.stat!=NETQ_ON) {
		netq_dbg_pstat(DBG_CALLER);
		return -1;
	}
	netq_info.stat=NETQ_END;
	commit32(&netq_info.stat);

	/* real code here */
	
	netq_info.shutdown=1;
	commit32(&netq_info.shutdown);

	if (!netq_info.isSingleMode) {
		req=(SENDREQ *)sa_alloc(netq_info.qalloc);
		req->from=req->to=-1;
		fifo_enqueue(netq_info.outq, req);
	}
	else {
		int i;
		int myid=netd_getMyID();
		int size=netd_getSize();
		NETQ_CONTROL ctl;

		ctl.type=NETQ_PEER_DOWN;
		ctl.payload=0;

		for (i=0; i<size; i++) {
			if (i==myid)
				continue;
			thr_mtx_lock((netq_info.send_mtx[i]));
			netd_sendmsg(i, (char *)&ctl, sizeof(NETQ_CONTROL));
			thr_mtx_unlock((netq_info.send_mtx[i]));
		}
	}

	thr_joinone(netq_info.movin, NULL);

	if (!netq_info.isSingleMode) {
		thr_joinone(netq_info.movout, NULL);
	}
	
	netd_waitAll();

	netq_info.lTableSize=netq_info.gTableSize-1;
	
	if (netq_info.gTable) {
		free(netq_info.gTable);
		netq_info.gTable=NULL;
	}
	if (netq_info.lTable) {
		free(netq_info.lTable);
		netq_info.lTable=NULL;
	}
	if (netq_info.outq) {
		fifo_destroy(netq_info.outq);
		netq_info.outq=NULL;
	}
	if (netq_info.peer_stat) {
		free(netq_info.peer_stat);
		netq_info.peer_stat=NULL;
	}
	if (netq_info.qalloc) {
		sa_destroy(netq_info.qalloc);
		free(netq_info.qalloc);
		netq_info.qalloc=NULL;
	}
	if (netq_info.lsend_modes) {
		free(netq_info.lsend_modes);
		netq_info.lsend_modes=NULL;
	}

	if (netq_info.send_mtx) {
		for (i=0; i<netd_getSize(); i++) {
			thr_mtx_destroy((netq_info.send_mtx[i]));
			free(netq_info.send_mtx[i]);
		}
		free(netq_info.send_mtx);
		netq_info.send_mtx=NULL;
	}

	/* last statement before sucessful start */
	netq_info.stat=NETQ_DOWN;
	commit32(&netq_info.stat);

	return 0;
}

/* NULL terminated list */
static void free_mac_list(MAC_ENTRY **mac_list)
{
	if (!mac_list)
		return;

	while (*mac_list) {
		free(*mac_list);
		mac_list++;
	}

	free(mac_list);
	return;
}

/** 
 * can only be called once before all other netq functions are called 
 * setting up connections and exchange ID translation table 
 */

int netq_amIStartupMaster()
{
	return netd_amIStartupMaster();
}

static int check_for_termination()
{
	int i;
	int size=netd_getSize();
	int myid=netd_getMyID();

	for (i=0; i<size; i++) {
		if (myid==i)
			continue;
		if (netq_info.peer_stat[i]==0)
			return 0;
	}

	return 1;
}

static int movin_msg_handle(int real_from, char *msg, int len, void **result)
{
	int from, to, hdrlen, pldlen;
	netq_msg_handler mh=netq_info.mh;
	int ret;

	if (len<12) {
		/* it is a control message */
		NETQ_CONTROL *ctl=(NETQ_CONTROL *)msg;
		switch(ctl->type) {
			case NETQ_PEER_DOWN : 
				netq_info.peer_stat[real_from]=1;
				return check_for_termination();
			default : 
				tmpi_error(DBG_INFO, "Unknown control message received");
				return 0;
		}
	}
	else {
		from=(*netq_info.nh.getFromAddr)(msg, len);
		to=(*netq_info.nh.getToAddr)(msg, len);
		hdrlen=(*netq_info.nh.getHeaderSize)(msg, len);
		pldlen=(*netq_info.nh.getPayloadSize)(msg, len);
		ret=(*mh)(from, to, msg, hdrlen, pldlen);
	
		if (result) {
			*result=(void *)ret;
			return 0;
		}

		return ret;
	}
}

static void *movin(void *dump)
{
	// initialize
	int uret, ret;
	int retry=1000;
	
	// initialize done
	movin_up=1;
	commit32(&movin_up);

	// looping aroud
	while (1) {
		if (netq_info.shutdown) {
			netq_info.movin_down=1;
			commit32(&netq_info.movin_down);
			return NULL;
		}
		/* real call back return code in uret and msg_handle return code in ret */
		if (netd_recvmsg(MAX_MSG_SIZE, movin_msg_handle, (void **)&uret, &ret)) {
			retry--;
			if (retry<0)
				return NULL;
			usleep(1);
			continue;
		}	
		if (ret==1) { /* shutdown */
			netq_info.movin_down=1;
			commit32(&netq_info.movin_down);
			return NULL;
		}
	}
}

static int netq_real_send(SENDREQ *req);

static void *movout(void *dump)
{
	SENDREQ *req;
	FIFO *outq=netq_info.outq;
	StackAllocator *sa=netq_info.qalloc;
	int retry=1000;
	// int isSingleMode=netq_info.isSingleMode;

	// initialize
	// Oops, nothing to be initialized.
	
	// initialize done
	movout_up=1;
	commit32(&movout_up);
	
	// looping aroud
	while (1) {
		req=fifo_dequeue(outq);
		if (req) {
			if (req->from==-1) {
				/* the terminator */
				NETQ_CONTROL ctl;
				int i, size, myid;
				// ID_ENTRY id_entry;
				// int idx;

				sa_free(sa, req);
				netq_info.movout_down=1;
				commit32(&netq_info.movout_down);

				// tell others that I am off duty now.
				ctl.type=NETQ_PEER_DOWN;
				ctl.payload=0;
				size=netd_getSize();
				myid=netd_getMyID();

				for (i=0; i<size; i++) {
					if (i==myid)
						continue;
					thr_mtx_lock((netq_info.send_mtx[i]));
					netd_sendmsg(i, (char *)&ctl, sizeof(NETQ_CONTROL));
					thr_mtx_unlock((netq_info.send_mtx[i]));
				}

				return NULL;
			}
			// if (!isSingleMode) {
				// thr_mtx_lock(&netq_info.send_mtx);
			// }

			netq_real_send(req);

			// if (!isSingleMode) {
				// thr_mtx_unlock(&netq_info.send_mtx);
			// }

			// netq_transGlobalRank(req->from, &id_entry);
			// idx=id_entry.localID*netq_info.gTableSize+req->to;
			// dec32(&(netq_info.lsend_mode[idx]));
			
			// free req->header and req->payload first.
			// to do ...
			
			sa_free(sa, req);
		}
		else {
			retry--;
			if (retry<0) {
				netq_info.movout_down=1;
				commit32(&netq_info.movout_down);
				return NULL;
			}
			usleep(1);
		}
	}
}

int netq_start(NETQ_HANDLE *nh, MAC_ENTRY **mac_list, PROG_RES *res, PROG_TOPO *topo, netq_msg_handler mh)
{
	/**
	 * should be called after netq_initEnv().
	 */

	int nMachine;
	int i, j;
	int freeMacList=1;
	int total=0;
	int my_mac_id;
	int lock_size;

	if (netd_getMyID()<0) {
		tmpi_error(DBG_CALLER, "You should call netq_initEnv before netq_start.");
		return -1;
	}

	if (netq_info.stat!=NETQ_OFF) {
		netq_dbg_pstat(DBG_CALLER);
		return -1;
	}

	netq_info.stat=NETQ_INIT;
	commit32(&netq_info.stat); /* flush memory */

	/**
	 * If I am startup master:
	 * 		If mac_list is not NULL, use mac_list to distribute the nodes.
	 * 		Otherwise, if res is not NULL, trying to allocat resource
	 * 		based on res.
	 * 		Otherwise, check if a group file is available of the name
	 * 		netq_info.grpFileName. 
	 * 		Otherwise, check if a resource file is avialble of the name 
	 * 		netq_info.resFileName.
	 * 		Otherwise, check if a default group file is available of the
	 * 		name <application>.grp.
	 * 		Otherwise, check if a default resource file is available of
	 * 		the name <application>.res. 
	 * 		Otherwise, set netq stat to be NETQ_ERROR and quit.
	 *
	 * 		Parse file if necessary and get a machine list
	 * 		netd_start (machine list);
	 * else
	 * 		netd_start (NULL);
	 * end if
	 *
	 * broadcast id mapping information from node 0.
	 *
	 * Initialize netq_info structure.
	 *
	 * startup movin/movout. sync with them.
	 *
	 * return.
	 */
	
	if (netd_amIStartupMaster()) {
		MAC_ENTRY **item;
		Machine *netd_macs;

		while (1) {
			int contTry=0; /* keep trying if failed? */

			/* make it more like a switch */

			/* use machine list */
			if (mac_list)  {
				/* user supplied machine list, don't free it */
				freeMacList=0;
				break;
			}

			/* try to dynamically query nodes */
			if (res && topo) {
				mac_list=netq_queryNodes(res, topo);
				if (!mac_list) {
					tmpi_error(DBG_FATAL, "Resource management service unavailable.");
					netq_info.stat=NETQ_ERROR_START;
					return -1;
				}
				break;
			}

			/**
			 * try to use group file only when:
			 * 1) user supplies one;
			 * 2) no user supplied group file or user supplied resource 
			 *    is available. (look for file with the default name)
			 */
			if ( (netq_info.grpFileName[0]) || (netq_info.resFileName[0]=='\0')) {
				contTry=0; /* keep trying if failed? */
				if (netq_info.grpFileName[0]=='\0') { 
					/* try to use default group file */
					char *fullpath=netd_getProgPathName();

					contTry=1;
					if ( (strlen(fullpath)+4)<MAX_FILENAME_LENGTH) {
						sprintf(netq_info.grpFileName, "%s.grp", fullpath);
					}
				}

				if (netq_info.grpFileName[0]) {
					mac_list=netq_parseGrpFile(netq_info.grpFileName);
					if (!mac_list) {
						if (!contTry) {
							tmpi_error(DBG_FATAL, "Cannot read group configuration file %s.", 
								netq_info.grpFileName);
							netq_info.stat=NETQ_ERROR_START;
							return -1;
						}
					}
					else
						break;
				}
			}

			contTry=0;
			if (netq_info.resFileName[0]=='\0') {
				char *fullpath=netd_getProgPathName();

				contTry=1;
				if ( (strlen(fullpath)+4)<MAX_FILENAME_LENGTH) {
					sprintf(netq_info.resFileName, "%s.res", fullpath);
				}
			}
			
			if (netq_info.resFileName[0]) {
				PROG_RES res0;
				PROG_TOPO topo0;
				if (netq_parseResFile(netq_info.resFileName, &res0, &topo0)) {
					if (!contTry) {
						tmpi_error(DBG_FATAL, "Cannot read resource configuration file %s", 
							netq_info.resFileName);
					}
					netq_info.stat=NETQ_ERROR_START;
					return -1;
				}
				mac_list=netq_queryNodes(&res0, &topo0);
				if (!mac_list) {
					tmpi_error(DBG_FATAL, "Resource management service unavailable.");
					netq_info.stat=NETQ_ERROR_START;
					return -1;
				}

				break;
			}

			tmpi_error(DBG_FATAL, "Cannot get resource information to start the program.");
			netq_info.stat=NETQ_ERROR_START;
			return -1;
		}

		/* use mac_list to start netd */
		item=mac_list;
		while (*item)
			item++;

		nMachine=item-mac_list;

		netd_macs=(Machine *)calloc(nMachine+1, sizeof(Machine));

		if (netd_macs==NULL) {
			if (freeMacList)
				free_mac_list(mac_list);
			tmpi_error(DBG_FATAL, "Memory allocation failed.");
			netq_info.stat=NETQ_ERROR_START;
			return -1;
		}

		for (i=0; i<nMachine; i++) {
			strcpy(netd_macs[i].szHost, mac_list[i]->szHost);
			netd_macs[i].nProcs=1;
		}

		netd_macs[i].szHost[0]='\0';
		netd_macs[i].nProcs=-1;

		if (netd_start(netd_macs)) {
			if (freeMacList)
				free_mac_list(mac_list);
			free(netd_macs);
			tmpi_error(DBG_FATAL, "Cannot start NETD service.");
			netq_info.stat=NETQ_ERROR_START;
			return -1;
		}

		my_mac_id=0;

		/* exchange id mapping information */
		netd_bcast((char *)&nMachine, sizeof(int), 0);
	}
	else {
		if (netd_start(NULL)) {
			tmpi_error(DBG_FATAL, "Cannot start NETD service.");
			netq_info.stat=NETQ_ERROR_START;
			return -1;
		}
		my_mac_id=netd_getMyID();
		/* exchange id mapping information */
		netd_bcast((char *)&nMachine, sizeof(int), 0);
		mac_list=(MAC_ENTRY **)malloc(sizeof(MAC_ENTRY *)*nMachine);
		for (i=0; i<nMachine; i++)
			mac_list[i]=(MAC_ENTRY *)malloc(sizeof(MAC_ENTRY));
	}

	for (i=0; i<nMachine; i++) {
		netd_bcast((char *)mac_list[i], sizeof(MAC_ENTRY), 0);
		total+=mac_list[i]->nNodes;
	}

	/* initialize netq_info */

	netq_info.gTable=(ID_ENTRY *)calloc(total, sizeof(ID_ENTRY));
	netq_info.lTable=(int *)calloc(mac_list[my_mac_id]->nNodes, sizeof(int));
	netq_info.outq=fifo_create(FIFO_LEN_DEFAULT);
	netq_info.peer_stat=(char *)calloc(netd_getSize(), sizeof(char));
	netq_info.qalloc=(StackAllocator *)calloc(1, sizeof(StackAllocator));
	netq_info.lsend_modes=(char *)calloc(mac_list[my_mac_id]->nNodes*total, sizeof(char));

	if ((!netq_info.gTable) || (!netq_info.lTable) || (!netq_info.outq) || 
			(!netq_info.peer_stat) || (!netq_info.qalloc) || (!netq_info.lsend_modes) ) {
		if (netq_info.gTable) {
			free(netq_info.gTable);
			netq_info.gTable=NULL;
		}
		if (netq_info.lTable) {
			free(netq_info.lTable);
			netq_info.lTable=NULL;
		}
		if (netq_info.outq) {
			fifo_destroy(netq_info.outq);
			netq_info.outq=NULL;
		}
		if (netq_info.peer_stat) {
			free(netq_info.peer_stat);
			netq_info.peer_stat=NULL;
		}
		if (netq_info.qalloc) {
			free(netq_info.qalloc);
			netq_info.qalloc=NULL;
		}
		if (netq_info.lsend_modes) {
			free(netq_info.lsend_modes);
			netq_info.lsend_modes=NULL;
		}
		tmpi_error(DBG_FATAL, "Cannot allocate memory.");
		return -1;
	}

	sa_init(netq_info.qalloc, sizeof(SENDREQ), 65536);
	netq_info.send_mtx=(thr_mtx_t **)calloc(netd_getSize(), sizeof(thr_mtx_t *));
	lock_size=(sizeof(thr_mtx_t)+CACHE_LINE-1)/CACHE_LINE*CACHE_LINE;
	for (i=0; i<netd_getSize(); i++) {
		netq_info.send_mtx[i]=(thr_mtx_t *)calloc(1, lock_size);
		thr_mtx_init((netq_info.send_mtx[i]));
	}

	netq_info.gTableSize=total;
	netq_info.lTableSize=mac_list[my_mac_id]->nNodes;

	// Always single mode
	// if (netq_info.lTableSize==1)
		netq_info.isSingleMode=1;

	if (nh) {
		memcpy((char *)&netq_info.nh, nh, sizeof(NETQ_HANDLE));
	}

	netq_info.mh=mh;

	for (i=0; i<nMachine; i++) {
		if (my_mac_id==i) {
			memcpy((char *)netq_info.lTable, (char *)mac_list[i]->nodeIDs, 
					sizeof(int)*mac_list[i]->nNodes);
		}

		for (j=0; j<mac_list[i]->nNodes; j++) {
			ID_ENTRY *id_entry=&(netq_info.gTable[mac_list[i]->nodeIDs[j]]);
			id_entry->macID=i;
			id_entry->localID=j;
		}
	}
	
	if (freeMacList)
		free(mac_list);

	/* create movin/movout */
	netq_info.movin=thr_createone(movin, NULL);
	
	if (!netq_info.isSingleMode)
		netq_info.movout=thr_createone(movout, NULL);
	else 
		movout_up=1;

	while ((!movin_up) || (!movout_up))
		usleep(1);

	/* last statement before sucessful start */
	netq_info.stat=NETQ_ON;
	commit32(&netq_info.stat);

	return 0;
}

static void netq_strip_out_args(char **argv, int *argc, int *c, int num)
{
    char **a;
    int i;

    /* Strip out the argument. */
    for (a = argv, i = (*c); i <= *argc-num; i++, a++)
		*a = (*(a + num));
	*a=NULL;
    (*argc) -= num;
}

int netq_initEnv(int *argc, char ***argv)
{
	int ret;
	int i;

	if ( (ret=netd_initEnv(argc, argv, NULL)) ) {
		tmpi_error(DBG_INTERNAL, "NETD initEnv failed.");
		return ret;
	}

	/**
	 * strip off netq specific flags
	 *
	 */

	for (i=*argc; i>0; i--) { 
		if (strcmp((*argv)[i-1], "-netqgrp")==0) {
			strncpy(netq_info.grpFileName, (*argv)[i], MAX_FILENAME_LENGTH);
			netq_strip_out_args((*argv)+(i-1), argc, &i, 2);
			tmpi_error(DBG_INFO, "Get group file name = %s.", netq_info.grpFileName); 
		}
		else if (strcmp((*argv)[i-1], "-netqres")==0) {
			strncpy(netq_info.resFileName, (*argv)[i], MAX_FILENAME_LENGTH);
			netq_strip_out_args((*argv)+(i-1), argc, &i, 2);
			tmpi_error(DBG_INFO, "Get resource file name = %s.", netq_info.resFileName); 
		}
	}

	return 0;
}

MAC_ENTRY ** netq_queryNodes(PROG_RES *res, PROG_TOPO *topo)
{
	tmpi_error(DBG_FATAL, "netq_queryNodes not implemented");
	return NULL;
}

static char *read_one_line(FILE *input)
{
	static char line[1024];
	int len;

	if (!input)
		return NULL;

	if (!fgets(line, 1024, input))
		return NULL;

	len=strlen(line);
	if ( (len>0) && (line[len-1]=='\n')) 
		line[len-1]='\0';

	return line;
}

# define COMMENT_LEAD '#'
static void strip_off_comment(char *line)
{
	char *idx;

	if (!line)
		return;

	if ((idx=strchr(line, COMMENT_LEAD))!=NULL)
		*idx='\0';
	return;
}

static int is_blank_line(char *line)
{
	/* treat NULL line as blank */
	if (!line)
		return 1;

	for (; *line; line++) {
		if (!isspace(*line))
			return 0;
	}

	return 1;
}

static int is_integer(char *token)
{
	if (!token)
		return 0;

	if (strlen(token)==0)
		return 0;

	for (; *token; token++)
		if (!isdigit(*token))
			return 0;

	return 1;
}

static int is_range(char *token)
{
	int nDigit=0, nDash=0;

	if (!token)
		return 0;

	if (strlen(token)==0)
		return 0;

	for (; *token; token++) {
		if (!isdigit(*token) && (*token != '-') )
			return 0;
		if (*token == '-') {
			nDash++;
			if (nDigit==0)
				return 0;
			nDigit=0;
		}
		else 
			nDigit++;
	}
	if ( (nDash != 1) || (nDigit == 0 ) )
		return 0;

	return 1;
}

static MAC_ENTRY *parse_one_line(char *line)
{
	char *hostname;
	char *sNodes;
	char *token;
	MAC_ENTRY *mac=(MAC_ENTRY *)calloc(1, sizeof(MAC_ENTRY));
	int i, j;

	if (mac==NULL)
		return NULL;

	hostname=strtok(line, "\t ");
	sNodes=strtok(NULL, "\t ");
	if (hostname==NULL || sNodes==NULL) {
		free(mac);
		return NULL;
	}

	if (!is_integer(sNodes)) {
		free(mac);
		return NULL;
	}

	strncpy(mac->szHost, hostname, NETQ_MAX_HOST_NAME);
	mac->nNodes=atoi(sNodes);
	if (mac->nNodes<0) {
		free(mac);
		return NULL;
	}

	for (i=0; i<NETQ_MAX_NODES_PER_HOST; i++)
		mac->nodeIDs[i]=-1;

	i=0;
	while ((token=strtok(NULL, "\t "))) {
		int from, to;
		if (strchr(token, '-')) {
			if (!is_range(token)) {
				free(mac);
				return NULL;
			}
			sscanf(token, "%d-%d", &from, &to);
			if (from>to) {
				int tmp=from;
				from=to;
				to=tmp;
			}

			if ((to-from+i)>=mac->nNodes) {
				free(mac);
				return NULL;
			}

			for (j=from; j<=to; j++)
				mac->nodeIDs[i++]=j;
		}
		else if (!is_integer(token)) {
			free(mac);
			return NULL;
		}
		else {
			if (i>=mac->nNodes) {
				free(mac);
				return NULL;
			}
			mac->nodeIDs[i++]=atoi(token);
		}
	}

	return mac;
}

static void dumpMachineList(MAC_ENTRY **mac_list)
{
	if (!mac_list)
		return;

	while (*mac_list) {
		int i;
		char list[1024]="(";

		for (i=0; i<(*mac_list)->nNodes-1; i++)
			sprintf(list+strlen(list), "%d, ",(*mac_list)->nodeIDs[i]);
		sprintf(list+strlen(list), "%d)", (*mac_list)->nodeIDs[i]);
		tmpi_error(DBG_INFO, "%s:%d\t%s", (*mac_list)->szHost, (*mac_list)->nNodes, list);
		mac_list++;
	}
}

/** fill in the resource structure, return the machine group */
MAC_ENTRY ** netq_parseGrpFile(char *filename)
{
	FILE *input=fopen(filename, "r");
	char *line;
	MAC_ENTRY *macs[NETQ_MAX_NODES];
	int idx=0;
	int total=0;
	char *used;
	MAC_ENTRY **mac_list;
	int i, j, k;

	if (!input)
		return NULL;

	while ((line=read_one_line(input))) {
		strip_off_comment(line);
		if (is_blank_line(line))
			continue;
		macs[idx]=parse_one_line(line);

		if (macs[idx]==NULL) {
			for (i=0; i<idx; i++)
				free(macs[i]);
			fclose(input);
			return NULL;
		}

		total+=macs[idx]->nNodes;
		idx++;
	}

	fclose(input);

	used=(char *)calloc(total, sizeof(char));

	/* initialized with 0 already */
	mac_list=(MAC_ENTRY **)calloc(idx+1, sizeof(MAC_ENTRY *));

	for (i=0; i<idx; i++) {
		mac_list[i]=macs[i];
		for (j=0; j<macs[i]->nNodes; j++) {
			int id=macs[i]->nodeIDs[j];
			if ( (id>=total) || ( (id!=-1) && (used[id]) ) ) {
				for (k=0; k<idx; k++)
					free(macs[k]);
				free(mac_list);
				free(used);
				return NULL;
			}

			if (id != -1)
				used[id]=1;
			else
				break;
		}
	}

	k=0;

	for (i=0; i<idx; i++) {
		for (j=0; j<macs[i]->nNodes; j++) {
			int id=macs[i]->nodeIDs[j];
			if (id==-1) {
				while (used[k])
					k++;
				macs[i]->nodeIDs[j]=k;
				used[k]=1;
			}
		}
	}

	free(used);

	// dumpMachineList(mac_list);

	return mac_list;
}

/** fill in the resource/topology structure */
int netq_parseResFile(char *filename, PROG_RES *res, PROG_TOPO *topo)
{
	tmpi_error(DBG_FATAL, "netq_parseResFile not implemented");
	return -1;
}

/* get a default resource spec, res has to be allocated space */
int netq_getDefaultResource(PROG_RES *res)
{
	res->nNodes=4;
	res->nExecTime=res->nNodes*4;
	res->nCPU=res->nNodes*2;
	res->nMemory=res->nNodes*4;
	res->nDisk=res->nNodes*1;
	res->nNetComm=res->nNodes*res->nNodes*1;
	res->cache=NULL;
	res->ignore_cache=0;

	return 0;
}

/* get a default topology spec, res has to be allocated space */
int netq_getDefaultTopology(PROG_TOPO *topo)
{
	tmpi_error(DBG_FATAL, "netq_getDefaultTopology not implemented");
	return -1;
}

int netq_getMacID(int grank, int *macid)
{
	if (netq_info.gTableSize==-1) {
		tmpi_error(DBG_CALLER, "netq_getMacID: netq translation table not yet initialized.");
		return -1;
	}

	if ( (grank < 0) || (grank >= netq_info.gTableSize) ) {
		tmpi_error(DBG_APP, "netq_getMacID: global rank out of range.");
		return -1;
	}

	if (netq_info.gTable) {
		*macid=netq_info.gTable[grank].macID;
	}
	else {
		tmpi_error(DBG_CALLER, "netq_getMacID: netq translation table not yet initialized.");
		return -1;
	}

	return 0;
}

int netq_transGlobalRank(int grank, ID_ENTRY *lrank)
{
	if (netq_info.gTableSize==-1) {
		tmpi_error(DBG_CALLER, "netq_transGlobalRank: netq translation table not yet initialized.");
		return -1;
	}

	if ( (grank < 0) || (grank >= netq_info.gTableSize) ) {
		tmpi_error(DBG_APP, "netq_transGlobalRank: global rank out of range.");
		return -1;
	}

	if (netq_info.gTable) {
		lrank->macID=netq_info.gTable[grank].macID;
		lrank->localID=netq_info.gTable[grank].localID;
	}
	else {
		tmpi_error(DBG_CALLER, "netq_transGlobalRank: netq translation table not yet initialized.");
		return -1;
	}

	return 0;
}

int netq_findGlobalRank(ID_ENTRY *id_entry, int *grank)
{
	int ret=-1;

	if ((!id_entry) || (!grank))
		return -1;

	if (netq_info.gTable) {
		int i;
		for (i=0; i<netq_info.gTableSize; i++) {
			if ((netq_info.gTable[i].macID==id_entry->macID) &&
				(netq_info.gTable[i].localID==id_entry->localID)) {
				*grank=i;
				ret=0;
				break;
			}
		}
	}
	return ret;
}

int netq_transLocalRank(int lrank, int *grank)
{
	if (netq_info.lTableSize==-1) {
		tmpi_error(DBG_CALLER, "netq_transLocalRank: netq translation table not yet initialized.");
		return -1;
	}

	if ( (lrank < 0) || (lrank >= netq_info.lTableSize) ) {
		tmpi_error(DBG_APP, "netq_transLocalRank: local rank out of range.");
		return -1;
	}

	if (netq_info.lTable) {
		*grank=netq_info.lTable[lrank];
	}
	else {
		tmpi_error(DBG_CALLER, "netq_transLocalRank: netq translation table not yet initialized.");
		return -1;
	}

	return 0;
}

int netq_getLocalMacID()
{
	return netd_getMyID();
}

int netq_isLocalNode(int grank)
{
	ID_ENTRY id_entry;

	if (netq_transGlobalRank(grank, &id_entry)<0)
		return 0;

	if (id_entry.macID==netq_getLocalMacID())
		return 1;

	return 0;
}

int netq_getLocalSize()
{
	return netq_info.lTableSize;
}

int netq_getGlobalSize()
{
	return netq_info.gTableSize;
}

int netq_getMacSize()
{
	return netd_getSize();
}

void netq_fillFromAddr(int from, char *header, int len)
{
	NETQ_HEADER *hdr=(NETQ_HEADER *)header;

	if (len<sizeof(NETQ_HEADER)) {
		tmpi_error(DBG_CALLER, "header length shorter than expected.");
		return;
	}

	hdr->from=from;
}

void netq_fillToAddr(int to, char *header, int len)
{
	NETQ_HEADER *hdr=(NETQ_HEADER *)header;

	if (len<sizeof(NETQ_HEADER)) {
		tmpi_error(DBG_CALLER, "header length shorter than expected.");
		return;
	}

	hdr->to=to;
}

void netq_fillHeaderSize(int hdrsize, char *header, int len)
{
	NETQ_HEADER *hdr=(NETQ_HEADER *)header;

	if (len<sizeof(NETQ_HEADER)) {
		tmpi_error(DBG_CALLER, "header length shorter than expected.");
		return;
	}

	hdr->hdrlen=hdrsize;
}

void netq_fillPayloadSize(int ploadsize, char *header, int len)
{
	NETQ_HEADER *hdr=(NETQ_HEADER *)header;

	if (len<sizeof(NETQ_HEADER)) {
		tmpi_error(DBG_CALLER, "header length shorter than expected.");
		return;
	}

	hdr->pldlen=ploadsize;
}

int netq_getFromAddr(char *header, int len)
{
	NETQ_HEADER *hdr=(NETQ_HEADER *)header;

	if (len<sizeof(NETQ_HEADER)) {
		tmpi_error(DBG_CALLER, "header length shorter than expected.");
		return -1;
	}

	return hdr->from;
}

int netq_getToAddr(char *header, int len)
{
	NETQ_HEADER *hdr=(NETQ_HEADER *)header;

	if (len<sizeof(NETQ_HEADER)) {
		tmpi_error(DBG_CALLER, "header length shorter than expected.");
		return -1;
	}

	return hdr->to;
}

int netq_getHeaderSize(char *header, int len)
{
	NETQ_HEADER *hdr=(NETQ_HEADER *)header;

	if (len<sizeof(NETQ_HEADER)) {
		tmpi_error(DBG_CALLER, "header length shorter than expected.");
		return -1;
	}

	return hdr->hdrlen;
}

int netq_getPayloadSize(char *header, int len)
{
	NETQ_HEADER *hdr=(NETQ_HEADER *)header;

	if (len<sizeof(NETQ_HEADER)) {
		tmpi_error(DBG_CALLER, "header length shorter than expected.");
		return -1;
	}

	return hdr->pldlen;
}

static int netq_real_send(SENDREQ *req)
{
	int real_to;
	ID_ENTRY id_entry;
	int ret;

	if (req->hdrlen<=0)
		return 0;

	netq_transGlobalRank(req->to, &id_entry);
	real_to=id_entry.macID;

	(*netq_info.nh.fillFromAddr)(req->from, req->hdr, req->hdrlen);
	(*netq_info.nh.fillToAddr)(req->to, req->hdr, req->hdrlen);
	(*netq_info.nh.fillHeaderSize)(req->hdrlen, req->hdr, req->hdrlen);
	(*netq_info.nh.fillPayloadSize)(req->pldlen, req->hdr, req->hdrlen);
	
	if (req->pldlen<=0) {
		thr_mtx_lock((netq_info.send_mtx[real_to]));
		ret=netd_sendmsg(real_to, req->hdr, req->hdrlen);
		thr_mtx_unlock((netq_info.send_mtx[real_to]));
	}
	else {
		thr_mtx_lock((netq_info.send_mtx[real_to]));
		ret=netd_sendmsgdat(real_to, req->hdr, req->hdrlen, req->payload, req->pldlen);
		thr_mtx_unlock((netq_info.send_mtx[real_to]));
	}

	return ret;
}

/**
 * successful return from netq_send doesn't mean that the 
 * message has been sent unless NETQ_BLK is specified.
 *
 * The reason is more related to memory management. The moveout
 * doesn't know how to deallocate the header and payload
 * memory.
 *
 * Another reason is that if one user sends two messages:
 * one non-block, the other block, do we need to force
 * block one follow the non-block one? I guess the anwser
 * is yes. So I need to figure out a way to deallocate
 * the space, e.g. by asking the netq_send API to provide
 * a distruction call back function if mode is NBLK.
 */
int netq_send(int from, int to, char *header, int headerlen, char *payload, int payloadlen, int mode)
{
	// ID_ENTRY id_entry;
	// int idx;
	SENDREQ *req=(SENDREQ *)sa_alloc(netq_info.qalloc);
	int ret=0;


	if (!req)
		return -1;

	req->from=from;
	req->to=to;
	req->hdr=header;
	req->hdrlen=headerlen;
	req->payload=payload;
	req->pldlen=payloadlen;

	// netq_transGlobalRank(from, &id_entry);
	// idx=id_entry.localID*netq_info.gTableSize+to;

	if ((!netq_info.isSingleMode) && ( (mode==NETQ_NBLK) 
				// || (netq_info.lsend_mode[idx]) /* conservative check */
		) ) {

		if (netq_info.movout_down)
			return -1;

		// inc32(&(netq_info.lsend_mode[idx]));
		
		ret=fifo_enqueue(netq_info.outq, req);
		
		return ret;
	}
	else {
		// if (!netq_info.isSingleMode) {
			// if (thr_mtx_lock(&netq_info.send_mtx))
				// return -1;
		// }

		ret=netq_real_send(req);

		// if (!netq_info.isSingleMode) {
			// thr_mtx_unlock(&netq_info.send_mtx);
		// }

		sa_free(netq_info.qalloc, req);

		return ret;
	}
}

int netq_recvdata(int from, char *data, int len)
{
	int realfrom;

	if (netq_getMacID(from, &realfrom))
		return -1;

	return netd_recvdat(realfrom, data, len);
}

/**
 * the root is in global rank. (otherwise,
 * the root will be specified in (macID, localID) pair.
 *
 * Any node can call this function, but the calls
 * need to be serialized. Given the following sample
 * code sequence, seems to me serialization is
 * automatically done. (A local node other than
 * the current local root needs to wait the current local 
 * root to enter intra_bcast, which is strictly 
 * after the netq_bcast.) 
 *
 * if (root is local node)
 * 		localRoot=lrank(root);
 * else
 * 		localRoot=0;
 *
 * if (me == localRoot)
 * 		netq_bcast(...);
 * // memory barrier here 
 * intra_bcast(root=localroot);
 *
 */
int netq_bcast(char *data, int len, int root)
{
	int realroot; 

	if (netq_getMacID(root, &realroot))
		return -1;

	return netd_bcast(data, len, realroot);
}

/**
 * Any node can call this function, but the calls
 * need to be serialized. Given the following sample
 * code sequence, seems to me serialization is
 * automatically done. (Suppose A, B are on the same 
 * machine and are the roots of two consecutive reduce,
 * A being root before B is. B cannot go beyond the 
 * intra_reduce() of the second reduce until after A 
 * enters the intra_reduce() of the second reduce, which
 * is strictly after the netq_reduce of the first 
 * reduce.)
 *
 * if (root is local node)
 * 		localRoot=lrank(root);
 * else
 * 		localRoot=0;
 *
 * intra_reduce(root=localroot);
 *
 * if (me == localRoot)
 * 		netq_reduce(...);
 */
int netq_reduce(char *src, char *result, int nelem, int op, int data_type, int root)
{
	int realroot; 

	if (netq_getMacID(root, &realroot))
		return -1;

	return netd_reduce(src, result, nelem, op, data_type, realroot);
}

/**
 *  intra_reduce(root=0);
 *
 *	if (my local rank == 0)
 *		netq_allreduce(...);
 *
 *	intra_bcast(root=0);
 *
 * Only performed by root 0.
 */
int netq_allreduce(char *src, char *result, int nelem, int op, int data_type)
{
	return netd_allreduce(src, result, nelem, op, data_type);
}

/**
 *
 *  intra_barrier()
 *
 *	if (my local rank==0)
 *		netq_barrier(...);
 *
 *  intra_barrier()
 *
 * Only performed by root 0.
 *
 * This probably can be optimized.
 */
int netq_barrier()
{
	return netd_barrier();
}
