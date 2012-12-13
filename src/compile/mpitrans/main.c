# include <stdio.h>
# include <time.h>
# include <stdlib.h>
# include <string.h>
# include <math.h>
# include "global.h"
# include "mpitrans.h"
# include "symbol.h"

FILE *INS; /* *.ins */
FILE *INIT; /* thrUserInit.c */
FILE *MAIN; /* thrMain.c */
char prefix[9]="_G000000";
int randomize=0;

# define MAIN_INIT_ROUTINE "tmpi_prog_main_init"
# define USER_INIT_ROUTINE "tmpi_prog_user_init"

StrBuf *symnames;

char filename[MAX_FILENAME+1]=""; /* source file name, need to parse #file directive */
char insfname[MAX_FILENAME+1];
char userFile[100][MAX_FILENAME+1]; /* currently only one file */

int LEVEL=0;
STG_CLS last_stg_cls=NONE;
Bool look_for_tag=FALSE;
Bool parse_attrib_list=FALSE;
static char *ofilename=NULL;
static char *welcome="Thanks for using MPI Translator!\n\t--by Tang Hong, ";
static char *date="04/11/2000 Version 1.2, All rights reserved.";
static char mainfilename[MAX_FILENAME+1];
static char initfilename[MAX_FILENAME+1];
static char id_suffix[101];

int yyparse();

void yerror(char *msg)
{
	fprintf(stderr, "%s\n", msg);
}

int cmdline_parse(argc,argv)
int argc;
char *argv[];
{
	int i=0;
	time_t tmpt;
	char 	*today;
	int j;

	if (argc==1)
		return 0;

	while (--argc) {
		if (argv[++i][0]=='-') {
			switch (argv[i][1]) {
			case 'h':
			case 'H': return 0; /* do_help */
			case 'r':
			case 'R': 
				if (!--argc) return 0;
				randomize=atoi(argv[++i]);
				break;
			case 'e':
			case 'E': 
				break;
			case 'i':
			case 'I': 
				if (!--argc) return 0;
				strncpy(id_suffix, argv[++i], 100);
				break;
			case 'o':
			case 'O': if (!--argc) return 0;
				ofilename=argv[++i];
			 	break;
			case 'f':
			case 'F': 
				j=0;
				do {
					if (!--argc) return 0;
					strncpy(userFile[j], argv[++i], MAX_FILENAME);
					j++;
				} while (argv[i+1] && argv[i+1][0]!='-');
			 	break;
			case '\0': 
			case '-' : 
				if (strlen(filename)) {
					yerror("source file specified!");
					return 0;
				}
				else
					strcpy(filename,"-");
				break;
			default: return 0;
			}
		}
		else {
			if ( strlen(filename) && (ofilename) ) {
				yerror("Source and destiny files have been specified already!");
				return 0;
			}
			if (strlen(filename))
				ofilename=argv[i];
			else
				strncpy(filename,argv[i],MAX_FILENAME);
		}
	}
	if (!strlen(filename)) {
		yerror("No source file specified!");
		return 0;
	}
	if (filename[0]=='-' && filename[1]=='\0')  {
		yyin=stdin;
		strcpy(filename,"$STDIN$");
	}
	else if ((yyin = fopen (filename,"r")) == NULL) {
		yerror("Cannot open source file!\n");
		return -1;
	}

	strcpy(mainfilename, filename);
	strcat(mainfilename, ".thrMain.c");
	strcpy(initfilename, filename);
	strcat(initfilename, ".thrUserInit.c");

	if ( (ofilename==NULL) || ((ofilename[0]=='-') && (ofilename[1]=='\0')) )
		yyout=stdout;
	else if ((yyout = fopen (ofilename,"w")) == NULL) {
		yerror("Cannot open destiny file!\n");
		return -1;
	}
	tmpt=time(NULL);
	today=ctime(&tmpt);
	today[strlen(today)-1]='\0';
	fprintf(yyout,"/* MPItrans processed. Date : %s */\n",today);
	fprintf(yyout,"int thrMPI_createkey(void);\n");
	fprintf(yyout,"void thrMPI_setval(int, void *);\n");
	fprintf(yyout,"void *thrMPI_getval(int);\n");

	if (strlen(userFile[0])==0)
		strcpy(userFile[0], filename);

	return 1;
}

void do_help()
{

	fprintf(stderr, "Usage:\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"   1) mpitrans -h\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"        Print this screen.\n");
	fprintf(stderr,"OR:\n");
	fprintf(stderr,"   2) mpitrans [options] source [output]\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"        in which, source cannot be omitted. One can use \"-\" to specify\n");
	fprintf(stderr,"        stdin/stdout as source/output. Options can appear anywhere in \n");
	fprintf(stderr,"        the command (not just before the source).\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"   [options] could be: \n");
	fprintf(stderr,"   \n");
	fprintf(stderr,"   [-o output] : specify main output file. -o is not needed if the\n");
	fprintf(stderr,"        output is followed immediately after the source. By default,\n");
	fprintf(stderr,"        the output is stdout, which is equivalent to -o \"-\".\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"   [-r random] : specify a random number that will be embedded into\n");
	fprintf(stderr,"        mpitrans generated symbols (to avoid name collision).  This is\n");
	fprintf(stderr,"        useful if we need to process multiple files. By default, mpitrans\n");
	fprintf(stderr,"        will generate a random number by itself.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"   [-i index] : specify an index number for the main initialization\n");
	fprintf(stderr,"        and per thread initialization routines (so that\n");
	fprintf(stderr,"        %s becomes %s<i> and %s becomes\n", MAIN_INIT_ROUTINE, MAIN_INIT_ROUTINE, USER_INIT_ROUTINE);
	fprintf(stderr,"        %s<i>). This is needed if we have multiple files. The\n", USER_INIT_ROUTINE);
	fprintf(stderr,"        user also need to create their own version of %s()\n", MAIN_INIT_ROUTINE);
	fprintf(stderr,"        and %s() which just calls %s<i>s and\n", USER_INIT_ROUTINE, MAIN_INIT_ROUTINE);
	fprintf(stderr,"        %s<i>s.\n", USER_INIT_ROUTINE);
	fprintf(stderr,"\n");
	fprintf(stderr,"   [-f <file list>] : Files included (directly or indirectly) from\n");
	fprintf(stderr,"        the source file. By default, mpitrans ignores all the included\n");
	fprintf(stderr,"        files. So this list should put all the header files that contain\n");
	fprintf(stderr,"        global variable declarations and references.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"   [-e] : file list terminator. -f <file list> will try to absorb as\n");
	fprintf(stderr,"        many file names following -f as possible.  So one cannot put the\n");
	fprintf(stderr,"        source immediately after -f <file list> (otherwise, it will be\n");
	fprintf(stderr,"        considered as another file in the file list).\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"After processing a file called foo.c, three more files will be genreated\n");
	fprintf(stderr,"besides the output file. They are called foo.ins, foo.thrMainInit.c and\n");
	fprintf(stderr,"foo.thrUserInit.c. They are included from the output file. So another pass\n");
	fprintf(stderr,"of \"cc -E <output file>\" can consolidate the four files into one file.\n");

	return;
}
	
void init()
{
	char *program[]={
		"{",
		"	/* allocate memory for external keys */",
		NULL
	};
	char *program2[]={
		"{",
		NULL
	};
	int i;
	char *pc=filename;

	/* init prefix */
	if (randomize==0) {
		while (*pc) {
			randomize+=*pc;
			pc++;
		}
		srand((int)time(NULL)+randomize);
		randomize=rand()%1000000;
	}
	i=7;
	while (randomize) {
		prefix[i]='0'+randomize%10;
		randomize/=10;
		i--;
	}
	/* init INS */
	strcpy(insfname, filename);
	strcat(insfname, ".ins");
	INS=fopen(insfname,"w");
	fprintf(INS, "%s%s%s",
		"/* Generated by mpiTrans, for file ",
		filename,
		". This file is to be included in the main file. */\n");

	INIT=fopen(initfilename,"w");
	fprintf(INIT, "%s%s%s%s",
		"/* Generated by mpiTrans, for file ",
		filename,
		". */\n",
		"/* Do NOT compile! This file is not an independent C file. */\n");
	i=0;
	fprintf(INIT, "void %s%s(void)\n", USER_INIT_ROUTINE, id_suffix);
	while (program[i])  {
		fprintf(INIT, "%s\n", program[i]);
		i++;
	}

	MAIN=fopen(mainfilename,"w");
	i=0;
	fprintf(MAIN, "void %s%s(void)\n", MAIN_INIT_ROUTINE, id_suffix);
	while (program2[i])  {
		fprintf(MAIN, "%s\n", program2[i]);
		i++;
	}
	/**
	gvnames=new_strbuf();
	lvnames=new_strbuf();
	**/
	symnames=new_strbuf();
	symtab_init();

	return;
}

void cleanup()
{
	fprintf(yyout,"\n# include \"%s\"\n",initfilename);
	fprintf(yyout,"\n# include \"%s\"\n", mainfilename);
	fprintf(INIT,"}\n");
	fprintf(MAIN,"}\n");
	fprintf(INS, "#undef %s\n", prefix);
}

int main(argc,argv)
int argc;
char *argv[];
{
# if YYDEBUG
	extern yydebug;

	yydebug = 1;
# endif
	printf("%s%s\n",welcome,date);

	switch (cmdline_parse(argc,argv)) {
	case -1 : exit(-1);
	case 0  : do_help(); return 0;
	default :
		init();
		if (yyparse() || errmsgno) {
			cleanup(); 
			exit(-1);
		}
		cleanup(); 
		return 0;
	}
}
