# include "atomic.h"

static int a=10;

static long long b=100;

static double c=0.111;

main()
{
	printf("a=%d, c=%e, b=%ld\n", a, c, b);
	printf("cas(a, 10, 11)=%d", cas32(&a, 10, 11));
	printf(", a=%d\n", a);
	printf("cas(b, 100, 110)=%d", cas64(&b, (long long)100, (long long)110));
	printf(", b=%ld\n", b);
	printf("cas(c, 0.111, 0.911)=%d", casd(&c, (double)0.111, (double)0.911)); 
	printf(", c=%e\n", c);

	printf("cas(a, 10, 11)=%d", cas32(&a, 10, 11));
	printf(", a=%d\n", a);
	printf("cas(b, 100, 110)=%d", cas64(&b, (long long)100, (long long)110));
	printf(", b=%ld\n", b);
	printf("cas(c, 0.111, 0.911)=%d", casd(&c, (double)0.111, (double)0.911)); 
	printf(", c=%e\n", c);
}

