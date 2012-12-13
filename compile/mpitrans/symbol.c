#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "hash.h"
#include "symbol.h"

HASH_TAB * sym_tab; /* the symbol table */

/* all local variables referenced in current function */
static LGL *lglist=NULL; 

static SYM *freesymlist = NULL; /* Free-list of recycled GV struct */
static LGL *freelgl = NULL; /* Free-list of recycled LV struct */

int cmp(void *s1, void *s2)
{
	return (strcmp((char *)s1, (char *)s2));
}

void *sym_get_key(void *sym)
{
	return (void *)SYMSTR(((SYM *)sym)->name);
}

void symtab_init()
{
	sym_tab=maketab(1021, hash_add, sym_get_key, cmp);
}

SYM * new_SYM (char *name, int LEVEL)
{
	SYM * symp;

	if (freesymlist) {
		symp=freesymlist;
		freesymlist=freesymlist->next;
	}
	else {
		symp=(SYM *)newsym(sizeof(SYM));
	}

	symp->name=sb_alloc(symnames, strlen(name)+1);
	strcpy(SYMSTR(symp->name), name);
	symp->LEVEL=LEVEL;
	symp->decltor=NULL;
	symp->stg_cls=ERR;
	symp->isFunc=UNKNOWN;
	symp->isConst=UNKNOWN;
	symp->initstr=NULL;
	symp->next=NULL;

	return symp;
}

/**
 * if the gv is not seen before, create a new item in lglist, otherwise
 * don't do any thing. Return the lglist item that refers to the gv. 
 */
LGL * new_LGL (SYM *sym)
{
	LGL *lglp=lglist;

	while (lglp) {
		if (sym==lglp->ref)
			return lglp;
		lglp=lglp->next;
	}
	if (freelgl) {
		lglp=freelgl;
		freelgl=freelgl->next;
	}
	else
		lglp=(LGL *)malloc(sizeof(LGL));

	lglp->ref=sym;
	lglp->next=lglist;
	lglist=lglp;

	return lglp;
}

void discard_SYM (SYM *symp)
{
	/* Just in case */
	if (symp->decltor) {
		del_str(symp->decltor);
		symp->decltor=NULL;
	}

	if (symp->initstr) {
		del_str(symp->initstr);
		symp->initstr=NULL;
	}

	symp->next=freesymlist;
	freesymlist=symp;

	return;
}

/* assuming head is not NULL */
void discard_LGL_chain ()
{
	LGL *lglp=lglist;

	if (lglp) { 
		while (lglp->next)
			lglp=lglp->next;
	
		lglp->next=freelgl;
		freelgl=lglist;
		lglist=NULL;
	}

	return;
}

/**
 * make the keys for those global variable accessed implicitly 
 * in the current function.
 * Right after the function header, before all variable declarations.
 */
void lgvw_gen()
{
	LGL *lglp=lglist;
	SYM *sym;
	char *symname;

      fprintf(INS, "# if (%s==%d)\n", prefix, insert_counter++);
	while (lglp) {
		sym=lglp->ref;
		symname=SYMSTR(sym->name);
		fprintf(INS, "\t%s *%s=thrMPI_getval(%s);\n",
			make_typedef(sym->type), make_lgvw(symname), make_gvkey(symname));
		lglp=lglp->next;
	}
      fprintf(INS, "# endif\n\n");
	discard_LGL_chain();
	return;
}

void add_SYM(SYM *sym)
{
	SYM *exists;

	exists=(SYM *)findsym(sym_tab, SYMSTR(sym->name));
	if (!exists || (exists->LEVEL!=sym->LEVEL)) {
		addsym(sym_tab, sym);
		sym->isDup=FALSE;
	}
	else
		sym->isDup=TRUE;

	
	return;
}

/**
 * The following is only used for adding the parameter_list currently.
 * So duplication is not allowed.
 */

SYM *add_SYMs_to_table(SYM *syms)
{
 	SYM *exists;	/* Existing symbol if there's a conflict */
 	SYM *new=syms;
	
	while (new) {
		exists = (SYM *)findsym(sym_tab,SYMSTR(new->name));
		if (exists && (exists->LEVEL==syms->LEVEL)) { 
			yyerror("Warning: duplicate declarations of %s.", SYMSTR(new->name));
		}

		addsym(sym_tab, new);
		new->isDup=FALSE; /* another dirty solution */
		new=new->next;
 	}

	return syms;
}

/***
SYM *add_SYMs_to_table(SYM *syms)
{
 	SYM *exists;	
 	SYM *new=syms, *previous=NULL;
	
	while (new) {
		exists = (SYM *)findsym(sym_tab,SYMSTR(new->name));

		if (!exists || (exists->LEVEL!=syms->LEVEL)) { 
			addsym(sym_tab,new);
			previous=new;
			new=new->next;
		}
		else { 
			if (previous) {
				previous->next=new->next;
				new=previous->next;
				discard_SYM(new);
			}
			else {
				previous=new;
				syms=new=new->next;
				discard_SYM(previous);
				previous=NULL;
			}
		}
 	}

	return syms;
}
***/

SYM *reverse_symlink(SYM *sym)
{
 	SYM *previous,*current,*next;

	if (!sym)
		return NULL;
	 
	previous = sym;
	current = sym->next;

	while (current) {
		next 	= current->next;
		current->next = previous;
		previous = current;
		current = next;
 	}

	sym->next = NULL;
	return previous;
}

void discard_SYM_chain(SYM *sym)
{
	SYM *p = sym;
 
	while (sym) {
		p = sym->next;
		if (sym->isDup==FALSE)
			delsym(sym_tab, sym);
		discard_SYM(sym);
		sym=p;
	}
	return;
}

void init_localstatic(SYM *symlink)
{
	SYM *sym=symlink;

	while (sym) {
		if ( (sym->stg_cls==STATIC)&&(sym->LEVEL>0) && (sym->isFunc!=TRUE) && (sym->isConst!=TRUE) ) {
			char *symname;
			char symtype[1000];
			char *symlgvw; /* pointer to the TSD */
			char *symkey;

			symname=SYMSTR(sym->name);
			strncpy(symtype, make_typedef(sym->type), 1000);
			symlgvw=make_lgvw(symname);
			symkey=make_gvkey(symname);

			setIndent(LEVEL);
			emitCode("if (%s==-1) {", symkey);

			setIndent(LEVEL+1);
			emitCode("int tmpkey;");
			setIndent(LEVEL+1);
			emitCode("tmpkey=thrMPI_createkey();");
			setIndent(LEVEL+1);
			emitCode("__compare_and_swap(&%s, -1, tmpkey);", symkey);
			
			setIndent(LEVEL);
			emitCode("}");

			setIndent(LEVEL);
			emitCode("if ((%s=thrMPI_getval(%s))==0) {", symlgvw, symkey);

			if (sym->initstr) {
				setIndent(LEVEL+1);
				emitCode("%s tmp=%s;\n", symtype, str_string(sym->initstr));
			}
			setIndent(LEVEL+1);
			if (sym->initstr) 
				emitCode("%s=(%s *)malloc(sizeof(tmp));", symlgvw, symtype);
			else
				emitCode("%s=(%s *)malloc(sizeof(%s));", symlgvw, symtype, symtype);

			setIndent(LEVEL+1);
			emitCode("thrMPI_setval(%s, %s);", symkey, symlgvw);
		
			if (sym->initstr) {
				setIndent(LEVEL+1);
				emitCode("memcpy((char *)%s, (char *)&tmp, sizeof(tmp));",
					symlgvw, symname);
				del_str(sym->initstr);
				sym->initstr=NULL;
			}
			setIndent(LEVEL);
			emitCode("}");
		}

		sym=sym->next;
	}

	return;
}

/**
 * the reason I use two parameters intead of one is I will move the symbol init code
 * as soon as the declarator is fully parsed instead of waiting until the declaration
 * list is seen. This is important when the initialization string is extremely long
 * e.g. when initialize a large array.
 *
 * Do not init extern structure because the type info might be incomplete to get size.
 */
void sym_init(SYM *sym, char *initstr)
{
	char *symname;
	char *symname2;

	if (sym->stg_cls != EXTERN) {
		fprintf(INIT, "\tdo {\n");

		symname=make_typedef(sym->type);
		if (initstr)
			fprintf(INIT, "\t\t%s tmp=%s;\n", symname, initstr);
	
		symname2=make_gvkey(SYMSTR(sym->name));
	
		fprintf(INIT, "\t\t%s *tmp2;\n",symname);
		fprintf(INIT, "\n");
		fprintf(INIT, "\t\tif ((tmp2=thrMPI_getval(%s))==0) {\n", symname2);
		/* fprintf(INIT, "\t\t\ttmp2=(%s *)malloc(sizeof(%s));\n", symname, symname); */
		if (initstr)
			fprintf(INIT, "\t\t\ttmp2=(%s *)malloc(sizeof(tmp));\n", symname);
		else 
			fprintf(INIT, "\t\t\ttmp2=(%s *)malloc(sizeof(%s));\n", symname, symname);
		fprintf(INIT, "\t\t\tthrMPI_setval(%s, tmp2);\n", symname2);
		fprintf(INIT, "\t\t}\n");

		if (initstr) 
			fprintf(INIT, "\t\tmemcpy((char *)tmp2, (char *)&tmp, sizeof(tmp));\n");
		fprintf(INIT, "\t} while(0);\n");
	}
	return;
}

void dumpSYMList(SYM *symlist)
{
	int i=0;

	fprintf(stderr, "FALSE=0, TRUE=1, UNKNOWN=2\n");
	while (symlist) {
		fprintf(stderr, "%d.\t%s\tLEVEL=%d\tisFunc=%d\tisConst=%d\tstorage=", ++i, SYMSTR(symlist->name), symlist->LEVEL, symlist->isFunc, symlist->isConst);
		switch (symlist->stg_cls) {
		case EXTERN: fprintf(stderr, "extern"); break;
		case STATIC: fprintf(stderr, "static"); break;
		case TYPEDEF: fprintf(stderr, "typedef"); break;
		case AUTO: fprintf(stderr, "auto"); break;
		case REGISTER: fprintf(stderr, "register"); break;
		case NONE: fprintf(stderr, "<none>"); break;
		case ERR: fprintf(stderr, "<err>"); break;
		default: fprintf(stderr, "<undef>");
		}
		fprintf(stderr, "\n"); 
		symlist=symlist->next;
	}
	fflush(stderr);
}
