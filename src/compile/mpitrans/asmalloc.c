# include <stdio.h>
# include <stdlib.h>
# include <strings.h>
# include "asmalloc.h"

/* single process version */
/* array size that stores pointer and size for all mallocs in program
that are generated with AS_Malloc or AS_Strdup (don't use AS_free on
any pointer that was not allocated with AS_Malloc or AS_Strdup) -
increase this value as necessary for larger programs */

#define MALLARRSZ 4096

/* 'blowup' value: if the malloc bug is not due to freeing twice or
   freeing sth. that hasn't been malloced than it must be due to
   overwriting malloc boundaries. we can figure this out by allocating
   inside this wrapper more than actually requested (size*BLOWUP) and
   fill the excess memory ([size*1 - size*BLOWUP-1]) with
   TESTCHARs. During AS_free()s we can test this excess memory if the
   application overwrote the TESTCHARs. The trick is setting BLOWUP to a

   'high enough' value that the code runs to completion (if the malloc
   bug is caused by a boundary overwriting there always exists such a
   BLOWUP value that the code won't break). Since it completes we'll see

   all the AS_free()'s corresponding to the AS_Mallocs()s...and
   therefore can see all possible boundary overwritings ... well, that
   is, only if YOU are a consentious programmer that frees the stuff
   that you malloc and don't rely on exit() to take care of that...

   an alternative to increasing BLOWUP until the code won't break is
   using alarm() and periodically check for boundary overwritings in
   here...but that was too tedious for me at the time ;) (and anyways
   you have to calibrate the alarm timer and all...) */
#define BLOWUP 5
#define TESTCHAR '\21'

/* the array with the info about all mallocs (AS_Mallocs and AS_Strdups)
*/
typedef struct {
	void *ptr;
	int size;
	int line;
	char *file;
} memblockDesc;

static memblockDesc m[MALLARRSZ];

/* the position until which the array is filled */
static int m_pos = 0;

void AS_InitMem()
{
	bzero(m,sizeof(m));
}

/*
modified malloc routine that is used instead of standard malloc

  blowup every malloc by a factor and initialize with magic value;
  either the @see checkmem routine or @see free, also called by every
  AS_Malloc1, will first check that the magic has not been
  overwritten; If it does, then we know that some one has written
  randomly into memory; Essentially, a poor man's electric fence.

  we also record line# and filename for every malloc, so to tackle the
  offended malloc as close as possible

  @param size size of requested block
  @param line line number of malloc (__LINE__)
  @param file filename (__FILE__)

  @returns memory or NULL if unsuccessful
  */

void * AS_Malloc1(const size_t size, const int line, char  *file)
{
	void *ptr;
	char *cptr;
	int i;

	AS_CheckMem();
	if(size==0) {
		fprintf(stderr, "allocating 0 bytes? strange ...\n");
		fflush(stderr);
	}

	if ((ptr = malloc(BLOWUP*size)) == NULL) {
		fprintf(stderr, "malloc failed (blowup %d)\n", BLOWUP);
		fflush(stderr);
		return NULL;
	}

	cptr = (char*)ptr;

	/* write TESTCHAR in excess memory */
	for (i=size; i<BLOWUP*size; i++)
		cptr[i] = TESTCHAR;

	/* insert in array: if there's a NULL entry between 0 and m_pos than
		     there was a AS_free for this spot and we can reuse it */
	for (i=0; i<m_pos; i++)
		if (m[i].ptr == NULL) {
			m[i].ptr = ptr;
			m[i].size = size;
			m[i].line=line;
			m[i].file=file;
			return ptr;
		}
	/* otherwise increase m_pos */
	m[m_pos].ptr = ptr;
	m[m_pos].size = size;
	m[m_pos].line=line;
	m[m_pos].file=file;
	m_pos++;

	/* don't cause a memory bug in the memory debugging code ;-) */
	if (m_pos==MALLARRSZ) {
		fprintf(stderr, "AS_Malloc: ran out of array space\n");
		fflush(stderr);
		return NULL;
	}
	return ptr;
}


/**
  checking version of free().

  we check that no previous Malloc has been overwritten and that we
  are freeing something that has been malloced before. Since we are
  using GC this is now somewhat superfluous :-)

  @param ptr pointer to be freed
  @returns nothing
 */
void
AS_Free1(void *ptr, const int line, const char *file)
{
	char *cptr;
	int i,j,error=0;

	if (!ptr) {
		fprintf(stderr,"Trying to free NULL pointer! (%s,%d)\n",file,line);
		fflush(stderr);
		return;
	}

	/* find ptr in array */
	for (i=0; i<m_pos; i++) {

		if (m[i].ptr == ptr) {

			cptr = (char*)ptr;

			/* detection of 1st possible malloc bug: check for malloc boundary
						
						         violation in excess memory */
			for (j=m[i].size; j<BLOWUP*m[i].size; j++) {
				if (cptr[j] != TESTCHAR) {
					fprintf(stderr,"memwrite at index %d, size %d, BLOWUP %d (value %c), " \
       "allocated at %s:%d\n",
					    j,m[i].size,BLOWUP,cptr[j],m[i].file,m[i].line);
					fflush(stderr);
					error=1;
					break;
				}
			}
			if(error==1) {
				return;
			}
			bzero(cptr,BLOWUP*m[i].size);
			m[i].ptr = NULL;
			m[i].size=-1;
			free(ptr);
			return;
		}
	}
	/* detection of 2nd possible malloc bug: ptr wasn't found in
		     array...what are you freeing, man? */
	fprintf(stderr, "AS_free: freeing something that hasn't been malloc'd"
	    " or freed before! (%s,%d)\n",file,line);
	fflush(stderr);
}

/**
   give a dump of the memory allocations
*/
void AS_DumpMem()
{
	int i;

	for (i=0; i<m_pos; i++) {
		/* detection of 1st possible malloc bug: check for malloc boundary
		       violation in excess memory */
		if(m[i].ptr) {
			fprintf(stderr, "m[%d]=0x%p,size = %d, line = %u, file = %s\n", 
			    i, m[i].ptr, m[i].size, m[i].line, m[i].file?m[i].file:"UNDEF");
			fflush(stderr);
		}
	}
}


/**
  check that no write in previously allocated blocks has happened
  useful to find out-of-bounds array accesses and such.
  @returns nothing, but will abort if error is diagnosed
  */
void AS_CheckMem()
{
	int i,j;
	char *cptr;

	for (i=0; i<m_pos; i++) {
		cptr=(char *) m[i].ptr;
		if(cptr==NULL)
			continue;
		for (j=m[i].size; j<BLOWUP*m[i].size; j++) {
			if (cptr[j] != TESTCHAR) {
				fprintf(stderr, "memwrite at index %d, size %d, BLOWUP %d (value '%d'), " \
     "allocated at %s:%d\n",
				    j,m[i].size,BLOWUP,(int)cptr[j],m[i].file,m[i].line);
				fprintf(stderr, "out-of-bounds error\n");
				fflush(stderr);
				abort();
			}
		}
	}
}
