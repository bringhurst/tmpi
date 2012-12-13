# ifndef _NETQ_H_
# define _NETQ_H_

# define NETQ_MAX_HOST_NAME 255
# define NETQ_MAX_NODES_PER_HOST 256
# define NETQ_MAX_NODES 256

# define NETQ_MASTER 1
# define NETQ_SLAVE 3

/* global id to local id */
typedef struct mac_entry {
	char szHost[NETQ_MAX_HOST_NAME+1]; /* host name */
	int nNodes; /* number of MPI nodes on the host */
	int nodeIDs[NETQ_MAX_NODES_PER_HOST];
} MAC_ENTRY; /* a single machine entry */

typedef struct id_entry {
	int macID;
	int localID;
} ID_ENTRY; /* invariant: machines[macID].nodeIDs[localID] == index */

typedef struct prog_res {
	int nNodes; /* not the same as nNodes in the mac_entry.
				   It is the total number of MPI Nodes */
	int nExecTime; /* acculated execution time of the program in minutes */
	int nCPU; /* accumulated cpu time in minutes */
	short nMemory; /* memory usage, in MegaBytes */
	short nDisk; /* disk usage in MegaBytes */
	int nNetComm; /* net communication volumn in KiloBytes */
	int nMachines; /* hint or back filled by query service if not set */
	MAC_ENTRY **cache;
	int ignore_cache; /* if 0, try to return the cached mac list info */
} PROG_RES;

typedef struct prog_topo {
	int empty_now;
} PROG_TOPO; /* will consider it later */

/**
 * query the resource allocator service to get a set of 
 * cluster nodes to run the program with the specific
 * resource requirement and communication topology.
 *
 * Note, that if the cache of both res and topo are
 * the same, we can send back the cached information
 * for query again. I don't know why or if people wants
 * to do that. But just in case people do call query
 * service twice, we might get something.
 */
MAC_ENTRY ** netq_queryNodes(PROG_RES *res, PROG_TOPO *topo);

/** fill in the resource structure, return the machine group */
MAC_ENTRY ** netq_parseGrpFile(char *filename);
int netq_parseResFile(char *filename, PROG_RES *res, PROG_TOPO *topo);

/* get a default resource spec, res has to be allocated space */
int netq_getDefaultResource(PROG_RES *res);

/* get a default topology spec, res has to be allocated space */
int netq_getDefaultTopology(PROG_TOPO *topo);

int netq_amIStartupMaster();

/**
 * To eliminate extra buffering, I ask for the application 
 * layer to provide several call back handles.
 * Ideally only handleMsg is needed and NETQ can manage address
 * embedding and extraction, but this will involve too much
 * copying. Application layer should leave two space holders
 * for the from and to address. But it should always use the
 * addresses given as parameters and not rely on the contents
 * in the space holders.
 */

/**
 * If the upper layer decides to put the 
 * following structure as the beginning of their
 * message header, then feel free to use our
 * default fillers/extractors.
 */
typedef struct netq_header {
	int from;
	int to;
	int hdrlen;
	int pldlen;
} NETQ_HEADER;

void netq_fillFromAddr(int from, char *header, int len);
void netq_fillToAddr(int to, char *header, int len);
void netq_fillHeaderSize(int hdrsize, char *header, int len);
void netq_fillPayloadSize(int ploadsize, char *header, int len);
int netq_getFromAddr(char *header, int len);
int netq_getToAddr(char *header, int len);
int netq_getHeaderSize(char *header, int len);
int netq_getPayloadSize(char *header, int len);

typedef struct netq_handles {
	void (*fillFromAddr)(int from, char *header, int len);
	void (*fillToAddr)(int to, char *header, int len);
	void (*fillHeaderSize)(int hdrsize, char *header, int len);
	void (*fillPayloadSize)(int ploadsize, char *header, int len);
	int (*getFromAddr)(char *header, int len);
	int (*getToAddr)(char *header, int len);
	int (*getHeaderSize)(char *header, int len);
	int (*getPayloadSize)(char *header, int len);
} NETQ_HANDLE;

typedef	int (*netq_msg_handler)(int from, int to, char *header, int len, int payloadSize);
/* setting up connections and exchange ID translation table */

int netq_start(NETQ_HANDLE *nh, MAC_ENTRY **mac_list, PROG_RES *res, PROG_TOPO *topo, netq_msg_handler mh);

/* once called, no functions can be called **/
int netq_end(); 


/**
 * id/rank translations
 */
int netq_transGlobalRank(int grank, ID_ENTRY *lrank);
int netq_findGlobalRank(ID_ENTRY *id_entry, int *grank);

int netq_transLocalRank(int lrank, int *grank);

int netq_isLocalNode(int grank);

int netq_getLocalMacID();

int netq_getMacID(int grank, int *macid);

/**
 * netq doesn't take care of the local ID
 * because I don't want to get messed up with
 * thread creation.
 *
 * for (i=0; i<netq_getLocalGroupSize(); i++) {
 * 		// create a thread,
 * 		// inside thread startup stub, call
 * 		// netq_setMyID(i);
 * 	}
 *
 *
 */

/**
 * for the convenience, I defined the following
 * macros
 */
# define netq_getMyID ukey_getid()
# define netq_setMyID(id) ukey_setid(id)

int netq_getLocalSize();

int netq_getGlobalSize();

int netq_getMacSize();

int netq_initEnv(int *argc, char ***argv);

# endif
