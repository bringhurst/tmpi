#include <stdio.h>
#include "../netd.h"
    
static void dump_args(char *prompt, int argc, char *argv[])
{
	int i;

	printf("%s\n", prompt);

	for (i=0; i<argc; i++)
		printf("%d) %s\n", i+1, argv[i]);
}

int main(int argc, char *argv[])
{
	Machine machines[5]={
		{"local", 1},
		{"node37", 1},
		{"node39", 1},
		{"node40", 1},
		/**
		**/
		{"nothing", -1}
	};
	char prompt[1024];

    netd_initEnv(&argc,&argv, NULL);

	netd_start(machines);

	sprintf(prompt, "ID %d:", netd_getMyID());

	dump_args(prompt, argc, argv);
	sleep(10);

    netd_waitAll();

	return 0;
}
