# include <stdio.h>
# include <stdlib.h>
# include "atomic.h"
# include "../fifo.h"

# define CHECK 0
# define PRINT 0

int n=0;

FIFO *q;
# if (CHECK)
char *chararr;
# endif

# define NENQER 1
# define NDEQER 1

# define NITER 1000000
volatile int b1=0, b2=0;
volatile int c1=NENQER, c2=NENQER;
volatile int d1=NDEQER, d2=NDEQER;

void *enquer(void *arg)
{
	int i, j;

	inc(&b1);
	while (b1!=(NENQER+NDEQER));
	inc(&b2);
	while (b2!=(NENQER+NDEQER));

	for (j=0; j<NITER; j++) {
		i=anf32(&n, 1);
		fifo_enqueue(q, (void *)i);
	}

	dec(&c1);
	while (c1!=0);
	dec(&c2);
	while (c2!=0);

	return NULL;
}

void *dequer(void *arg)
{
	int i;

	inc(&b1);
	while (b1!=(NENQER+NDEQER));
	inc(&b2);
	while (b2!=(NENQER+NDEQER));

	while (1) { 
		i=(int)fifo_dequeue(q);
		if (i==0)
			break;
# if (PRINT)
		printf("%d\n", i);
# endif

# if (CHECK)
		if (chararr[i-1])
			printf("%d is already seen\n", i);
		else
			chararr[i-1]=1;
# endif
	}

	dec(&d1);
	while (d1!=0);
	dec(&d2);
	while (d2!=0);

	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t thrs[NENQER+NDEQER];
	
	int i, j;
	int qlen=FIFO_LEN_DEFAULT;

	if (argc>1) {
		qlen=atoi(argv[1]);
	}

	q=fifo_create(qlen); /* smaller buffer to saturate the buffer */
# if (CHECK)
	chararr=(char *)calloc(NITER*NENQER, sizeof(char));
# endif

	for (i=0; i<NENQER; i++)
		pthread_create(thrs+i, NULL, enquer, NULL);
	for (; i<NENQER+NDEQER; i++)
		pthread_create(thrs+i, NULL, dequer, NULL);

	for (i=0; i<NENQER; i++)
		pthread_join(thrs[i], NULL);

	for (j=0; j<NDEQER; j++)
		fifo_enqueue(q, (void *)0);

	for (; i<NENQER+NDEQER; i++)
		pthread_join(thrs[i], NULL);

# if (CHECK)
	for (j=0; j<NITER*NENQER; j++)
		if (chararr[j]==0) {
			printf("%d hasn't been seen\n", j+1);
		}
# endif
	return 0;
}
