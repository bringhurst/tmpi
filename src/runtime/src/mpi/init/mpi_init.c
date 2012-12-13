# include "mpi_init.h"
# include "netq.h"
# include "tmpi_debug.h"
# include "intra_sr.h"
# include "comm.h"
# include "mbuffer.h"
# include "thread.h"
# include "ukey.h"
# include "usc_time.h"

/**
 * currently we don't control the memory usage for internal
 * data structrues
 *
 * But we are using 14M for each MPI node for system buffer.
 *
 * Sometime it fails to get 14M bytes
 */
# define GLOBAL_SHARE (0x1000000)
# define SYS_BUFFER (GLOBAL_SHARE - 0x200000)
# define MAX_THREAD_PER_NODE 256
# define MAX_UKEYS 65536

int tmpi_main_init(int *argc, char ***argv)
{
	int ret=0;
	int local_size;

	ret+=netq_initEnv(argc, argv);
	ret+=netq_start(NULL, NULL, NULL, NULL, (int (*)(int, int, char *, int, int))tmpi_inter_msg_handle);

	local_size=netq_getLocalSize();
	if (local_size<=0) {
		return -1;
	}

	ret+=tmpi_intra_init(local_size);
	ret+=tmpi_comm_ginit(local_size);
	ret+=mb_global_init(local_size);
	ret+=thr_init(MAX_THREAD_PER_NODE);
	ret+=ukey_setkeymax(MAX_UKEYS);
	usc_init();
	tmpi_prog_main_init();

	return ret;
}

int tmpi_main_end()
{
	int ret=0;

	ret+=thr_end();
	ret+=mb_global_end();
	ret+=tmpi_comm_gend();
	tmpi_intra_end();
	ret+=netq_end();

	return ret;
}

int tmpi_thr_init(int *argc, char ***argv, int myid)
{
	int ret=0;

	ret+=mb_init(myid, SYS_BUFFER);
	ret+=tmpi_comm_init(myid);
	tmpi_prog_user_init();

	return ret;
}

int tmpi_thr_end(int myid)
{
	int ret=0;

	ret+=tmpi_comm_end(myid);
	ret+=mb_end(myid);

	return ret;
}
