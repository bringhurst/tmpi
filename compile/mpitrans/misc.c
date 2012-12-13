# include <stdio.h>
# include <varargs.h>
# include <stdlib.h>
# include "global.h"
# include "debug.h"
# include "misc.h"
# include "symbol.h"

int type_counter=0;
int insert_counter=0;
int errmsgno=0;

static FILE	*errfile=NULL;
static STR *freestrlist=NULL;

void	yyerror(va_alist)
va_dcl
{
	va_list	arglist;
	char	*errmsg;

	if (!errfile)
		errfile=stdout;

	va_start(arglist);
	errmsg=va_arg(arglist,char *);
	fprintf(errfile," ERR(%d):\tfile=%s\n\tline=%d\n\taround '%s' \n\t\t ",++errmsgno,filename,lineno(),token());
	vfprintf(errfile,errmsg,arglist);
	va_end(arglist);
	fprintf(errfile,"!\n");

	return;
}

# if (DEBUG)
static void stk_dbg_init(Stack *stk)
{
	int i=0;
	for (; i<stk->limit; i++)
		stk->thestk[i]=0;
}
# endif

Stack *new_stack()
{
	Stack *new_stk;

	new_stk=(Stack *)malloc(sizeof(Stack));
	new_stk->limit=STACK_SIZE;
	new_stk->top=0;
# if (DEBUG)
	stk_dbg_init(new_stk);
# endif

	return new_stk;
}

int stk_pop(Stack *stack)
{
	if (stack->top<=0) {
		yyerror("Stack underflow!");
		return 0;
	}

	return (stack->thestk[--(stack->top)]);
}

void stk_push(Stack *stack, int elem)
{
	if (stack->top>=stack->limit) {
		yyerror("Stack overflow!\n");
		return;
	}

	stack->thestk[(stack->top)++]=elem;
	return;
}

StrBuf *new_strbuf()
{
	StrBuf *newsb;

	newsb=(StrBuf *)malloc(sizeof(StrBuf));
	newsb->stk=new_stack();
	newsb->current=0;

	return newsb;
}

void sb_newframe(StrBuf *sb)
{
	stk_push(sb->stk, sb->current);
	return;
}

# if (DEBUGSB)

char * sb_alloc(StrBuf *sb, int size)
{
	return (char *)malloc(size);
}

char *sb_string(StrBuf *sb, char *offset)
{
	return offset;
}

# else
/* size is the strlen of a string, so add 1 */
short sb_alloc(StrBuf *sb, int size)
{
	short previous=sb->current;

	if ((sb->current+size+1)<STRBUF_SIZE) {
		sb->current+=size+1;
		return previous;
	}

	yyerror("StrBuf overflow!");
	return -1;
}

char *sb_string(StrBuf *sb, int offset)
{
	return sb->buf+offset;
}
# endif

void sb_restore(StrBuf *sb)
{
	sb->current=(unsigned short)stk_pop(sb->stk);
	return;
}

# if (NOSTR)
char *new_blankstr()
{
	char *str=malloc(1);
	str[0]='\0';
	return str;
}

char *new_str(char *s)
{ 
	char *str=malloc(strlen(s)+1);

	strcpy(str, s);

	return str;
}
    
void del_str(char *str)
{
	if (str)
		free(str);
}

char *str_append(char *str, char *s)
{
	char *newstr=malloc(strlen(str)+strlen(s)+1);
	strcpy(newstr, str);
	strcat(newstr, s);
	free(str);
	return newstr;
}

char *str_cat(char *str, char *s)
{
	char *newstr=malloc(strlen(str)+strlen(s)+1);
	strcpy(newstr, str);
	strcat(newstr, s);
	free(str);
	return newstr;
}
char * str_string(char *str)
{
	return str;
}

# else

# if (DEBUG)
static void str_dbg_init(char *str, int len)
{
	int i=0;
	for (; i<len; i++)
		str[i]='`';
}

static void str_dbg_check(STR *str)
{
	int i=str->curlen;
	for (; i<str->maxlen; i++)
		if (str->namex[i]!='`') {
			fprintf(stderr, "str debug checking failed\n");
			fflush(stderr);
		}
}
# endif

STR *new_blankstr() /* return an STR with buffer size >=STRBLK */
{
	STR *str;

	if (freestrlist && (freestrlist->maxlen>=STRBLK) ) {
		str=freestrlist;
		freestrlist=freestrlist->next;
# if (DEBUG)
		if (str->isFree==0) {
			fprintf(stderr, "Got an active STR in free list.\n");
			fflush(stderr);
		}
		str->isFree=0;
# endif

# if (DEBUG)
		str_dbg_init(str->namex, str->maxlen);
# endif
		str->curlen=1;
		(str->namex)[0]='\0';
	}
	else {
		str=(STR *)malloc(sizeof(STR));
		str->curlen=1;
		str->maxlen=STRBLK;
		str->namex=(char *)malloc(STRBLK);
# if (DEBUG)
		str_dbg_init(str->namex, str->maxlen);
# endif
		(str->namex)[0]='\0';
# if (DEBUG)
		str->isFree=0;
# endif
	}
# if (DEBUG)
	str_dbg_check(str);
# endif
	return str;
}

STR *new_str(char *s)
{
	STR *str;

	if (!s) return new_blankstr();

	if (freestrlist) {
		str=freestrlist;
		freestrlist=freestrlist->next;
# if (DEBUG)
		if (str->isFree==0) {
			fprintf(stderr, "Got an active STR in free list.\n");
			fflush(stderr);
		}
		str->isFree=0;
# endif
		str->curlen=strlen(s)+1;
		if (str->curlen>str->maxlen) {
			free(str->namex);
			str->maxlen=(str->curlen+STRBLK-1)/STRBLK*STRBLK;
			str->namex=(char *)malloc(str->maxlen);
		}
	}
	else {
		str=(STR *)malloc(sizeof(STR));
		str->curlen=strlen(s)+1;
		str->maxlen=(str->curlen+STRBLK-1)/STRBLK*STRBLK;
		str->namex=(char *)malloc(str->maxlen);
# if (DEBUG)
		str->isFree=0;
# endif
	}

# if (DEBUG)
		str_dbg_init(str->namex, str->maxlen);
# endif

	strcpy(str->namex, s);

# if (DEBUG)
	str_dbg_check(str);
# endif
	return str;
}

void del_str(STR *str)
{
	if (!str)
		return;

# if (DEBUG)
	if (!(str->isFree)) {
		str->isFree=1;
# endif
		str->next=freestrlist;
		freestrlist=str;
# if (DEBUG)
	}
	else {
		fprintf(stderr, "internal error, attmpt to delete a deleted str\n");
		fflush(stderr);
	}
# endif
	return;
}

STR *str_append(STR *str, char *s)
{
	char *ss;
	int size;
	int maxlen;

# if (DEBUG)
	if (str->isFree==1) {
		fprintf(stderr, "Use of a nonactive STR with str_append.\n");
		fflush(stderr);
		str->isFree=0; /* avoid complaining next time */
	}
# endif
# if (DEBUG)
	str_dbg_check(str);
# endif
	if (!s) return str;

	size=strlen(s)+str->curlen; /* move '\0' around */
	if (str->maxlen<size) {
		maxlen=(size+STRBLK-1)/STRBLK*STRBLK;
		ss=(char *)malloc(maxlen);
# if (DEBUG)
		str_dbg_init(ss, maxlen);
# endif
		strcpy(ss, str_string(str));
		strcpy(ss+str->curlen-1, s);
		free(str->namex);
		str->namex=ss;
		str->maxlen=maxlen;
		str->curlen=size;
	}
	else {
		strcpy(str->namex+(str->curlen-1), s);
		/* strcat(str->namex, s); */
		str->curlen=size;
	}
	
# if (DEBUG)
	if (strlen(str_string(str))>=str->maxlen) {
		fprintf(stderr, "internal error in str_append\n");
		fflush(stderr);
	}
# endif
# if (DEBUG)
	str_dbg_check(str);
# endif
	return str;
}

STR *str_cat(STR *str1, STR *str2)
{
	char *ss;
	int size;
	int maxlen;
# if (DEBUGMEM)
	static int count=0;
	static int freecount=0;
# endif

# if (DEBUGMEM)
	fprintf(stderr, "count=%d\n", count++);
	fflush(stderr);
# endif

# if (DEBUG)
	if ( (str1->isFree==1) || (str2->isFree==1) ) {
		fprintf(stderr, "Use of a nonactive STR with str_cat.\n");
		fflush(stderr);
		str1->isFree=0; /* avoid complaining next time */
		str2->isFree=0; /* avoid complaining next time */
	}
# endif
	
# if (DEBUGMEM)
	if (count==601) {
		int bug=0;
		bug++;
	}
# endif

	if (!str2) {
# if (DEBUG)
	str_dbg_check(str1);
# endif
		return str1;
	}
	size=str1->curlen+str2->curlen-1; /* move '\0' around */
	if (str1->maxlen<size) {
		maxlen=(size+STRBLK-1)/STRBLK*STRBLK;
		ss=(char *)malloc(maxlen);
# if (DEBUG)
		str_dbg_init(ss, maxlen);
# endif
		strcpy(ss, str_string(str1));
		strcpy(ss+str1->curlen-1, str_string(str2));
		/* strcat(ss, str_string(str2)); */
# if (DEBUGMEM)
		fprintf(stderr, "freecount=%d, pointer=%x\n", freecount++, str1->namex);
		fflush(stderr);
# endif
		free(str1->namex);
		str1->namex=ss;
		str1->maxlen=maxlen;
		str1->curlen=size;
	}
	else {
		strcpy(str1->namex+str1->curlen-1, str_string(str2));
		str1->curlen=size;
	}	

# if (DEBUG)
	if (strlen(str_string(str1))>=str1->maxlen) {
		fprintf(stderr, "internal error in str_append\n");
		fflush(stderr);
	}
# endif
# if (DEBUG)
	str_dbg_check(str1);
# endif
	return str1;
}

char *str_string(STR *str)
{
# if (DEBUG)
	if (str->isFree==1) {
		fprintf(stderr, "Attempting to reference a nonactive STR from str_string.\n");
		fflush(stderr);
	}
	else {
# if (DEBUGMEM)
		AS_CheckMem();
		str_dbg_check(str);
# endif
	}
# endif
	return str->namex;
}

# endif
char *make_typedef(int typecnt)
{
	static char name[128];
	sprintf(name, "%sTYPE%d", prefix, typecnt);
	return name;
}
	
char *make_gvkey(char *gvname)
{
	static char name[128];
	sprintf(name, "%sKEY%s", prefix, gvname);
	return name;
}
	
char *make_lgvw(char *gvname)
{
	static char name[128];
	sprintf(name, "%sLGVW%s", prefix, gvname);
	return name;
}

void gv_createkey(char *gvname)
{
	/* assuming all global data are implicitly initialized to zero */
	fprintf(MAIN, "\tif (%s==0)\n", make_gvkey(gvname));
	fprintf(MAIN, "\t\t%s=thrMPI_createkey();\n", make_gvkey(gvname));
}


void lgv_include()
{
	fprintf(yyout, "\n# define %s %d\n", prefix, insert_counter);
	fprintf(yyout, "# include \"%s\"\n", insfname);
}

void emitCode(va_alist)
va_dcl
{
	va_list arglist;
	char *format;

	va_start(arglist);
	format=va_arg(arglist, char *);
	vfprintf(yyout, format, arglist);
	va_end(arglist);
# if (DEBUG)
	fflush(yyout);
# endif
	return;
}

void strIndent(STR *str, int indent)
{
	int i;

	str_append(str, "\n");
	for  (i=0;i<indent;i++)
		str_append(str, "\t");
}

void setIndent(int indent)
{
	int i;

	emitCode("\n");
	for  (i=0;i<indent;i++)
		emitCode("\t");
}
