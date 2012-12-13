# include "tmpi_i386.h"

__inline__ int cas32(volatile int *addr, int old, int new)
{
    char ret;

    __asm__ __volatile__(LOCK_PREFIX
                         "cmpxchg %2, %1\n\t"   /* swap values if current value
                                                   is known */
                         "sete %0"              /* read success/fail from EF */
                         : "=q" (ret), "=m" (ADDR)
                         : "r" (new), "a" (old)
						 : "%eax" );
    return (int)ret;
}

int cas64(volatile long long *ptr, long long old, long long new);

__inline__ int casp(void * *pval, void * old, void * new)
{
	/**
	 * somehow I have to decide the size of (void *)
	 * during the runtime.
	 */
	if (sizeof(void *)==sizeof(int))
		return cas32((int*)pval, (int)old, (int)new);
	else 
		return cas64((long long *)pval, (long long)old, (long long)new);
}

__inline__ int test_and_set(volatile void *addr, int bit)
{
    int oldbit;

    __asm__ __volatile__(LOCK_PREFIX
                         "btsl %2, %1\n\t"  /* set bit, store old in CF */
                         "sbbl %0, %0"      /* oldbit -= CF */
                         : "=r" (oldbit),"=m" (ADDR)
                         : "Ir" (bit));

    return (-oldbit);
}

__inline__ int test_and_clear(volatile void *addr, int bit)
{
    int oldbit;

    __asm__ __volatile__(LOCK_PREFIX
                         "btrl %2, %1\n\t"  /* clear bit, store old in CF */
                         "sbbl %0, %0"      /* oldbit -= CF */
                         : "=r" (oldbit), "=m" (ADDR)
                         : "Ir" (bit));

    return (-oldbit);
}

__inline__ int test_and_change(volatile void *addr, int bit)
{
    int oldbit;

    __asm__ __volatile__(LOCK_PREFIX
                         "btcl %2, %1\n\t"  /* change bit, store in CF */
                         "sbbl %0, %0"      /* oldbit -= CF */
                         : "=r" (oldbit), "=m" (ADDR)
                         : "Ir" (bit));

    return (-oldbit);
}

__inline__ int anf32(volatile int *ptr, int val)
{
	int old;
	
	if (!val) return *ptr;

	do {
		old=*ptr;
	} while (!cas32(ptr, old, old+val));

	return old+val;
}

__inline__ int snf32(volatile int *ptr, int val)
{
	int old;
	
	if (!val) return *ptr;

	do {
		old=*ptr;
	} while (!cas32(ptr, old, old-val));

	return old-val;
}
__inline__ int fna32(volatile int *ptr, int val)
{
	int old;
	
	if (!val) return *ptr;

	do {
		old=*ptr;
	} while (!cas32(ptr, old, old+val));

	return old;
}

__inline__ int fns32(volatile int *ptr, int val)
{
	int old;
	
	if (!val) return *ptr;

	do {
		old=*ptr;
	} while (!cas32(ptr, old, old-val));

	return old;
}

__inline__ long long anf64(volatile long long *ptr, long long val)
{
	long long old;
	
	if (!val) return *ptr;

	do {
		old=*ptr;
	} while (!cas64(ptr, old, old+val));

	return old+val;
}

__inline__ int snf64(volatile long long *ptr, long long val)
{
	long long old;
	
	if (!val) return *ptr;

	do {
		old=*ptr;
	} while (!cas64(ptr, old, old-val));

	return old-val;
}
