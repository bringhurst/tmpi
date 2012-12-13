/*************************************************/

# include <pthread.h>
# include "atomic.h"
# include "tmpi_debug.h"
# define ID_MAX 1000

static pthread_key_t pthread_idkey=0; /* 0 means not yet initialized */

void ukey_setid(short id)
{
	pthread_key_t newkey;

	if (pthread_idkey==0) { /* hasn't been set */
		pthread_key_create(&newkey, NULL);
		if (!cas32(&pthread_idkey, (pthread_key_t)0, newkey)) 
		/* idkey has already been set */
			pthread_key_delete(newkey);
	}

	pthread_setspecific(pthread_idkey, (void *)(long)(unsigned short)id);
	return;
}

short ukey_getid(void)
{
	return (short)(long)pthread_getspecific(pthread_idkey);
}

int ukey_create(pthread_key_t *pkey)
{
	return pthread_key_create(pkey, NULL);
}

int ukey_setval(pthread_key_t key, void *val)
{
	return pthread_setspecific(key, val);
}

void * ukey_getval(pthread_key_t key)
{
	return pthread_getspecific(key);
}

int ukey_setkeymax(int max)
{
	/* tmpi_error(DBG_INTERNAL, "ukey_setspecific for pthread is not defined."); */
	return 0;
}
