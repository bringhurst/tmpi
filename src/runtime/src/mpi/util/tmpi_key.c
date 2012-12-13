# include "ukey.h"

int thrMPI_createkey()
{
	int key;
	if (ukey_create(&key)<0)
		return  -1;

	return key;
}

void thrMPI_setval(int key, void *val)
{
	ukey_setval(key, val);
}

void *thrMPI_getval(int key)
{
	return ukey_getval(key);
}
