/* 
 * scon.c
 * Sequential consistence checking
 */

/**
#include <sys/types.h>
**/
# include <stdlib.h>
# include <stdio.h>
# include <pthread.h>
# include "atomic.h"

# define REPEAT ((unsigned long)100000)
# define MSG_SIZE 64

volatile int x=0, y=0, lx=0, ly=0, b1=0, b2=0;
int error=0;

void *sconx(void *);
void *scony(void *);

int n=REPEAT;

void main(int argc, char *argv[]) 
{
  int i;

  pthread_t tid[2];

  if (argc>=2)
	n=atoi(argv[1]);

  pthread_create(&(tid[0]), NULL, sconx, NULL);
  pthread_create(&(tid[1]), NULL, scony, NULL);

  for (i=0; i<2; i++) {
	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
  }

  if (error==0) {
	  printf("Possible sequential consistent architecture.\n");
	}
	return 0;
}

void *sconx(void *arg)
{
   int i;
	register int t;

   fprintf(stderr, "start\n");
   fflush(stderr);
   for (i=1; i<=n; i++) {

	/* entry barrier */
	anf32(&b1, 1);
	while (b1!=2);
	anf32(&b2, 1);
	while (b2!=2);

	/**
	 * x=i;
	 * ly=y;
	 */
	/**
	asm __volatile__ ( 
		"movl %4, %0\n\t"
		"movl %3, %2\n\t"
		"movl %2, %1\n\t"
		: "=m"(x), "=m"(ly), "=r"(t)
		: "m" (y), "r"(i), "2"(t)
		);
	**/
	asm __volatile__ ( 
		"movl %1, %0"
		: "=m"(x)
		: "r"(i)
	);

	// commit32(&x);

	asm __volatile__ (
		"movl %2, %1\n\t"
		"movl %1, %0\n\t"
		: "=m"(ly), "=r"(t)
		: "m" (y), "1"(t)
		);

	// commit32(&ly);

	/* exit barrier */
	snf32(&b1, 1);
	while (b1!=0);
	snf32(&b2, 1);
	while (b2!=0);

	if ( (ly!=i) && (lx!=i) ) {
		fprintf(stderr, "Sequential consistency violated! lx=%d, ly=%d, x=%d, y=%d\n", lx, ly,
		x, y);
		error++;
		return NULL;
	}
  }
}

void *scony(void *arg)
{
   int i;
   register int t;

   fprintf(stderr, "start\n");
   fflush(stderr);
   for (i=1; i<=n; i++) {

	/* entry barrier */
	anf32(&b1, 1);
	while (b1!=2);
	anf32(&b2, 1);
	while (b2!=2);

	/**
	 * y=i;
	 * lx=x;
	 */
	/**
	asm __volatile__ (
		"movl %4, %0\n\t"
		"movl %3, %2\n\t"
		"movl %2, %1\n\t"
		: "=m"(y), "=m"(lx), "=r"(t)
		: "m" (x), "r"(i), "2"(t)
		);
	**/
	
	asm __volatile__ ( 
		"movl %1, %0"
		: "=m"(y)
		: "r"(i)
	);

	// commit32(&y); 

	asm __volatile__ (
		"movl %2, %1\n\t"
		"movl %1, %0\n\t"
		: "=m"(lx), "=r"(t)
		: "m" (x), "1"(t)
		);

	// commit32(&lx);

	/* exit barrier */
	snf32(&b1, 1);
	while (b1!=0);
	snf32(&b2, 1);
	while (b2!=0);

	if ( (ly!=i) && (lx!=i) ) {
		fprintf(stderr, "Sequential consistency violated! lx=%d, ly=%d, x=%d, y=%d\n", lx, ly,
		x,y);
		error++;
		return NULL;
	}
  }
}
