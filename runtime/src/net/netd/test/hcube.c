#include <stdio.h>
#include "../netd.h"
#include "../netd_comm.h"
    
int main(int argc, char *argv[])
{
	Machine machines[]={
		{"local", 1},
		{"node21", 1},
		{"node22", 1},
		{"node23", 1},
		{"node24", 1},
		{"node37", 1},
		{"node39", 1},
		{"node40", 1},
		{"node42", 1},
		{"", -1}
	};
	int root=0;

    netd_initEnv(&argc,&argv, NULL);
    netd_start(machines);
	if (argc>1)
		root=atoi(argv[1]);
	netd_dump_hcube(root);
    netd_waitAll();

	return 0;
}
