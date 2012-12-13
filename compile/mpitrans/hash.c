/**
 * This file is a simplfied version of the hash table implementation 
 * by Allen I. Holub. 
 * The bigest difference is the "cmp" function used in findsym. It
 * does not take two symbol structure, instead, it takes two keys.
 * findsym also uses a key to find desired symbol instead of a symbol.
 * Thus the maketab requires one more function parameter which
 * extracts the key from the symbol structure.
 */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

# if (DEBUGMEM)
# include "asmalloc.h"
# define malloc(size) as_malloc(size)
# define free(mem) as_free(mem)
# endif
void *newsym(int size)
{
 /* Allocate space for a new symbol; return a pointer to the user space */

 	BUCKET *sym;

 	if (!(sym=(BUCKET *)calloc(size + sizeof(BUCKET),1))) {
		fprintf(stderr,"Can't get memory for BUCKET\n");
		return NULL;
 	}
 	return (void *)(sym+1); /* return pointer to user space */
}

void freesym(void *sym)
{
 	free((BUCKET *)sym -1);
}

/**
 * HASH_TAB looks like the following:
 * +---------------+
 * |               |
 * |    header     |
 * |               |
 * +---------------+
 * |   bucket 0   -+-> double link list
 * +---------------+
 * |   bucket 1   -+-> double link list
 * +---------------+
 * |   ...        -+-> ...
 * +---------------+
 * |   bucket n-1 -+-> double link list
 * +---------------+
 */
HASH_TAB * maketab(
	unsigned maxsym, 
	unsigned (*hash_key)(void *key), /* key to unsigned hashing */
	void * (*get_key)(void *sym), /* extract key from symbol */
	int (* cmp_function)(void *key1, void *key2) /* key comparison */
)
{
 	/* Make a hash table of the indicated size. */
	
 	HASH_TAB *p;
	
 	if (!maxsym)
		maxsym = 127; /* default hashing buckets */
 	if ((p=(HASH_TAB *)calloc(1,(maxsym * sizeof(BUCKET *)) + sizeof(HASH_TAB)))) {
		p->size = maxsym;
		p->numsyms = 0;
		p->hash = hash_key;
		p->get_key = get_key;
		p->cmp = cmp_function;
 	}
 	else {
		fprintf(stderr,"Insufficient memory for symbol table \n");
		return NULL;
 	}
 	return p;
}

/* two wrapper functions */
static int symcmp(HASH_TAB *tabp, void *sym1, void *sym2)
{
	return (*tabp->cmp)((*tabp->get_key)(sym1), (*tabp->get_key)(sym2));
}

static unsigned int symhash(HASH_TAB *tabp, void *sym)
{
	return (*tabp->hash)((*tabp->get_key)(sym));
}

void * addsym(HASH_TAB *tabp, void *isym)
{
 	/* Add a symbol to the hash_table */

 	BUCKET **p,*tmp;
 	BUCKET *sym = (BUCKET *)isym;

 	p = &(tabp->table)[ symhash(tabp, sym--) % tabp->size ];

 	tmp = *p;
 	*p = sym;
 	sym->prev = p;
 	sym->next = tmp;

 	if (tmp)
		tmp->prev = &sym->next;
 	tabp->numsyms ++ ;
 	return (void *)(sym + 1);
}

void delsym(HASH_TAB *tabp, void *isym)
{
 /**
  * remove a symbol from the hash table. " sym " is a pointer returned from
  * a previous findsym() call. It points initially at the user space, but
  * is decremented to get at the BUCKET header.
  */

 	BUCKET *sym = (BUCKET *)isym;

 	if (tabp && sym) {
		--tabp->numsyms;
		--sym;
	 
		if ( (*(sym->prev) = sym->next))
			sym->next->prev = sym->prev;
 	}
}

void *findsym(HASH_TAB *tabp, void *key)
{
 /**
  * Return a pointer to the hash table element having a particular name
  * or NULL if the name isn't in the table.
  */

 	BUCKET *p;
 	if (!tabp) /* table empty */
		return NULL;
 
 	p = (tabp->table)[ (*tabp->hash)(key) % tabp->size ];
	
 	while ( p && (*tabp->cmp)(key,(*tabp->get_key)(p+1) ) ) {
		p = p->next;
	}

 	return (void *)( p?p+1:NULL );
}

void * nextsym(HASH_TAB *tabp, void *i_last)
{
 
 	BUCKET *last = (BUCKET *)i_last;

 	for (--last; last->next; last=last->next)
		if ( symcmp(tabp, last+1,last->next + 1)==0) /* keys match */
			return (void *)(last->next +1);
 	return NULL;
}

/* a simple hash function */
unsigned hash_add(void *key)
{
	register unsigned char *name=(unsigned char *)key;
 	unsigned h;
 	for (h=0;*name;h+=*name++)
		;
 	return h;
}

#define HIGH_BITS		(~( (unsigned)(~0) >> 4))

/* a better hash function */
unsigned hash_pjw(void *key)
{
	register unsigned char *name=(unsigned char *)key;
 	unsigned h = 0;
 	unsigned g;

 	for (;*name;++name) {
		h = ( h << 4) + *name;
		if ( (g = h & HIGH_BITS) )
			h = (h^(g>>24)) & ~HIGH_BITS; 
 	}
 	return h;
}


/* refined by Hong Tang to utilize the highest four bits */
unsigned hash_pjw2(void *key)
{
	register unsigned char *name=(unsigned char *)key;
 	unsigned h = 0;
 	unsigned hh;
 	unsigned g;

 	for (;*name;++name) {
		h = ( h << 4) + *name;
		hh=h;
		if ( (g = h & HIGH_BITS) )
			h = (h^(g>>24)) & ~HIGH_BITS; 
 	}
 	return hh;
}

