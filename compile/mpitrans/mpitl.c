# include <stdio.h>
# include "hash.h"
# include "symbol.h"
# include "misc.h"
# include "y.tab.h"

char *token();
YYSTYPE	yylval;
int LEVEL;
Bool look_for_tag=FALSE;
Bool parse_attrib_list=FALSE;
char filename[128];
STG_CLS last_stg_cls;
HASH_TAB *sym_tab;

void	*findsym(table,lexme)
HASH_TAB *table;
void *lexme;
{
	return NULL;
}

SYM * new_SYM(char *name, int LEVEL)
{
	return (SYM *)NULL;
}
void 	yyerror(str)
char *str;
{
	fprintf(stderr,"%s",str);
	return;
}

void disptok(str)
char *str;
{
	if (str)
		printf("%30s  %-47s\n",str,token());
	else printf("unknown token\t\t%s\n",token());

	return;
}

int yylex();

int main()
{
	int	token;
	while ((token=yylex())) {
		switch (token) {
 			case (TK_AUTO): disptok("auto"); break;
		 	case (TK_BREAK): disptok("break");break;
		  	case (TK_CASE): disptok("case");break;
		  	case (TK_CHAR): disptok("char");break;
		  	case (TK_CONST): disptok("const");break;
		  	case (TK_CONTINUE): disptok("continue");break;
		  	case (TK_DEFAULT): disptok("default");break;
		  	case (TK_DO): disptok("do");break;
		  	case (TK_DOUBLE): disptok("double");break;
		  	case (TK_ELSE): disptok("else");break;
		  	case (TK_ENUM): disptok("enum");break;
		  	case (TK_EXTERN): disptok("extern");break;
		  	case (TK_FLOAT): disptok("float");break;
		  	case (TK_FOR): disptok("for");break;
		  	case (TK_GOTO): disptok("goto");break;
		  	case (TK_IF): disptok("if");break;
		  	case (TK_INT): disptok("int");break;
		  	case (TK_LONG): disptok("long");break;
		  	case (TK_REGISTER): disptok("register");break;
		  	case (TK_RETURN): disptok("return");break;
		  	case (TK_SHORT): disptok("short");break;
		  	case (TK_SIGNED): disptok("signed");break;
		  	case (TK_SIZEOF): disptok("sizeof");break;
		  	case (TK_STATIC): disptok("static");break;
		  	case (TK_STRUCT): disptok("struct");break;
		  	case (TK_SWITCH): disptok("switch");break;
		  	case (TK_TYPEDEF): disptok("typedef");break;
		  	case (TK_UNION): disptok("union");break;
		  	case (TK_UNSIGNED): disptok("unsigned");break;
		  	case (TK_VOID): disptok("void");break;
		  	case (TK_VOLATILE): disptok("volatile");break;
		  	case (TK_WHILE): disptok("while");break;
		 	case (TK_IDENTIFIER): disptok("IDENTIFIER");break;
		 	case (TK_TYPE_NAME): disptok("TYPEDEF NAME");break;
 			case (TK_CONSTANT): disptok("CONSTANT");break;
		   	case (TK_STRING_LITERAL): disptok("STRING");break;
		  	case (TK_ELIPSIS): disptok("...");break;
		  	case (TK_RIGHT_ASSIGN): disptok(">>=");break;
		  	case (TK_LEFT_ASSIGN): disptok("<<=");break;
		  	case (TK_ADD_ASSIGN): disptok("+=");break;
		  	case (TK_SUB_ASSIGN): disptok("-=");break;
		  	case (TK_MUL_ASSIGN): disptok("*=");break;
		  	case (TK_DIV_ASSIGN): disptok("/=");break;
		  	case (TK_MOD_ASSIGN): disptok("%=");break;
		  	case (TK_AND_ASSIGN): disptok("&=");break;
		  	case (TK_XOR_ASSIGN): disptok("^=");break;
		  	case (TK_OR_ASSIGN): disptok("|=");break;
		  	case (TK_RIGHT_OP): disptok(">>");break;
		  	case (TK_LEFT_OP): disptok("<<");break;
		  	case (TK_INC_OP): disptok("++");break;
		  	case (TK_DEC_OP): disptok("--");break;
		  	case (TK_PTR_OP): disptok("->");break;
		  	case (TK_AND_OP): disptok("&&");break;
		  	case (TK_OR_OP): disptok("||");break;
		  	case (TK_LE_OP): disptok("<=");break;
		  	case (TK_GE_OP): disptok(">=");break;
		  	case (TK_EQ_OP): disptok("==");break;
		  	case (TK_NE_OP): disptok("!=");break;
		  	case (TK_EXTENSION): disptok("__extension__");break;
		  	case (TK_ATTRIBUTE): disptok("__attribute__");break;
		  	case (';'): disptok(";");break;
		  	case ('{'): disptok("{");break;
		  	case ('}'): disptok("}");break;
		  	case (','): disptok(",");break;
		  	case (':'): disptok(":");break;
		  	case ('='): disptok("=");break;
		  	case ('('): disptok("(");break;
		  	case (')'): disptok(")");break;
		  	case ('['): disptok("[");break;
		  	case (']'): disptok("]");break;
		  	case ('.'): disptok(".");break;
		  	case ('&'): disptok("&");break;
		  	case ('!'): disptok("!");break;
		  	case ('~'): disptok("~");break;
		  	case ('-'): disptok("-");break;
		  	case ('+'): disptok("+");break;
		  	case ('*'): disptok("*");break;
		  	case ('/'): disptok("/");break;
		  	case ('%'): disptok("%");break;
		  	case ('<'): disptok("<");break;
		  	case ('>'): disptok(">");break;
		  	case ('^'): disptok("^");break;
		  	case ('|'): disptok("|");break;
		  	case ('?'): disptok("?");break;
			default:disptok(NULL); printf("token=%d\n",token);
		}
	}
	printf("DONE!\n");
	return 0;
}
