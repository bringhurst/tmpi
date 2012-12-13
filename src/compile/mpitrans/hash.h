# ifndef _HASH_H_
# define _HASH_H_
typedef struct BUCKET
{
	struct BUCKET *next;
	struct BUCKET **prev;
} BUCKET;

typedef struct hash_tab
{
	int	size;		/* Max number of elements in table */
	int 	numsyms;	/* number of elements currently in table */
	/* it is a bit awkward to request for two hash functions */
	unsigned (*hash)(void *key);	/* hashing based by key */
	void* (*get_key)(void *sym);	/* extract the key from sym */
	int 	(*cmp)(void *k1, void *k2);/* comparison function */
	BUCKET 	*table[1];	/* First element of actual hash table */
}HASH_TAB;

extern void *newsym(int size);
extern void freesym(void *sym);
extern HASH_TAB * maketab(
	unsigned maxsym, 
	unsigned (*hash_key)(void *key),
	void * (*get_key)(void *sym),
	int (* cmp_function)(void *key1, void *key2)
);

extern void * addsym(HASH_TAB *tabp, void *isym);
extern void delsym(HASH_TAB *tabp, void *isym);
extern void *findsym(HASH_TAB *tabp, void *key);
extern void * nextsym(HASH_TAB *tabp, void *i_last_sym);
extern unsigned hash_add(void *name);
extern unsigned hash_pjw(void *name);
# endif
