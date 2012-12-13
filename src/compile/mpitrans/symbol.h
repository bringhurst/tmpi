# ifndef _SYMBOL_H_
# define _SYMBOL_H_
# include "debug.h"
# include "mytypes.h"
# include "misc.h"

/* Symbol.h: definitions for the top level variable symbol table and local 
 *		variable symbol table.
 */

# define MAX_SYM_LEN 127

/* eventually I have to use one structure for GV, LV, TDEF, etc */
typedef struct SYM {
  struct SYM *next; /* link to the next SYM */
  STR *decltor; /* lexical representation for a declarator */
  /* the name is allocated from a contiguous buffer: name -> namebuffer+name*/
  /* use macro SYMSTR(name) to retrieve the string */
  /* short name; */ /* if not a typedef name, func name or a variable name, name=-1 */ 
# if (DEBUGSB)
  char *name; 
# else
  short name;
# endif
  short LEVEL; /* the place the declaration occurs */
  Bool isFunc; 
  /* specifically only functions declared in the following way 
   * need to set the isConst flag to be TRUE:
   *
   * typedef int Func();
   * Func func;
   * 
   * However, we set this flag to be true for all functions later.
   */
  Bool isConst; /* simple constant or function */
  STG_CLS stg_cls; 
  STR *initstr; /* initialization */
  /* if not global variable, type and wrap_type will not be used */
  /* type and wrap_type may be merged as one */
  short type; /* type of the variable */
  short wrap_type; /* type of the wrapper struct */ 
  Bool isDup; /* isDup is set when inserting the sym to sym_tab. It is */
		  /* FALSE when it is inserted or TRUE otherwise. Used to 		*/
		  /* determine whether to do delsym or not. 				*/
} SYM;

# define isGlobal(s) ((s) && (((s)->stg_cls==EXTERN) \
	|| ((s)->stg_cls==STATIC) || (((s)->stg_cls==NONE)&&((s)->LEVEL==0))))
# define isNewSYM(s) ((s)&&((s)->stg_cls==ERR))
# define isValidSYM(s) ((s)&&(s->stg_cls!=ERR))

/**
 * all global variables used in a function, i.e. all variables that are not 
 * apperad in the local variable declaration or are declared as extern */
typedef struct LGL { /* local global var list */
  SYM *ref;
  struct LGL *next;
} LGL;

SYM * new_SYM (char *name, int LEVEL);
LGL * new_LGL (SYM *sym);
void discard_SYM (SYM *sym); 
void discard_SYM_chain (SYM *sym); 
void discard_LGL_chain ();
SYM *add_SYMs_to_table(SYM *syms);
void add_SYM(SYM *sym);
void init_localstatic(SYM *);
void sym_init(SYM *, char *);
SYM *reverse_symlink(SYM *sym);
void dumpSYMlist(SYM *sym);
void lgvw_gen();
void symtab_init();
# endif
