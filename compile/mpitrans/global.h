# ifndef _GLOBAL_H_
# define _GLOBAL_H_
# include "mytypes.h"
# include "misc.h"
# include "hash.h"
# include "debug.h"
# if (DEBUGMEM)
# include "asmalloc.h"
# define malloc(size) as_malloc(size)
# define free(mem) as_free(mem)
# endif
/* declaration of all global variables */

extern char prefix[]; /* _G<random> */
# define MAX_FILENAME 127
extern char filename[]; /* main.c */
extern char userFile[][MAX_FILENAME+1]; /* main.c */
extern char insfname[]; /* main.c */
extern int errmsgno; /* misc.c */
extern int type_counter; /* misc.c */
extern int insert_counter; /* misc.c */

extern StrBuf *symnames; /* main.c */

extern FILE *yyin, *yyout; /* bison */
extern FILE *INS, *INIT, *MAIN; /* main.c */

char *token();
unsigned lineno();

# define SYMSTR(name) sb_string(symnames,name)

/* nested level, both for indention purpose and */
/* to distinguish global def/decl from nested decls */
extern int LEVEL; /* main.c  */
extern STG_CLS last_stg_cls; /* main.c  */
extern Bool look_for_tag; /* main.c  */
extern Bool parse_attrib_list; /* main.c  */

# define ISGVDEF ((LEVEL==0) && (last_stg_cls!=TYPEDEF))
# define ISLVDEF ((LEVEL!=0) && (last_stg_cls!=TYPEDEF))
# define ISTYPEDEF (last_stg_cls==TYPEDEF)
# define isGlobalDecl ( ((LEVEL==0)&&(last_stg_cls==NONE))||\
	(last_stg_cls==EXTERN)||(last_stg_cls==STATIC) )

extern HASH_TAB *sym_tab; /* symbol.c */

# endif
