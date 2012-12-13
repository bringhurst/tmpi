# ifndef _ATOMIC_H_
# define _ATOMIC_H_

# include "machine.h"

int cas32(volatile int *pval, int old, int new);
int cas64(volatile long long *pval, long long old, long long new);
int casd(volatile double *pval, double old, double new);

/* casp */
# if (SIZEOF_PTR == SIZE_UNKNOWN)
int casp(void * *pval, void * old, void * new);
# elif (SIZEOF_PTR == SIZEOF_INT)
# define casp(pval, old, new) ( cas32((int *)(pval), (int)(old), (int)(new)) )
# elif (SIZEOF_PTR == SIZEOF_LONG)
# define casp(pval, old, new) ( cas64((long long*)(pval), (long long)(old), (long long)(new)) )
# endif

int test_and_set(volatile void *addr, int bit);
int test_and_clear(volatile void *addr, int bit);
int test_and_change(volatile void *addr, int bit);

int anf32(int *pval, int val);
int fna32(int *pval, int val);
int snf32(int *pval, int val);
int fns32(int *pval, int val);

long long anf64(long long *pval, long long val);
long long snf64(long long *pval, long long val);

/* some hacks to defeat gcc over-optimizations */
struct __dummy0 { unsigned long a[100]; };
#define ADDR1(addr) (*(volatile struct __dummy0 *)(addr))

/** commit a memory cell **/
# define commit32(ptr32) \
	asm __volatile__ ( \
		"lock; " "addl $0, %0\n\t" \
		: :"m"(ADDR1(ptr32)) \
	)

# define commit64(ptr64) \
	asm __volatile__ ( \
		"lock; " "addl $0, %0\n\t" \
		"lock; " "addl $0, %1\n\t" \
		: :"m"(ADDR1(ptr64), "m"(ADDR1((char *)(ptr64)+4)) \
	)

# define inc(pval) \
	asm __volatile__ ( \
			"lock; " "addl $1, %0" \
			: "=m"(ADDR1(pval)) \
			: \
			)

# define dec(pval) \
	asm __volatile__ ( \
			"lock; " "subl $1, %0" \
			: "=m"(ADDR1(pval)) \
			: \
			)

# define aadd32(pval, val) \
	asm __volatile__ ( \
			"lock; " "addl %1, %0" \
			: "=m"(ADDR1(pval)) \
			: "r"(val) \
			)

# define asub32(pval, val) \
	asm __volatile__ ( \
			"lock; " "subl %1, %0" \
			: "=m"(ADDR1(pval)) \
			: "r"(val) \
			)
# endif
