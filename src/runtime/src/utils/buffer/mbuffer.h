# ifndef _MBUFFER_H_
# define _MBUFFER_H_

/* allocat/free from the owner's buffer space */
void mb_free(int owner, void* item);
void *mb_alloc(int owner, int isize);

int mb_init(int myid, int bsize);
int mb_end(int myid);

int mb_global_init(int nproc);
int mb_global_end();

int mb_attach_buffer(int myid, void *buffer, int size);
int mb_detach_buffer(int myid, void *buffer, int *size);

# endif
