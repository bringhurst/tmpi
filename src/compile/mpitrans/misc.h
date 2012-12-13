# ifndef _MISC_H_
# define _MISC_H_
# include "debug.h"

# define STACK_SIZE 126
typedef struct _stack {
	int thestk[STACK_SIZE];
	int limit;
	int top;
} Stack;

Stack *new_stack();

int	stk_pop(Stack *stk);
void	stk_push(Stack *stk,int elem);

/* for symbol names */
# define STRBUF_SIZE ((unsigned int)65000)
typedef struct _strbuf {
	Stack *stk;
	char buf[STRBUF_SIZE];
	unsigned short current;
} StrBuf;

StrBuf *new_strbuf();
void sb_newframe(StrBuf *sb);
# if (DEBUGSB)
char *sb_alloc(StrBuf *sb, int size);
# else
short sb_alloc(StrBuf *sb, int size);
# endif
void sb_restore(StrBuf *sb);
# if (DEBUGSB)
char *sb_string(StrBuf *sb, char * offset);
/* # define sb_string(sb, offset) (offset) */
# else
char *sb_string(StrBuf *sb, int offset);
/* # define sb_string(sb, offset) ((sb)->buf+(offset)) */
# endif

# if (NOSTR)
typedef char STR;
# else
/* for small token (char*) attributions */
# define STRBLK ((unsigned int)128)
typedef struct _str {
	char *namex;
	struct _str *next;
	short maxlen;
	short curlen; /* must always be greater than 0 */
# if (DEBUG)
	unsigned isFree; /* debug */
# endif
} STR;

# endif

/* constructors */
STR *new_blankstr(); /* return an STR with buffer size >=STRBLK */
STR *new_str(char *s);

/* deconstructor */
void del_str(STR *str);

STR *str_append(STR *str, char *s); /* no deallocation for s */
STR *str_cat(STR *str1, STR *str2); /* no deallocation for s */
/* # define str_string(str) ((str)->namex) */
char * str_string(STR *str);

void yyerror();
void emitCode();
void lgv_include();
void gv_createkey(char *);

char *make_typedef(int typecnt); /* increment type_counter */
char *make_gvkey(char *gvname);
char *make_lgvw(char *gvname);

void setIndent(int);
void strIndent(STR *, int);
# endif
