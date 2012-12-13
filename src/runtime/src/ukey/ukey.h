# ifndef _UKEY_H_
# define _UKEY_H_

# include "ukey_def.h"
short ukey_getid(void);
int ukey_setid(short id);
int ukey_create(ukey_t *key);
int ukey_setval(ukey_t key, void *val);
void *ukey_getval(ukey_t key);
int ukey_setkeymax(int max);

# endif
