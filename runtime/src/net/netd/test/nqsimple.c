#include <stdio.h>
#include "../netq.h"
    
int msg_handler(int from, int to, char *header, int len, int payloadSize)
{
	return 0;
}

int main(int argc, char *argv[])
{
    netq_initEnv(&argc,&argv);

	netq_start(NULL, NULL, NULL, NULL, msg_handler);

	sleep(10);

    netq_end();

	return 0;
}
