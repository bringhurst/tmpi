# include <stdio.h>
# include "../usc_time.h"

main()
{
	usc_time_t t1, t2;

	usc_init();

	t1=usc_gettime();
	printf("MHZ=%d\n", (int)usc_mhzrate());
	t2=usc_gettime();
	printf("the above printf takes %lf microseconds.\n", usc_difftime(t2, t1));
	printf("t1=%lf, t2=%lf\n", usc_wtime(t1), usc_wtime(t2));
	t1=usc_gettime();
	sleep(10);
	t2=usc_gettime();
	printf("sleeping 10 seconds will give me %lf microseconds.\n", usc_difftime(t2, t1));
}
