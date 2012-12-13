# ifndef _STACKALLOCATOR_H_
# define _STACKALLOCATOR_H_
# include <stdio.h>
# include "thread.h"

typedef struct _listable {
	struct _listable *next;
	char content[1]; /* content */
} Listable;

typedef struct _stackallocator {
	Listable *first;
	thr_mtx_t lock;
	int bufIdx;
	char **buffers;
	int maxBuffers;
	int size;
} StackAllocator;

int sa_init(StackAllocator *sa, int size, int max_items);
int sa_destroy(StackAllocator *sa);
void *sa_alloc(StackAllocator *sa);
int sa_free(StackAllocator *sa, void *item);
int sa_check(StackAllocator *sa);
void sa_dump(StackAllocator *sa, FILE *out);
# endif
