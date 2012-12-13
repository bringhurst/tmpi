# include <stdio.h>
# include <pthread.h>
# include "../atomic.h"

int m, n=0;

# define ITERATION 1000000
# define NTHREAD 10

int b1=0; b2=0;

void *worker(void *arg)
{
	int i=0;

	inc(&b1);
	/* anf32(&b1, 1); */
	while (b1!=NTHREAD);
	inc(&b2);
	/* anf32(&b2, 1); */
	while (b2!=NTHREAD);

	for (; i<ITERATION; i++) {
		inc(&m);
		dec(&n);
	}

	/* snf32(&b1, 1); */
	dec(&b1);
	while (b1!=0);
	/* snf32(&b2, 1); */
	dec(&b2);
	while (b2!=0);

	return NULL;
}
	
int main()
{
	int i=0;
	pthread_t thrs[NTHREAD];

	for (i=0; i<NTHREAD; i++) {
		pthread_create(thrs+i, NULL, worker, NULL);
	}

	for (i=0; i<NTHREAD; i++) {
		pthread_join(thrs[i], NULL);
	}

	printf("%d, %d\n", m, n);

	return 0;
}
