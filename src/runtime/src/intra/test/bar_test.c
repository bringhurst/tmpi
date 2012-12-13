# include <stdio.h>
# include <stdlib.h>
# include "thread.h"
# include "barrier.h"

# define NWORKER 4

tmpi_Bar bar;

int worker(void *arg)
{
	return bar_wait(&bar);
}

int main(int argc, char *argv[])
{
	int i;

	thr_init(1000);
	bar_init(&bar, NWORKER);

	for (i=0; i<NWORKER; i++)
		thr_createbatch(worker, i, 0, NULL);

	thr_start(NWORKER);

	bar_destroy(&bar);
	thr_end();

	return 0;
}

