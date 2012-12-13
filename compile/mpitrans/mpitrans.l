/* digits */
D                       [0-9]
/* letters */
L                       [a-zA-Z_$]
/* hexical */
H                       [a-fA-F0-9]
/* exponent */
E                       [Ee][+-]?{D}+
/* ??? */
FS                      (f|F|l|L)
/* int suffix */
IS                      (u|U|l|L)*
/* white space */
LWSC                    [ \t\v\f]
%{
/**
 * Based on Paul Long's Metre C lexer. 
 * All uses yytext to pass the attribution except for 
 * TK_IDENTIFER 
 */
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include "global.h"
# include "misc.h"
# include "hash.h"
# include "symbol.h"
# include "mpitrans.h"
# include "y.tab.h"

static char *extract_file_name(char *string);
static void comment(void);
int yywrap();
static int check_type();
char *token(void);
unsigned lineno(void);
static unsigned extract_line_number(char *string);
%}

%option yylineno

%%
"/*"	{ comment();   /* Read in rest of comment. */ }
^{LWSC}*#[ \t]*("line"[ \t]+)?{D}+([ \t]+\"[^"\n]*\")?.*  {
		/* found something like 
		 * # 100 "file1.c" 
		 * or
		 * # line 100 "file1.c"
		 */
		char *temp_file_name;

		/* Don't know why had to subtract 1. Oh well. */
		yylineno = extract_line_number(yytext) - 1;

		/* Use new file name if present. */
		temp_file_name = extract_file_name(yytext);
		if (temp_file_name != NULL)
			strncpy(filename, temp_file_name, MAX_FILENAME);
	}

"auto"                  {  
		/* keywords */
		return(TK_AUTO); 
	}
"break"                 { return(TK_BREAK); }
"case"                  {  return(TK_CASE); }
"char"                  {  return(TK_CHAR); }
"const"                 {  return(TK_CONST); }
"__const"                 {  return(TK_CONST); }
"continue"              {  return(TK_CONTINUE); }
"default"               {  return(TK_DEFAULT); }
"do"                    {  return(TK_DO); }
"double"                {  return(TK_DOUBLE); }
"else"                  {  return(TK_ELSE); }
"enum"                  {  return(TK_ENUM); }
"extern"                {  return(TK_EXTERN); }
"float"                 {  return(TK_FLOAT); }
"for"                   {  return(TK_FOR); }
"goto"                  {  return(TK_GOTO); }
"if"                    {  return(TK_IF); }
"int"                   {  return(TK_INT); }
"long"                  {  return(TK_LONG); }
"register"              {  return(TK_REGISTER); }
"return"                {  return(TK_RETURN); }
"short"                 {  return(TK_SHORT); }
"signed"                {  return(TK_SIGNED); }
"sizeof"                {  return(TK_SIZEOF); }
"static"                {  return(TK_STATIC); }
"struct"                {  return(TK_STRUCT); }
"switch"                {  return(TK_SWITCH); }
"typedef"               {  return(TK_TYPEDEF); }
"union"                 {  return(TK_UNION); }
"unsigned"              {  return(TK_UNSIGNED); }
"void"                  {  return(TK_VOID); }
"volatile"              {  return(TK_VOLATILE); }
"while"			{  return(TK_WHILE); }

"__extension__" 	{ return TK_EXTENSION;}
"__attribute__" 	{ return TK_ATTRIBUTE;}
{L}({L}|{D})*		{ return(check_type()); }

0[xX]{H}+{IS}?          {  
		/* constants and string literals */

		/* hexadecimal */
 		return(TK_CONSTANT); 
	}
0{D}+{IS}?              {  /* octal */ return(TK_CONSTANT); }
{D}+{IS}?               {  /* decimal */ return(TK_CONSTANT); }
("L")?'(\\.|[^\\'])+'         {  /* char */ return(TK_CONSTANT); }
{D}+{E}{FS}?            {  /* floating point */ return(TK_CONSTANT); }
{D}*"."{D}+({E})?{FS}?  {  /* floating point */ return(TK_CONSTANT); }
{D}+"."{D}*({E})?{FS}?  {  /* floating point */ return(TK_CONSTANT); }
\"(\\.|[^\\"])*\"       {  /* string */ return(TK_STRING_LITERAL); }

"\.\.\."                {  
		/* Operators */
 		return(TK_ELIPSIS); 
	}
">>="                   {  return(TK_RIGHT_ASSIGN); }
"<<="                   {  return(TK_LEFT_ASSIGN); }
"+="                    {  return(TK_ADD_ASSIGN); }
"-="                    {  return(TK_SUB_ASSIGN); }
"*="                    {  return(TK_MUL_ASSIGN); }
"/="                    {  return(TK_DIV_ASSIGN); }
"%="                    {  return(TK_MOD_ASSIGN); }
"&="                    {  return(TK_AND_ASSIGN); }
"^="                    {  return(TK_XOR_ASSIGN); }
"|="                    {  return(TK_OR_ASSIGN); }
">>"                    {  return(TK_RIGHT_OP); }
"<<"                    {  return(TK_LEFT_OP); }
"++"                    {  return(TK_INC_OP); }
"--"                    {  return(TK_DEC_OP); }
"->"                    {  return(TK_PTR_OP); }
"&&"                    {  return(TK_AND_OP); }
"||"                    {  return(TK_OR_OP); }
"<="                    {  return(TK_LE_OP); }
">="                    {  return(TK_GE_OP); }
"=="                    {  return(TK_EQ_OP); }
"!="                    {  return(TK_NE_OP); }
";"                     {  return(';'); }
"{"                     {  return('{'); }
"}"                     {  return('}'); }
","                     {  return(','); }
":"                     {  return(':'); }
"="                     {  return('='); }
"("                     {  return('('); }
")"                     {  return(')'); }
"["                     {  return('['); }
"]"                     {  return(']'); }
"."                     {  return('.'); }
"&"                     {  return('&'); }
"!"                     {  return('!'); }
"~"                     {  return('~'); }
"-"                     {  return('-'); }
"+"                     {  return('+'); }
"*"                     {  return('*'); }
"/"                     {  return('/'); }
"%"                     {  return('%'); }
"<"                     {  return('<'); }
">"                     {  return('>'); }
"^"                     {  return('^'); }
"|"                     {  return('|'); }
"?"                     {  return('?'); }

{LWSC}                  {  
		/* Absorb whitespace character. */
	}
\n 				{}

^{LWSC}*#.*             {}

.                       {  
		/* Trap any non-standard characters. */
 		yyerror("Syntax error"); }

%%

int yywrap()
{
	return 1;
}

/**
 * The beginning of a comment has been detected. Handle until the entire
 * comment has been consumed, then give control back over to lex.
 */
static void comment(void)
{
   int c;

   /* Loop until input exhausted or end-of-comment reached. */
   for ( ; (c = input()) != EOF; )
      if (c == '*')                    /* Could be end-of-comment. */
      {
         int c1;

         if ((c1 = input()) == '/')
            break;                     /* Is end-of-comment. */
         else
            unput(c1);                 /* False alarm. Not end-of-comment. */
      }
}

/*******************************************************************
 * I had planned to make check_type as powerful as possible but
 * eventually I abandoned such a design with the philosophy that
 * one mudule should not do what it is not supposed to do even
 * though it can handle it.
 * In this case, though the lexer can recognize the context 
 * for the current identifier scanned (by using last_stg_cls,
 * LEVEL and look_for_tag), I don't want it to do so simply because
 * the parser is more proper to do so. (This method may
 * involve some overhead -- e.g. some unnecessary string buffer
 * allocation -- but should make debugging easier.)
 * However, it still does something quite intelligent: it 
 * tries to see if the parser is parsing a declaration or an
 * expression. (If the current name is not a typedef name,
 * and last_stg_cls is ERR, then it is parsing an expression.)
 *
 * So check_type always put a symbol structure to yylval (or NULL) 
 * and return either TK_IDENTIFIER or TK_TYPE.
 * If the identifier is a tag, return TK_IDENTIFIER and set yylval to
 * NULL because tag is always used
 * temporarily. Otherwise, it will try to search the symbol table (
 * according to the id name). If not found, then if it is now
 * in declaration, it makes a new symbol with decltor and LEVEL
 * set and return TK_IDENTIFIER; if in expression, set yylval to NULL
 * because either it is a struct field name (which is not stored 
 * in symbol table) or it encounters an error. 
 * If found, check if a typedef name, if so, return TK_TYPE;
 * otherwise, if in declaration, report error if the existing symbol has
 * the same level and not extern and not isFunc, or set yylval 
 * to a new symbol if not; if in expression, set yylval to 
 * the existing symbol.
 *
 * To summarize: a non-keyword name could be:
 * tag (after enum, struct, union)
 * global var def (including top level static), global var decl,
 * typedef name (local or global), 
 * struct(union) field name in def, 
 * struct(union) field name in reference, 
 * function def name, 
 * function decl name(local or global), 
 * local decled global var, local static var, 
 * local var reference, 
 * global var reference, 
 * local static var reference, 
 * local decled global var reference.
 * function decl/def parameter names in parenthese;
 * function def parameter names after function header (old fashion)
 * enumeration constants
 *
 * The following comments are obsolate and I kept them because 
 * it records somethings still needed to be considered in the
 * future.
 */

/** 
 * Check if the yytext is global declaration, local declaration, typedef
 * name, defined type name, variable reference or tag.
 * The actual yylval is (GV *), (LV *) and (ID *) in different situations. 
 * First check if it is a tag (look_for_tag==TRUE), if so, return an ID
 * struct. Otherwise, it could be either a typename that is defined before
 * or one of the following categories. If it is the former, we return a
 * ID struct, otherwise, we return either a LV struct or GV struct
 * depending on different situations. 
 * The category of a name is determined by the states of two variables:
 * 	 		LEVEL and last_stg_cls 
 * according to the following table:
 *-----------------------------------------------------------------------
 * last_stg_cls=   EXTERN     STATIC/NONE  TYPEDEF    ERR
 * LEVEL=0         GV decl       GV def   TDEF name  error
 * LEVEL>0       local decl GV  LV decl   TDEF name  var ref
 *-----------------------------------------------------------------------
 * There are six categories. The action for each category is described 
 * in the implementation manual.
 * TK_TYPE_NAME : TK_IDENTIFIER
 */
static int check_type()
{
   int type;
   SYM *sym;

   /*
      looking_for_tag is set to TRUE only when the parser is looking for
      a struct, union, or enum tag. Since tags are in a separate name
      space, the current lexeme can never be a typedef type name and are
      therefore always an identifier.
   */
  	if (look_for_tag) {
		type=TK_IDENTIFIER;
		look_for_tag=FALSE; /* I can do it because it is impossible */
				    /* to have two tags together. */
		yylval.p_sym=NULL; /* yytext has the lexeme */
	} 
	else if (parse_attrib_list) {
		type=TK_IDENTIFIER;
		yylval.p_sym=NULL;
	}
	else {
		if ((sym=(SYM *)findsym(sym_tab, yytext))) { 
			/* found in the symbol table */
			if (sym->stg_cls==TYPEDEF) {
				yylval.p_sym=sym;
				type=TK_TYPE_NAME;
			}
			else {
				type=TK_IDENTIFIER;
				if (last_stg_cls==ERR) {
					/* assuming in expression */
					/**
					 * such assumption may be wrong when
					 * parsing the parameter list or 
					 * enum constants, but
					 * yylval are ignored in these situations
					 */
					yylval.p_sym=sym;
				}
				else if ((LEVEL!=sym->LEVEL)||
					   ((sym->stg_cls==EXTERN)&&(last_stg_cls==EXTERN))||
					   ((sym->stg_cls==SYS)&&(last_stg_cls==EXTERN))||
					   (sym->isFunc==TRUE)) /* and hope this id is also a func */
				 {
					yylval.p_sym=new_SYM(yytext, LEVEL);
				 }
				else {
					/* possibly the last_stg_cls has no time to change */
					/* because this identifier may be the lookahead symbol */
					yylval.p_sym=sym;
				}
			}
		}
		else {
			type=TK_IDENTIFIER;
			if (last_stg_cls==ERR) {
				/**
				 * assuming in expression or 
				 * function parameter_list or
				 * enum constants
				 */
				yylval.p_sym=NULL;
			}
			else {
				yylval.p_sym=new_SYM(yytext, LEVEL);
			}

		}
	}
   	return type;
}

/* Return pointer to current input token (lexeme). */
char *token(void)
{
   return yytext;
}


/* Return current line nuber. */
unsigned lineno(void)
{
   return yylineno;
}

/* Extract and return the line number out of the #line directive. */
static unsigned extract_line_number(char *string)
{
   return (unsigned)strtol(&string[strcspn(string, "0123456789")], NULL, 10);
}

/*
   Extract and return the file name out of the #line directive. If not
   present, return NULL.
*/
static char *extract_file_name(char *string)
{
   char *start_of_file_name;

   /* File name is enclosed in quotes. Return NULL if no first quote. */
   start_of_file_name = strchr(string, '"');
   if (start_of_file_name != NULL)
   {
      char *end_of_file_name;

      ++start_of_file_name;      /* Skip past first quote. */

      /* If no trailing quote, return NULL. */
      end_of_file_name = strchr(start_of_file_name, '"');
      if (end_of_file_name == NULL)
         start_of_file_name = NULL;
      else
      {
         size_t file_name_length;
         static char return_buffer[MAX_FILENAME+1];

         file_name_length = end_of_file_name - start_of_file_name;

         /* Copy file name between quotes. */
         strncpy(return_buffer, start_of_file_name, file_name_length);
         return_buffer[file_name_length] = '\0';

         /* Buffer is static, so it's still viable after returning. */
         start_of_file_name = return_buffer;
      }
   }

   return start_of_file_name;
}

