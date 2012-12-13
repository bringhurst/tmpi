# ifndef _BCAST_H_
# define _BCAST_H_

# include "ukey.h"
# include "thread.h"

typedef struct tmpi_BCHandle {
	struct tmpi_BCHandle *next;
	int avail; /* 0: not available, k: availabe and (k-1) has read */
	char *data;
	unsigned int len; /* in bytes */
	thr_mtx_t lock;
	thr_cnd_t wait;
} tmpi_BCHandle;
	
typedef struct tmpi_BCQueue {
	ukey_t key_curhandle; /* each process keeps the current handle pointer */
	struct tmpi_BCHandle *free;
	struct tmpi_BCHandle *first;
	struct tmpi_BCHandle cache[1]; /* a variable length array */
} tmpi_BCQueue;	

typedef struct tmpi_BcastObj {
	int group_size;
	tmpi_BCQueue *(bco_array[1]); /* indexed by the root, variable length */
} tmpi_BcastObj;
	
tmpi_BcastObj *tmpi_bco_create(int groupsize); /* constructor */
int tmpi_bco_destroy(tmpi_BcastObj *bco); /* destructor */
int tmpi_bco_bcast(tmpi_BcastObj *bco, int myid, int root, char *data, unsigned int len /* in bytes */);

# endif
