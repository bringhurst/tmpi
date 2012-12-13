# include <stdlib.h>
# include "thread.h"
# include "netq.h"
# include "mpi_init.h"
# include "tmpi_debug.h"

struct tmpi_para {
	int argc;
	char **argv;
	int myid;
};

static int tmpi_thr_stub(void *arg)
{
	struct tmpi_para *para=(struct tmpi_para *)arg;
	int ret;

	if (tmpi_thr_init(&(para->argc), &(para->argv), para->myid)<0)
		return -1;

	ret=tmain(para->argc, para->argv);

	tmpi_thr_end(para->myid);

	return ret;
}

static void free_para(struct tmpi_para *paras, int count)
{
	int i, j;

	if (paras) {
		for (i=0; i<count; i++) {
			if (paras[i].argv) {
				for (j=0; j<paras[i].argc; j++) {
					if (paras[i].argv[j])
						free(paras[i].argv[j]);
				}
				free(paras[i].argv);
			}
		}

		free(paras);
	}
}

int MAIN__(int argc, char *argv[]) __attribute__ ((weak, alias ("main")));

int main(int argc, char *argv[])
{
	int local_size;
	int i, j;
	int success;
	struct tmpi_para *paras;
	thr_t *threads;

	if (tmpi_main_init(&argc, &argv)<0) {
		tmpi_error(DBG_FATAL, "Main initialization failed.");
		return -1;
	}

	local_size=netq_getLocalSize();

	if (local_size<0) {
		tmpi_error(DBG_FATAL, "Cannot get local group size.");
		tmpi_main_end();
		return -1;
	}

	paras=(struct tmpi_para *)calloc(local_size, sizeof(struct tmpi_para));
	threads=(thr_t *)calloc(local_size, sizeof(thr_t));

	if (!paras) {
		tmpi_error(DBG_FATAL, "Cannot allocate parameter array.");
		tmpi_main_end();
		return -1;
	}

	success=1;
	for (i=0; i<local_size; i++) {
		paras[i].myid=i;
		paras[i].argc=argc;
		paras[i].argv=(char **)calloc(argc+1, sizeof(char *));
		if (!paras[i].argv) {
			success=0;
			break;
		}
		for (j=0; j<argc; j++) {
			paras[i].argv[j]=(char *)malloc(strlen(argv[j])+1);
			if (!paras[i].argv[j]) {
				success=0;
				break;
			}
			strcpy(paras[i].argv[j], argv[j]);
		}

		if (!success)
			break;
	}

	if (!success) {
		free_para(paras, local_size);
		tmpi_error(DBG_FATAL, "Cannot allocate parameter array.");
		tmpi_main_end();

		return -1;
	}

	for (i=0; i<local_size; i++) {
		threads[i]=thr_createbatch(tmpi_thr_stub, i, 0, (void *)(paras+i));
	}

	thr_start(local_size);

	/* now everything should have been done */
	free(threads);
	free_para(paras, local_size);
	
	tmpi_main_end();

	return 0;
}
