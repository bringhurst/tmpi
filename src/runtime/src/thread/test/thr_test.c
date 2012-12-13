# include <unistd.h>
# include "thread.h"

void *start(void *dump)
{
	sleep(10);
	return NULL;
}

int main()
{
	thr_t thr;

	thr=thr_createone(start, NULL);
	thr_joinone(thr, NULL);

	return 0;
}
