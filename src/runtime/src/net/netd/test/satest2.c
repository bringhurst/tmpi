# include <stdio.h>
# include <stdlib.h>
# include "thread.h"
# include "atomic.h"
# include "../salf.h"
# include "satestcomm.h"

StackAllocator sa;


volatile int b1=0, b2=0;

void *worker(void *arg)
{
	int i, j;
	void *items[CONCURRENT];

	for (i=0; i<CONCURRENT; i++)
		items[i]=NULL;

	inc(&b1);
	while (b1!=(NWORKER));
	inc(&b2);
	while (b2!=(NWORKER));

	for (j=0; j<NITER; j++) {
		/* get i from 0 to CONCURRENT -1 */
		i=rand()%CONCURRENT;

		if (items[i]) {
			sa_free(&sa, items[i]);
			items[i]=NULL;
		}
		else
			items[i]=sa_alloc(&sa);
	}

	dec(&b1);
	while (b1!=0);
	dec(&b2);
	while (b2!=0);

	for (i=0; i<CONCURRENT; i++)
		if (items[i]) {
			sa_free(&sa, items[i]);
			items[i]=NULL;
		}

	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t thrs[NWORKER];
# if (CHECK)
	char *ret;
# endif
	
	int i;

	sa_init(&sa, 37, NWORKER*CONCURRENT);

	for (i=0; i<NWORKER; i++)
		pthread_create(thrs+i, NULL, worker, NULL);

	for (i=0; i<NWORKER; i++)
		pthread_join(thrs[i], NULL);

# if (CHECK)
	if ((ret=(char *)sa_check(&sa))) {
		fprintf(stderr, "StackAllocator check failed!\n");
		switch ((int)ret) {
			case 1 :
				fprintf(stderr, "Count doesn't match.\n");
				break;
			default :
				fprintf(stderr, "Invalid block %p\n", ret);
		}
		sa_dump(&sa, stderr);
	}
	else {
		fprintf(stderr, "StackAllocator check passed!\n");
	}
# endif
	return 0;
}

