%{
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <limits.h>
# include <stdarg.h>
# include <string.h>
# include <ctype.h>
# include "global.h"
# include "hash.h"
# include "symbol.h"
# include "misc.h"

/**
# define YYPRINT(file, type, value) yyprint1(type)
**/
extern int yylex();
extern char *token();

static STR *SafeName(STR *);
static void	ResetStates();
static SYM * ParseDeclList(SYM *list);
static void merge_paralist(SYM *decltor, STR *lp, SYM *paralist, STR *rp);
static STR *merge_paralist2(STR *opt, STR *lp, SYM *paralist, STR *rp);
static unsigned int saveStates();
static void restoreStates(unsigned int states);
static void fixFuncName(SYM *);
static Bool toBeTranslated();
static void yyprint1(int type);
static Bool isSpecial(SYM *);

static Bool tbool;

/**
# define sb_newframe(x) (1)
# define sb_restore(x) (1)
# define del_str(x) (1)
**/
/**
 * is_const_decl_spec is TRUE when parsing a SIMPLE constant var
 * declaration (not typedef name, not struct or union, not 
 * pointer, not array). is_func_type_spec is TURN if the last 
 * type_specifier is a tk_type_name and has isFunc set. 
 * Both flags are needed to check whether one variable is a simple 
 * constant and whether it is a function declaration. The
 * latter can be complicated as following:
 * typedef int intFunc(int);
 * intFunc abs; 
 *         ^^^ abs is a function instead of a global var!
 * int abs(int x) {return (x=0)?x:-x; }
 * 
 * is_const_decl_spec is set to UNKNOWN when not in declaration.
 * is_const_decl_spec remains UNKNOWN when only simple type_spec
 * is seen so far and no volatile or const encountered.
 * In the SYM struct, isConst is the per-symbol version of 
 * is_const_decl_spec. It can be UNKNOWN, in which case it meams
 * this is a simple variable or a simple typename. 
 * Such info is useful to associate the variable directly 
 * with the key instead of associating a pointer pointing
 * to a structure containing the variable.
 */

 /**
  * To exploit constant delcarations, 
  * for arrays, we need to have the array elements
  * to be constants. For pointer, we need to have both
  * the pointer and the element it points to are constants.
  */
static Bool is_func_type_spec=UNKNOWN;
static Bool is_const_decl_spec=UNKNOWN;

/**
 * look_for_type_specifier is TRUE when no type_specifier
 * is seen so far in the declaration_specifiers. If 
 * this flag is TRUE after parsing the whole list, then
 * insert an "int" to the end of the list. e.g. 
 * extern a[10];
 * CONVERTED TO:
 * typedef int T1[10];
 * typedef struct {
 * 		T1 a;
 * } T2;
 * _Gxxxxlgvwa=(T *)thrMPI_getval(_GxxxxKEYa);
 * 
 * The flag is set to TRUE when not in declaration.
 * 
 * This approach doesn't work!
 * For example:
 * extern int a[];
 * CONVERTED TO:
 * typedef int T1[];
 * typedef struct {
 * 		T1 a; //<---- compiler complains that it cannot determine the size of a.
 * } T2;
 */
static Bool look_for_type_specifier=TRUE;

/** I finally decided to abort such an unsafe approach and use compare_and_swap instead */
/**
 * staticKEY keeps the total static keys used so far. We statically
 * allocate keys corresponding to static variables. We exploit the
 * fact that type of key is actually an integer. This may not
 * be a good idea, but it helps reduce the possible weiry code in
 * each function that contains some static variable, whose purpose 
 * is to check if the key has been assigned a value (meanwhile such
 * a test must be atomic.)
 */
/* static int staticKEY=1; */ /* key==1 is reserved for IDKEY */

/*
	Indicates whether we are within a function definition. Used to
	differentiate the situations between function definition parameter
	list and function declaration parameter list. For the former case,
	we need to add the parameter list into the symbol table while for
	the latter, we will merge the parameter list with the function name.
*/

/**
 * Note the tk_xxx -> TK_XXX are used to get the real lexicon of the token 
 * because these are one to one productions are guaranteed not to look
 * ahead.
 */

/* Control depth--current number of nested control structures. */
/* static unsigned depth; */

static char last_ident[MAX_SYM_LEN+1]; /* keep the last id, because 
token() might not get what we want due to lookahead */

%}

%union {
	SYM 	*p_sym;
	STR	*p_str;
	/* 
		Containing all decl states:
			is_func_type_spec=UNKNOWN;
			is_const_decl_spec=UNKNOWN;
			look_for_type_specifier=TRUE;
			is_within_funcdef=FALSE;
			last_stg_cls;
	*/
	int	states; 
}

%token <p_sym> TK_IDENTIFIER TK_TYPE_NAME
%token TK_CONSTANT TK_STRING_LITERAL TK_SIZEOF 
%token TK_PTR_OP TK_INC_OP TK_DEC_OP TK_LEFT_OP TK_RIGHT_OP
%token TK_LE_OP TK_GE_OP TK_EQ_OP TK_NE_OP
%token TK_AND_OP TK_OR_OP TK_MUL_ASSIGN TK_DIV_ASSIGN TK_MOD_ASSIGN
%token TK_ADD_ASSIGN TK_SUB_ASSIGN TK_LEFT_ASSIGN TK_RIGHT_ASSIGN 
%token TK_XOR_ASSIGN TK_OR_ASSIGN TK_AND_ASSIGN

%token TK_TYPEDEF TK_EXTERN TK_STATIC TK_AUTO TK_REGISTER
%token TK_CHAR TK_SHORT TK_INT TK_LONG TK_SIGNED TK_UNSIGNED
%token TK_FLOAT TK_DOUBLE TK_CONST TK_VOLATILE TK_VOID
%token TK_STRUCT TK_UNION TK_ENUM TK_ELIPSIS TK_RANGE

%token TK_CASE TK_DEFAULT TK_IF TK_ELSE TK_SWITCH TK_WHILE TK_DO TK_FOR
%token TK_GOTO TK_CONTINUE TK_BREAK TK_RETURN
%token TK_EXTENSION TK_ATTRIBUTE

%type <p_str> tk_lparen
%type <p_str> tk_rparen
%type <p_str> tk_lsquare_brace
%type <p_str> tk_rsquare_brace
%type <p_str> tk_dot
%type <p_str> tk_ptr_to
%type <p_str> tk_inc
%type <p_str> tk_dec
%type <p_str> tk_comma
%type <p_str> tk_sizeof
%type <p_str> tk_ampersand
%type <p_str> tk_asterisk
%type <p_str> tk_plus
%type <p_str> tk_minus
%type <p_str> tk_tilde
%type <p_str> tk_exclamation
%type <p_str> tk_slash
%type <p_str> tk_percent
%type <p_str> tk_left
%type <p_str> tk_right
%type <p_str> tk_lt
%type <p_str> tk_gt
%type <p_str> tk_le
%type <p_str> tk_ge
%type <p_str> tk_eq
%type <p_str> tk_ne
%type <p_str> tk_circumflex
%type <p_str> tk_bar
%type <p_str> tk_land
%type <p_str> tk_lor
%type <p_str> tk_question
%type <p_str> tk_colon
%type <p_str> tk_assign
%type <p_str> tk_mul_assign
%type <p_str> tk_div_assign
%type <p_str> tk_mod_assign
%type <p_str> tk_add_assign
%type <p_str> tk_sub_assign
%type <p_str> tk_left_assign
%type <p_str> tk_right_assign
%type <p_str> tk_and_assign
%type <p_str> tk_xor_assign
%type <p_str> tk_or_assign
%type <p_str> tk_semicolon
%type <p_str> tk_typedef
%type <p_str> tk_extern
%type <p_str> tk_static
%type <p_str> tk_auto
%type <p_str> tk_register
%type <p_str> tk_char
%type <p_str> tk_short
%type <p_str> tk_int
%type <p_str> tk_long
%type <p_str> tk_signed
%type <p_str> tk_unsigned
%type <p_str> tk_float
%type <p_str> tk_double
%type <p_str> tk_const
%type <p_str> tk_volatile
%type <p_str> tk_void
%type <p_str> tk_struct
%type <p_str> tk_union
%type <p_str> tk_enum
%type <p_sym> tk_type_name
%type <p_str> tk_lcurly_brace
%type <p_str> tk_rcurly_brace
%type <p_str> tk_elipsis
%type <p_str> tk_case
%type <p_str> tk_default
%type <p_str> tk_if
%type <p_str> tk_switch
%type <p_str> tk_else
%type <p_str> tk_while
%type <p_str> tk_do
%type <p_str> tk_for
%type <p_str> tk_goto
%type <p_str> tk_continue
%type <p_str> tk_break
%type <p_str> tk_return
%type <p_str> tk_string
%type <p_str> tk_constant
%type <p_sym> identifier
%type <p_str> primary_expr
%type <p_str> string_list
%type <p_str> postfix_expr
%type <p_str> argument_expr_list
%type <p_str> unary_expr
%type <p_str> unary_operator
%type <p_str> cast_expr
%type <p_str> multiplicative_expr
%type <p_str> additive_expr
%type <p_str> shift_expr
%type <p_str> relational_expr
%type <p_str> equality_expr
%type <p_str> and_expr
%type <p_str> exclusive_or_expr
%type <p_str> inclusive_or_expr
%type <p_str> logical_and_expr
%type <p_str> logical_or_expr
%type <p_str> conditional_expr
%type <p_str> assignment_expr
%type <p_str> assignment_operator
%type <p_str> expr
%type <p_str> constant_expr
%type <p_sym> declaration
%type <p_str> declaration_specifiers
%type <p_sym> init_declarator_list
%type <p_sym> init_declarator
%type <p_str> storage_class_specifier
%type <p_str> type_specifier
%type <p_str> type_qualifier
%type <p_str> struct_or_union_specifier
%type <p_str> struct_or_union
%type <p_str> struct_declaration_list
%type <p_str> struct_declaration
%type <p_str> struct_declarator_list
%type <p_str> struct_declarator
%type <p_str> enum_specifier
%type <p_str> enumeration
%type <p_str> enum
%type <p_str> enumerator_list
%type <p_str> enumerator
%type <p_sym> declarator
%type <p_sym> declarator2
%type <states> func_decl_intro
%type <p_str> pointer
%type <p_str> type_specifier_list
%type <p_str> type_qualifier_list
%type <p_str> specifier_qualifier_list
%type <p_sym> parameter_identifier_list
%type <p_sym> identifier_list
%type <p_sym> parameter_type_list
%type <p_sym> parameter_list
%type <p_sym> parameter_declaration
%type <p_str> type_name
%type <p_str> abstract_declarator
%type <p_str> direct_abstract_declarator
%type <p_str> initializer
%type <p_str> initializer_list
%type <p_sym> declaration_list
%type <p_str> opt_expr
%type <p_sym> translation_unit
%type <p_sym> external_declaration
%type <p_sym> function_definition
%type <p_str> tk_extension
%type <p_str> tk_attribute
%type <p_str> attrib_list
%type <p_str> attrib_item
%type <p_str> attrib_name
%type <p_str> attrib_value
%type <p_str> attrib_value_list
%type <p_str> opt_attribute

%start program

/* Used to disambiguate if from if/else. */
%nonassoc THEN
%nonassoc TK_ELSE

%%

tk_lparen
   : '(' { $$ = new_str(token()); }
   ;

tk_rparen
   : ')' { $$ = new_str(token()); }
   ;

tk_lsquare_brace
   : '[' { $$ = new_str(token()); }
   ;

tk_rsquare_brace
   : ']' { $$ = new_str(token()); }
   ;

tk_dot
   : '.' { $$ = new_str(token()); }
   ;

tk_ptr_to
   : TK_PTR_OP { $$ = new_str(token()); }
   ;

tk_inc
   : TK_INC_OP { $$ = new_str(token()); }
   ;

tk_dec
   : TK_DEC_OP { $$ = new_str(token()); }
   ;

tk_comma
   : ',' { $$ = new_str(token()); }
   ;


tk_sizeof
   : TK_SIZEOF { $$ = new_str(token()); }
   ;

tk_ampersand 
	: '&' { $$ = new_str(token()); }
   ;

tk_asterisk
   : '*' { $$ = new_str(token()); }
   ;

tk_plus
   : '+' { $$ = new_str(token()); }
   ;

tk_minus
   : '-' { $$ = new_str(token()); }
   ;

tk_tilde
   : '~' { $$ = new_str(token()); }
   ;

tk_exclamation
   : '!' { $$ = new_str(token()); }
   ;

tk_slash
   : '/' { $$ = new_str(token()); }
   ;

tk_percent
   : '%' { $$ = new_str(token()); }
   ;

tk_left
   : TK_LEFT_OP { $$ = new_str(token()); }
   ;

tk_right
   : TK_RIGHT_OP { $$ = new_str(token()); }
   ;

tk_lt 
	: '<' { $$ = new_str(token()); }
   ;

tk_gt
   : '>' { $$ = new_str(token()); }
   ;

tk_le
   : TK_LE_OP { $$ = new_str(token()); }
   ;

tk_ge
   : TK_GE_OP { $$ = new_str(token()); }
   ;

tk_eq
   : TK_EQ_OP { $$ = new_str(token()); }
   ;

tk_ne
   : TK_NE_OP { $$ = new_str(token()); }
   ;

tk_circumflex
   : '^' { $$ = new_str(token()); }
   ;

tk_bar
   : '|' { $$ = new_str(token()); }
   ;

tk_land
   : TK_AND_OP { $$ = new_str(token()); }
   ;

tk_lor
   : TK_OR_OP { $$ = new_str(token()); }
   ;

tk_question
   : '?' { $$ = new_str(token()); }
   ;

tk_colon
   : ':' { $$ = new_str(token()); }
   ;

tk_assign
   : '=' { $$ = new_str(token()); }
   ;

tk_mul_assign
   : TK_MUL_ASSIGN { $$ = new_str(token()); }
   ;

tk_div_assign
   : TK_DIV_ASSIGN { $$ = new_str(token()); }
   ;

tk_mod_assign
   : TK_MOD_ASSIGN { $$ = new_str(token()); }
   ;

tk_add_assign
   : TK_ADD_ASSIGN { $$ = new_str(token()); }
   ;

tk_sub_assign
   : TK_SUB_ASSIGN { $$ = new_str(token()); }
   ;

tk_left_assign
   : TK_LEFT_ASSIGN { $$ = new_str(token()); }
   ;

tk_right_assign
   : TK_RIGHT_ASSIGN { $$ = new_str(token()); }
   ;

tk_and_assign
   : TK_AND_ASSIGN { $$ = new_str(token()); }
   ;

tk_xor_assign
   : TK_XOR_ASSIGN { $$ = new_str(token()); } ;

tk_or_assign
   : TK_OR_ASSIGN { $$ = new_str(token()); } 
	;

tk_semicolon
   : ';' { $$ = new_str(token()); }
   ;

tk_typedef
   : TK_TYPEDEF { $$ = new_str(token()); }
   ;

tk_extern
   : TK_EXTERN { $$ = new_str(token()); }
   ;

tk_static
   : TK_STATIC { $$ = new_str(token()); }
   ;

tk_auto
   : TK_AUTO { $$ = new_str(token()); }
   ;

tk_register
   : TK_REGISTER { $$ = new_str(token()); }
   ;

tk_char
   : TK_CHAR { $$ = new_str(token()); }
   ;

tk_short
   : TK_SHORT { $$ = new_str(token()); }
   ;

tk_int
   : TK_INT { $$ = new_str(token()); }
   ;

tk_long
   : TK_LONG { $$ = new_str(token()); }
   ;

tk_signed
   : TK_SIGNED { $$ = new_str(token()); }
   ;

tk_unsigned 
	: TK_UNSIGNED { $$ = new_str(token()); }
   ;

tk_float
   : TK_FLOAT { $$ = new_str(token()); }
   ;

tk_double
   : TK_DOUBLE { $$ = new_str(token()); }
   ;

tk_const
   : TK_CONST { $$ = new_str(token()); }
   ;

tk_volatile
   : TK_VOLATILE { $$ = new_str(token()); }
   ;

tk_void
   : TK_VOID { $$ = new_str(token()); }
   ;

tk_struct
   : TK_STRUCT { $$ = new_str(token()); } 
	;

tk_union
   : TK_UNION { $$ = new_str(token()); }
   ;

tk_enum
   : TK_ENUM { $$ = new_str(token()); }
   ;

tk_type_name
   : TK_TYPE_NAME 
   ;

tk_lcurly_brace
   : '{' { $$ = new_str(token()); }
   ;

tk_rcurly_brace
   : '}' { $$ = new_str(token()); }
   ;

tk_elipsis
   : TK_ELIPSIS { $$ = new_str(token()); }
   ;

tk_case
   : TK_CASE { $$ = new_str(token()); }
   ;

tk_default
   : TK_DEFAULT { $$ = new_str(token()); }
   ;

tk_if
   : TK_IF { $$ = new_str(token()); }
   ;

tk_switch
   : TK_SWITCH { $$ = new_str(token()); }
   ;

tk_else
   : TK_ELSE { $$ = new_str(token()); }
   ;

tk_while
   : TK_WHILE { $$ = new_str(token()); }
   ;

tk_do
   : TK_DO { $$ = new_str(token()); }
   ;

tk_for
   : TK_FOR { $$ = new_str(token()); }
   ;

tk_goto
   : TK_GOTO 
	{ 
		look_for_tag=TRUE;
		$$ = new_str(token()); 
	}
   ;

tk_continue
   : TK_CONTINUE { $$ = new_str(token()); }
   ;

tk_break
   : TK_BREAK { $$ = new_str(token()); }
   ;

tk_return
   : TK_RETURN { $$ = new_str(token()); }
   ;

tk_string
   : TK_STRING_LITERAL { $$ = new_str(token()); }
   ;

tk_constant
   : TK_CONSTANT { $$ = new_str(token()); }
   ;

tk_extension
   : TK_EXTENSION { $$ = new_str(token()); }
   ;

tk_attribute
   : TK_ATTRIBUTE { $$ = new_str(token()); }
   ;

identifier
   : TK_IDENTIFIER 
	{
         /*
            Save this identifier in case token buffer (yytext[]) gets
            overwritten by the time the identifier is needed.
         */
         strncpy(last_ident, token(), MAX_SYM_LEN);
	$$=$1;
	}
	;

primary_expr
	: identifier
	{
		if ($1==NULL) {
			/* really can say nothing, because it may be a enum constant */
			yyerror("Warning: possible missing declaration for %s unless it is an enum name", last_ident);
			$$=new_str(last_ident);
			$$=SafeName($$);
		}
		else {
			if ( ($1->isFunc==TRUE) || ($1->isConst==TRUE) || (!(isGlobal($1))) 
				|| (!toBeTranslated()) )  /* this condition should not occur */
			{
				$$=new_str(SYMSTR($1->name));
				$$=SafeName($$);
			} else {
				/* handle global var, check if needs to put into lglist */
				$$=new_str("(*");
				$$=str_append($$, make_lgvw(SYMSTR($1->name)));
				$$=str_append($$,")");
				if ($1->LEVEL==0) { /* need to change lglist */
					new_LGL($1);
				}
			}		
		}
	}
   | tk_constant
   | string_list
   | tk_lparen expr tk_rparen
   	{
      	$$ = str_cat($1,$2);
         	$$ = str_cat($$,$3);
		del_str($2);
		del_str($3);
      }
   ;

/* (In case a preprocessor wasn't run to join adjacent strings.) */
string_list
   : tk_string
   | string_list tk_string
   	{
         $$ = str_append($1," ");
         $$ = str_cat($$,$2);
		del_str($2);
      }
   ;

postfix_expr
   : primary_expr
   | postfix_expr tk_lsquare_brace expr tk_rsquare_brace
   {
     	$$ = str_cat($1,$2);
     	$$ = str_cat($$,$3);
     	$$ = str_cat($$,$4);
		del_str($2);
		del_str($3);
		del_str($4);
   }
   | postfix_expr 
	tk_lparen tk_rparen
  	{
     	$$ = str_cat($1,$2);
     	$$ = str_cat($$,$3);
		del_str($2);
		del_str($3);
	}
   | postfix_expr
	tk_lparen argument_expr_list tk_rparen
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
      	$$ = str_cat($$,$4);
			del_str($2);
			del_str($3);
			del_str($4);
		}
   | postfix_expr tk_dot identifier
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_append($$,last_ident);
			del_str($2);
      }
   | postfix_expr tk_ptr_to identifier
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_append($$,last_ident);
			del_str($2);
      }
   | postfix_expr tk_inc
   	{
      	$$ = str_cat($1,$2);
			del_str($2);
      }
   | postfix_expr tk_dec
   	{
      	$$ = str_cat($1,$2);
			del_str($2);
      }
   ;

argument_expr_list
   : assignment_expr
   | argument_expr_list tk_comma assignment_expr
   	{
      	$$ = str_cat($1,$2);
       $$ = str_append($$," ");
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

unary_expr
   : postfix_expr
   | tk_inc unary_expr
   	{
      	$$ = str_cat($1,$2);
			del_str($2);
      }
   | tk_dec unary_expr
   	{
      	$$ = str_cat($1,$2);
			del_str($2);
      }
   | unary_operator cast_expr
   	{
      	$$ = str_cat($1,$2);
			del_str($2);
      }
   | tk_sizeof unary_expr
   	{
         $$ = str_append($1," ");
      	$$ = str_cat($$,$2);
			del_str($2);
      }
   | tk_sizeof tk_lparen type_name tk_rparen
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
      	$$ = str_cat($$,$4);
			del_str($2);
			del_str($3);
			del_str($4);
      }
   ;

unary_operator
   : tk_ampersand
   | tk_asterisk
   | tk_plus
   | tk_minus
   | tk_tilde
   | tk_exclamation
   ;

cast_expr
   : unary_expr
   | tk_lparen type_name tk_rparen cast_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
      	$$ = str_cat($$,$4);
			del_str($2);
			del_str($3);
			del_str($4);
      }
   ;

multiplicative_expr
   : cast_expr
   | multiplicative_expr tk_asterisk cast_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   | multiplicative_expr tk_slash cast_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   | multiplicative_expr tk_percent cast_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

additive_expr
   : multiplicative_expr
   | additive_expr tk_plus multiplicative_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   | additive_expr tk_minus multiplicative_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

shift_expr
   : additive_expr
   | shift_expr tk_left additive_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   | shift_expr tk_right additive_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

relational_expr
   : shift_expr
   | relational_expr tk_lt shift_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   | relational_expr tk_gt shift_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   | relational_expr tk_le shift_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   | relational_expr tk_ge shift_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

equality_expr
   : relational_expr
   | equality_expr tk_eq relational_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   | equality_expr tk_ne relational_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

and_expr
   : equality_expr
   | and_expr tk_ampersand equality_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

exclusive_or_expr
   : and_expr
   | exclusive_or_expr tk_circumflex and_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

inclusive_or_expr
   : exclusive_or_expr
   | inclusive_or_expr tk_bar exclusive_or_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

logical_and_expr
   : inclusive_or_expr
   | logical_and_expr tk_land inclusive_or_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

logical_or_expr
   : logical_and_expr
   | logical_or_expr tk_lor logical_and_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

conditional_expr
   : logical_or_expr
   | logical_or_expr tk_question expr tk_colon conditional_expr
      {
      	$$ = str_cat($1,$2);
         $$ = str_append($$," ");
      	$$ = str_cat($$,$3);
         $$ = str_append($$," ");
      	$$ = str_cat($$,$4);
         $$ = str_append($$," ");
      	$$ = str_cat($$,$5);
			del_str($2);
			del_str($3);
			del_str($4);
			del_str($5);
      }
   ;

assignment_expr
   : conditional_expr
   | unary_expr assignment_operator assignment_expr
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

assignment_operator
   : tk_assign
   | tk_mul_assign
   | tk_div_assign
   | tk_mod_assign
   | tk_add_assign
   | tk_sub_assign
   | tk_left_assign
   | tk_right_assign
   | tk_and_assign
   | tk_xor_assign
   | tk_or_assign
   ;

expr
   : assignment_expr
   | expr tk_comma assignment_expr
   	{
      	$$ = str_cat($1,$2);
         $$ = str_append($$," ");
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

constant_expr
   : conditional_expr
   ;

attrib_name 
	: identifier
	{
		$$=new_str(last_ident);
	}
	| tk_const
	;

attrib_value 
	: identifier
	{
		$$=new_str(last_ident);
	}
	| tk_constant
	| string_list
	;

attrib_value_list
	: attrib_value
	| attrib_value_list tk_comma attrib_value
	{
		$$=str_cat($1, $2);
		$$=str_append($$, " ");
		$$=str_cat($$, $3);
		del_str($2);
		del_str($3);
	}
	;

attrib_item 
	: attrib_name
	| attrib_name tk_lparen attrib_value_list tk_rparen
	{
		$$=str_cat($1, $2);
		$$=str_cat($$, $3);
		$$=str_cat($$, $4);
		del_str($2);
		del_str($3);
		del_str($4);
	}
	;
	
attrib_list 
	: attrib_item
	| attrib_list tk_comma attrib_item
	{
		$$=str_cat($1, $2);
		$$=str_append($$, " ");
		$$=str_cat($$, $3);
		del_str($2);
		del_str($3);
	}
	;

opt_attribute
	: {
		/* epsilon */
		$$=NULL;
		}
	| tk_attribute tk_lparen 
	{
		parse_attrib_list=TRUE;
	}
	tk_lparen attrib_list tk_rparen tk_rparen
	{
		$$=str_append($1, " ");
		$$=str_cat($$, $2);
		$$=str_cat($$, $4);
		$$=str_cat($$, $5);
		$$=str_cat($$, $6);
		$$=str_cat($$, $7);
		parse_attrib_list=FALSE;
		del_str($2);
		del_str($4);
		del_str($5);
		del_str($6);
		del_str($7);
	}
	;

declaration
   : declaration_specifiers opt_attribute tk_semicolon
   {
		if (look_for_type_specifier) { /* missing "int" */
			$1=str_append($1," int");
		}
		/* output it */
		setIndent(LEVEL);

		if (!$2)
			emitCode("%s;", str_string($1));
		else
			emitCode("%s %s;", str_string($1), str_string($2));
		$$=NULL;
		ResetStates();
		del_str($1);
		if ($2)
			del_str($2);
		del_str($3);
		last_stg_cls=ERR; /* change */
   }
   | declaration_specifiers init_declarator_list opt_attribute tk_semicolon
	{
		if (look_for_type_specifier) { /* missing "int" */
			$1=str_append($1," int");
		}
		/**
		 * need states LEVEL, is_func_type_spec, is_const_decl_spec,
		 * last_stg_cls add symbols to sym_tab, process extern or 
		 * static var decl/def process initialization.
		 * Free the decltor field and initstr field of each symbol.
		 */
		$2=ParseDeclList($2); /* reverse the link, set isFunc, isConst fileds */
		setIndent(LEVEL);
		if (isGlobalDecl && toBeTranslated() ) {
			/* static, extern or top level without stg_cls spec */
			SYM *sym=$2;

			/* generate typedef */

			emitCode("typedef ");
			emitCode("%s\t", str_string($1));
			while (sym) {
				emitCode(str_string(sym->decltor));
				del_str(sym->decltor);
				sym->decltor=NULL;
				if (sym->next)
					emitCode(", ");
				sym=sym->next;
			}
			if ($3)
				emitCode(" %s", str_string($3));

			emitCode(";");

			sym=$2;
			while (sym) {
				setIndent(LEVEL);
				setIndent(LEVEL);
				if ( (sym->isFunc==TRUE) || (sym->isConst==TRUE) || (isSpecial(sym)) ) { 
					/* do not translate for constant/function definitions */
					switch (sym->stg_cls) {
					case EXTERN: emitCode("extern "); break;
					case STATIC: emitCode("static "); break;
					case NONE: break;
					default: yyerror("Internal: stg_cls ??"); exit(-1); 
					}
					if (sym->initstr) {
						emitCode("%s\t%s=%s;", make_typedef(sym->type), \
							SYMSTR(sym->name), str_string(sym->initstr) );
						del_str(sym->initstr);
						sym->initstr=NULL;
					}
					else
						emitCode("%s %s;", make_typedef(sym->type), \
							SYMSTR(sym->name));
				}
				else {
					/* generate key */
					switch (sym->stg_cls) {
					case EXTERN: emitCode("extern "); break;
					case STATIC: emitCode("static "); break;
					case NONE: break;
					default: yyerror("Internal: stg_cls ??"); exit(-1); 
					}
					emitCode("int\t%s", make_gvkey(SYMSTR(sym->name)));

					if ( (sym->stg_cls==STATIC) && (LEVEL>0) ) {
						/* local static */
						emitCode("=-1;");
					}
					else {
						emitCode(";");
					}
					if ( (sym->stg_cls!=EXTERN) && (LEVEL==0) )
						gv_createkey(SYMSTR(sym->name));

					if (LEVEL>0) { /* local static/extern def/decl */
						setIndent(LEVEL);
						if (sym->stg_cls==STATIC) {
							char *symname=SYMSTR(sym->name);

							emitCode("%s *%s;", make_typedef(sym->type), \
								make_lgvw(symname));
						}
						else if (sym->stg_cls==EXTERN) {
							char *symname=SYMSTR(sym->name);

							emitCode("%s *%s=thrMPI_getval(%s);", make_typedef(sym->type), \
								make_lgvw(symname), make_gvkey(symname));
						}
						else {
							yyerror("Internal error: not global variable.");
							exit(-1);
						}
					}
					else {
						/* initialize top level global data, including allocating space */
						if (sym->initstr) {
							sym_init(sym, str_string(sym->initstr)); 
							del_str(sym->initstr);
							sym->initstr=NULL;
						}
						else
							sym_init(sym, NULL); 
					}
					/* for local defined static data, do the initialization after */
					/* parsing all the declaration in the same block */
				}
				sym=sym->next;
			}
		}
		else {
			SYM *sym=$2;

			switch(last_stg_cls) {
			case AUTO: /* emitCode("auto "); */ break;
			case REGISTER: /* emitCode("register "); */ break;
			case EXTERN: emitCode("extern "); break;
			case STATIC: emitCode("static "); break; /* just in case */
			case TYPEDEF:
			case NONE: break;
			default: yyerror("Internal: last_stg_cls==ERR"); exit(-1); 
			}
			
			emitCode("%s\t", str_string($1));
			while (sym) {
				emitCode(str_string(sym->decltor));
				del_str(sym->decltor);
				sym->decltor=NULL;
				if (sym->initstr) {
					emitCode("=%s", str_string(sym->initstr));
					del_str(sym->initstr);
					sym->initstr=NULL;
				}
				if (sym->next)
					emitCode(", ");
				sym=sym->next;
			}
			if ($3)
				emitCode(" %s", str_string($3));
			emitCode(";");
		}

		$$=$2;
		ResetStates();
		del_str($1);
		if ($3)
			del_str($3);
		del_str($4);
		last_stg_cls=ERR; /* change */
     }
   ;

declaration_specifiers
	: storage_class_specifier
	| storage_class_specifier declaration_specifiers
	{
		$$ = str_append($1," ");
		$$ = str_cat($$,$2);
		del_str($2);
	}
	| type_specifier 
	{
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		look_for_type_specifier=FALSE;
		$$=$1;
	}
	| type_specifier declaration_specifiers
	{
		look_for_type_specifier=FALSE;
		$$ = str_append($1," ");
		$$ = str_cat($$,$2);
		del_str($2);
	}
	| type_qualifier
	{
		$$=$1;
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
	}
	| type_qualifier declaration_specifiers
	{
		$$ = str_append($1," ");
		$$ = str_cat($$,$2);
		del_str($2);
	}
	| tk_extension declaration_specifiers
	{
		del_str($1);
		$$ = $2;
	}
   ;

init_declarator_list
   : init_declarator
	{
		/*
			This is the proper place to insert the symbol to the symbol
			table, because in C, a variable declaration can have its 
			initializer referring to another variable declared in the 
			same declaration but appears earlier.
			E.g.
			int i, j=i;
			For more info, see sample test program lvar.c.
		*/
		/*
			However, if we are parsing the function header, we do
			not want to add the symbols to the symbol table because
			we already did so when parsing the parameter list.
		*/
		/***
		if (!is_within_funcdef)
			add_SYM($1);
		else 
			$1->isDup=FALSE;
		***/
		if ($1->isFunc==TRUE) {
			/* purge the parameter link */
			if ($1->next)
				discard_SYM_chain($1->next);

			/* purge the symname buffer space */
			sb_restore(symnames);
		}
		$1->next=NULL;
		/*yyerror("%s->decltor=%s", SYMSTR($$->name), str_string($$->decltor)); */
		add_SYM($1);
		$$=$1;
		/*yyerror("%s->decltor=%s", SYMSTR($$->name), str_string($$->decltor)); */
	}
   | init_declarator_list tk_comma init_declarator
   	{
		if ($3->isFunc==TRUE) {
			/* purge the parameter link */
			if ($3->next)
				discard_SYM_chain($3->next);

			/* purge the symname buffer space */
			sb_restore(symnames);
		}
		$3->next=$1;
		del_str($2);
		add_SYM($3);
		$$=$3;

		/***
		if (!is_within_funcdef)
			add_SYM($3);
		else 
			$1->isDup=FALSE;
		***/
      }
   ;

init_declarator
   : declarator
   | declarator tk_assign 
	{
		/*
			The following is necessary because the initializer is
			just an expression.
		*/
		$<states>$=saveStates();
		is_func_type_spec=UNKNOWN;
		is_const_decl_spec=UNKNOWN;
		look_for_type_specifier=TRUE;
		last_stg_cls=ERR;
		/* yyerror("last_stg_cls changed to ERR"); */
	}
	initializer
   {
			$1->initstr=$4;
			del_str($2);
			$$=$1;
			restoreStates($<states>3);
   }
   ;

storage_class_specifier
   : tk_typedef
      { 
		last_stg_cls = TYPEDEF; 
		$$=$1;
	}
   | tk_extern
      {
			last_stg_cls = EXTERN;
			del_str($1);
			$1=new_blankstr(); /* eat up the extern keyword */
			$$=$1;
	}
   | tk_static
	{
		last_stg_cls = STATIC;
		del_str($1);
		$1=new_blankstr(); /* eat up the extern keyword */
		$$=$1;
	}
   | tk_auto
      	{
		last_stg_cls = AUTO; 
		$$=$1;
	}
   | tk_register
      	{ 
		last_stg_cls = REGISTER; 
		$$=$1;
	}
   ;

type_specifier
   : tk_char 
	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		$$=$1;
	}
   | tk_short
	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		$$=$1;
	}
   | tk_int
	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		$$=$1;
	}
   | tk_long
	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		$$=$1;
	}
   | tk_signed
	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		$$=$1;
	}
   | tk_unsigned
	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		$$=$1;
	}
   | tk_float
	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		$$=$1;
	}
   | tk_double
	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		$$=$1;
	}
   | tk_void
	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		$$=$1;
	}
   | struct_or_union_specifier 
   	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		is_const_decl_spec=FALSE; /* not simple const */
		$$=$1;
	}
   | enum_specifier
	{
		is_func_type_spec=FALSE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		$$=$1;
	}
   | tk_type_name 
	{ 
		$$=new_str(SYMSTR($1->name));
		is_func_type_spec=$1->isFunc; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		/**
		 *			  	is_const_decl_spec
		 *					U	T	F
		 * isConst U   U  T  F
		 *			  T   T  T  F
		 *			  F   F  F  F
		 */
		if ((is_const_decl_spec==UNKNOWN)||($1->isConst==FALSE))
			is_const_decl_spec=$1->isConst;
	}
   ;

type_qualifier
	: tk_const 
	{
		$$=$1;
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		if (is_const_decl_spec!=FALSE)
			is_const_decl_spec=TRUE;
	}
	| tk_volatile
	{
		$$=$1;
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
		is_const_decl_spec=FALSE;
	}
	;

struct_or_union_specifier
   /*
      In the following rules, regardless of whether there was a tag,
      indicate that we are no longer looking for one by setting
      look_for_tag to FALSE.
   */
   : struct_or_union identifier tk_lcurly_brace 
	{ 
		look_for_tag = FALSE;  
		$1=str_append($1, " ");
		$1=str_append($1, last_ident);
		strIndent($1, LEVEL);
		$1=str_cat($1, $3);
		LEVEL++; /* necessary to generate indention in struct_declaration_list */
		del_str($3);
		$<states>$=saveStates();
		is_func_type_spec=UNKNOWN;
		is_const_decl_spec=UNKNOWN;
		look_for_type_specifier=TRUE;
		last_stg_cls=NONE; 
	} 
	struct_declaration_list
	{ restoreStates($<states>4); } 
	tk_rcurly_brace 
	{
		LEVEL--;
		$$ = str_cat($1,$5);
		strIndent($$, LEVEL);
		$$ = str_cat($$,$7);
		del_str($5);
		del_str($7);
	}
   | struct_or_union tk_lcurly_brace
	{ 
		look_for_tag = FALSE;  
		/* $1=str_append($1, ""); */
		strIndent($1, LEVEL);
		$1=str_cat($1, $2);
		LEVEL++; /* necessary to generate indention in struct_declaration_list */
		del_str($2);
		$<states>$=saveStates();
		is_func_type_spec=UNKNOWN;
		is_const_decl_spec=UNKNOWN;
		look_for_type_specifier=TRUE;
		last_stg_cls=NONE; 
	}
	struct_declaration_list 
	{ restoreStates($<states>3); } 
	tk_rcurly_brace
	{
		LEVEL--;
		$$ = str_cat($1,$4);
		strIndent($$, LEVEL);
		$$ = str_cat($$,$6);
		del_str($4);
		del_str($6);
	}
   | struct_or_union identifier
	{
		look_for_tag = FALSE;
		$$ = str_append($1," ");
		$$ = str_append($$,last_ident);
	}
   ;

struct_or_union
   /*
      Indicate that we are looking for a tag. This information is passed
      to the lexer. Since tags are in a separate name space from other
      identifiers, this helps the lexer determine whether a lexeme is an
      identifier or a type name.
   */
   : tk_struct
      	{ 
		$$=$1;
		look_for_tag = TRUE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
	}
   | tk_union
      	{ 
		$$=$1;
		look_for_tag = TRUE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
	}
   ;

struct_declaration_list
   : struct_declaration
   | struct_declaration_list struct_declaration
   	{
      	$$ = str_cat($1,$2);
			del_str($2);
      }
   ;

struct_declaration
   : specifier_qualifier_list struct_declarator_list opt_attribute tk_semicolon
     {
			$$=new_blankstr();
			strIndent($$, LEVEL);
       $$ = str_cat($$,$1);
       $$ = str_append($$," ");
      	$$ = str_cat($$,$2);
      	$$ = str_cat($$,$3);
      	$$ = str_cat($$,$4);
			del_str($1);
			del_str($2);
			if ($3)
				del_str($3);
			del_str($4);
		is_func_type_spec=UNKNOWN;
		is_const_decl_spec=UNKNOWN;
		look_for_type_specifier=TRUE;
     }
   ;

specifier_qualifier_list 
	: type_specifier specifier_qualifier_list
	{
		$$=str_append($1, " ");
		$$=str_cat($$, $2);
		del_str($2);
	}
	| type_qualifier specifier_qualifier_list
	{
		$$=str_append($1, " ");
		$$=str_cat($$, $2);
		del_str($2);
	}
	| type_specifier
	| type_qualifier
	| tk_extension specifier_qualifier_list
	{
		del_str($1);
		$$ = $2;
	}
	;

struct_declarator_list
   : struct_declarator
   | struct_declarator_list tk_comma struct_declarator
   	{
      	$$ = str_cat($1,$2);
         $$ = str_append($$," ");
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

struct_declarator
	/* parsing a struct_field is virtually the same as parsing a local */
	/* variable, the last_stg_cls will always be NONE.	So we can always */
	/* assume that the returned SYM structure is a new one. We can test */
	/* it by checking whether stg_cls==ERR */
   : declarator
		{ 
			if ($1->isFunc==TRUE) {
				if ($1->next)
					discard_SYM_chain($1->next);
				sb_restore(symnames);
			}
			$$=$1->decltor;
			$1->decltor=NULL;
			/* ignore isConst, isFunc */
			if ($1->initstr) {
				yyerror("struct/union field initialization not allowed!");
				exit(-1);
			}
			discard_SYM($1);
		}
   | tk_colon constant_expr
		{
			$$=str_cat($1, $2);
			del_str($2);
		}
   | declarator tk_colon constant_expr
   	{
			if ($1->isFunc==TRUE) {
				if ($1->next)
					discard_SYM_chain($1->next);
				sb_restore(symnames);
				yyerror("field width not allowed with function delcaration"); 
			}
			$$=$1->decltor;
			$1->decltor=NULL;
			/* ignore isConst, isFunc */
			if ($1->initstr) {
				yyerror("struct/union field initialization not allowed!");
				exit(-1);
			}
			discard_SYM($1);
      	$$ = str_cat($$,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

enum_specifier
	/*
		It is so arranged that during the parsing of the enumeration,
		last_stg_cls becomes ERR. This way, the lexer will assume it
		is now parsing an expression and will not create new SYM 
		structure.
	*/
	/* 
      In the following rules, regardless of whether there was a tag,
      indicate that we are no longer looking for one by setting
      look_for_tag to FALSE.
   */
   : enum 
	{
		$<states>$=saveStates();
		last_stg_cls=ERR;
		/* yyerror("last_stg_cls changed to ERR"); */
	}
	enumeration
      {
			restoreStates($<states>2);
      	look_for_tag = FALSE;
       $$ = str_append($1," ");
      	$$ = str_cat($$,$3);
			del_str($3);
      }
   | enum identifier
    {
		$<states>$=saveStates();
		last_stg_cls=ERR;
		/* yyerror("last_stg_cls changed to ERR"); */
			look_for_tag = FALSE; 
       $1 = str_append($1," ");
      	$1 = str_append($1,last_ident);
		}
      enumeration
   	{
			restoreStates($<states>3);
       $$ = str_append($1," ");
      	$$ = str_cat($$,$4);
			del_str($4);
    }
   | enum identifier
      {
      	look_for_tag = FALSE;
       $$ = str_append($1," ");
      	$$ = str_append($$,last_ident);
      }
   ;

enumeration
   /*
      Note that an enumerator list can have a trailing comma. This is
      not Standard C, but some C compilers allow it anyway.
   */
   : tk_lcurly_brace enumerator_list tk_rcurly_brace
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   | tk_lcurly_brace enumerator_list tk_comma tk_rcurly_brace
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
      	$$ = str_cat($$,$4);
			del_str($2);
			del_str($3);
			del_str($4);
      }
   ;

enum
   /*
      Indicate that we are looking for a tag. See struct_or_union for
      more info.
   */
   : tk_enum 
   	{ 
		$$=$1;
		look_for_tag = TRUE; 
		if (last_stg_cls==ERR)
			last_stg_cls=NONE;
	}
   ;

enumerator_list
   : enumerator
   | enumerator_list tk_comma enumerator
   	{
      	$$ = str_cat($1,$2);
         $$ = str_append($$," ");
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

enumerator
   : identifier
	{
		/* just ignore the returned SYM pointer */
		$$=new_str(last_ident);
	}
   | identifier tk_assign constant_expr
	{
		$$=new_str(last_ident);
		$$=str_cat($$,$2);
		$$=str_cat($$,$3);
		del_str($2);
		del_str($3);
	}
   ;

declarator
   : declarator2
   | pointer declarator2
   	{
      	$1=str_append($1," ");
      	$1=str_cat($1,$2->decltor);
		del_str($2->decltor);
		$2->decltor=$1;

		if ($2->isFunc==UNKNOWN)
			$2->isFunc=FALSE;
		if ( ($2->isConst==UNKNOWN) )
			$2->isConst=FALSE;

		$$ = $2;
      }
   ;

func_decl_intro
	: /* empty */
	{ 
		LEVEL++;
		$$=saveStates();
		is_func_type_spec=UNKNOWN;
		is_const_decl_spec=UNKNOWN;
		look_for_type_specifier=TRUE;
		last_stg_cls=NONE;
		sb_newframe(symnames);
	}
	;

declarator2
   : identifier
	{
		if (!(isNewSYM($1))) {
			yyerror("Warning: expecting a new SYM when parsing a declarator. Possible redeclaration of a previously delared symbol");
			$1=new_SYM(last_ident, LEVEL); /* quick and dirty recovery */
			/* yyerror("Internal: recovered"); */
		}
		$1->LEVEL=LEVEL;
		tbool=toBeTranslated();
		if ( (!tbool) && (isGlobalDecl) )
			$1->stg_cls=SYS;
		else
			$1->stg_cls=last_stg_cls;

		if (isGlobalDecl && tbool ) {
			/* assign type and wraptype */
			$1->type=type_counter++;
			$1->wrap_type=type_counter++;
			$1->decltor=new_str(make_typedef($1->type));
			/* yyerror("%d->decltor=%s", SYMSTR($1->name), str_string($1->decltor)); */
		}
		else {
			$1->decltor=new_str(SYMSTR($1->name));
		}
		$$=$1;
	}
   | tk_lparen declarator tk_rparen
   	{
			$1=str_cat($1, $2->decltor);
      	$1=str_cat($1,$3);
			del_str($3);
			del_str($2->decltor);
			$2->decltor=$1;
			$$=$2;
      }
   | declarator2 tk_lsquare_brace tk_rsquare_brace
   	{
			$1->decltor=str_cat($1->decltor, $2);
			$1->decltor=str_cat($1->decltor, $3);
			if ($1->isFunc==UNKNOWN) 
				$1->isFunc=FALSE;
			/**
			 * although a pointer is a constant, we need to
			 * to decide based on the elements it contains
			 * rather than the variable itself.
			 */
			/**
			if ($1->isConst==UNKNOWN) 
				$1->isConst=TRUE;
			**/
			del_str($2);
			del_str($3);
			$$=$1;
      }
   | declarator2 tk_lsquare_brace 
	{
		$<states>$=saveStates();
		last_stg_cls=ERR;
		/* yyerror("last_stg_cls changed to ERR"); */
	}
	constant_expr 
	{
		restoreStates($<states>3);
	}
	tk_rsquare_brace
	{
		$1->decltor=str_cat($1->decltor, $2);
		$1->decltor=str_cat($1->decltor, $4);
		$1->decltor=str_cat($1->decltor, $6);
		if ($1->isFunc==UNKNOWN) 
			$1->isFunc=FALSE;
		/**
		if ($1->isConst==UNKNOWN) 
			$1->isConst=TRUE;
		**/
		del_str($2);
		del_str($4);
		del_str($6);
		$$=$1;
	}
	/* 
		for the rest three rules, we need to distinguish the situation when we
		are parsing a function definition header or a function declaration via
		flag is_within_funcdef.
		Be careful in the following situation:
		int f(int  (*f)(int x))
		{ ... }

		int *f()(int x)
		{ ... }
		In both cases, the parameter_list containing x is not the REAL parameter
		list we desire of.
	*/
   | declarator2 tk_lparen tk_rparen
	{
		$1->decltor=str_cat($1->decltor, $2);
		$1->decltor=str_cat($1->decltor, $3);
		del_str($2);
		del_str($3);

		/***
		if ( (is_within_funcdef==TRUE) && (LEVEL==0) &&
			($1->isFunc==UNKNOWN) ) { 
				sb_newframe(symnames);
		}
		***/
		$1->next=NULL;
		if ($1->isFunc==UNKNOWN) {
			$1->isFunc=TRUE;
			sb_newframe(symnames);
		}
		$1->isConst=FALSE;	
		$$=$1;
	}
   | declarator2 tk_lparen func_decl_intro parameter_type_list tk_rparen
	{
		$4=reverse_symlink($4);
		LEVEL--;
		merge_paralist($1, $2, $4, $5);
		/******
		if ( (is_within_funcdef==TRUE) && (LEVEL==0) &&
			($1->isFunc==UNKNOWN) ) { 
			$1->next=$4;
		}
		else {
			discard_SYM_chain($4);

			sb_restore(symnames);
		}
		***/

		if ($1->isFunc==UNKNOWN) {
			$1->isFunc=TRUE;
			$1->next=$4;
		}
		else {
			discard_SYM_chain($4);
			sb_restore(symnames);
		}
		$1->isConst=FALSE;	
		$$=$1;

		restoreStates($3);
	}
	| declarator2 tk_lparen func_decl_intro parameter_identifier_list tk_rparen
	{
		$4=reverse_symlink($4);
		LEVEL--;
		merge_paralist($1, $2, $4, $5);
		/*****
		if ( (is_within_funcdef==TRUE) && (LEVEL==0) &&
			($1->isFunc==UNKNOWN) ) { 
			$1->next=$4;
		}
		else {
			discard_SYM_chain($4);

			sb_restore(symnames);
		}
		****/

		if ($1->isFunc==UNKNOWN) {
			$1->isFunc=TRUE;
			$1->next=$4;
		}
		else {
			discard_SYM_chain($4);
			sb_restore(symnames);
		}
		$1->isConst=FALSE;	
		
		$$=$1;
		restoreStates($3);
	}
   ;

pointer
   : tk_asterisk
   | tk_asterisk type_qualifier_list
   	{
      	$$ = str_cat($1,$2);
			del_str($2);
      }
   | tk_asterisk pointer
   	{
      	$$ = str_cat($1,$2);
			del_str($2);
      }
   | tk_asterisk type_qualifier_list pointer
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   ;

type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
	{
		$$=str_append($1, " ");
		$$=str_cat($$,$2);
		del_str($2);
	}
	;

type_specifier_list
   : type_specifier
   | type_specifier_list type_specifier
	{
		$$ = str_append($1," ");
		$$ = str_cat($$,$2);
		del_str($2);
	}
   ;

parameter_identifier_list
	: identifier_list
	| identifier_list tk_comma tk_elipsis
	{
		$1->decltor=str_cat($1->decltor, $2);
		$1->decltor=str_append($1->decltor, " ");
		$1->decltor=str_cat($1->decltor, $3);

		del_str($2);
		del_str($3);
		$$=$1;
	}
   ;

identifier_list
   : identifier
	{
		/* need to make $1 look like a common variable with type */
		
		$1->next=NULL;
		$$=$1;
		$$->isConst=FALSE;
		$$->isFunc=FALSE;
		$$->decltor=new_str(SYMSTR($$->name));
		$$->stg_cls=NONE;

		$$->LEVEL=LEVEL;
		/* assuming they will not be put into sym_tab */
		/* why? */
		$$->isDup=TRUE; 
	}
   | identifier_list tk_comma identifier
	{
		$3->next=$1;
		$$=$3;
		$$->isConst=FALSE;
		$$->isFunc=FALSE;
		$$->decltor=new_str(SYMSTR($$->name));
		$$->stg_cls=NONE;

		$$->LEVEL=LEVEL;

		/* assuming they will not be put into sym_tab */
		/* why? */
		$$->isDup=TRUE; 

		del_str($2);
	}
   ;

parameter_type_list
   : parameter_list
   | parameter_list tk_comma tk_elipsis
	{
		$1->decltor=str_cat($1->decltor, $2);
		$1->decltor=str_append($1->decltor, " ");
		$1->decltor=str_cat($1->decltor, $3);
		del_str($2);
		del_str($3);
		$$=$1;
	}
   ;

parameter_list
   : parameter_declaration
	{
		$1->next=NULL;
		$$=$1;

		/* assuming they will not be put into sym_tab */
		$$->isDup=TRUE; 

		is_func_type_spec=UNKNOWN;
		is_const_decl_spec=UNKNOWN;
		look_for_type_specifier=TRUE;
		last_stg_cls=NONE; 
	}
   | parameter_list tk_comma parameter_declaration
	{
		$3->next=$1;
		$$=$3;

		/* assuming they will not be put into sym_tab */
		$$->isDup=TRUE; 

		is_func_type_spec=UNKNOWN;
		is_const_decl_spec=UNKNOWN;
		look_for_type_specifier=TRUE;
		last_stg_cls=NONE; 

		del_str($2);
	}
   ;

parameter_declaration
	: declaration_specifiers declarator
	{
		if ($2->isFunc==TRUE) {
			if ($2->next)
				discard_SYM_chain($2->next);
			sb_restore(symnames);
		}
		$1=str_append($1, " ");
		$1=str_cat($1, $2->decltor);
		del_str($2->decltor);
		$2->decltor=$1;
		$$=$2;
		if ($$->isFunc==UNKNOWN)
			$$->isFunc=is_func_type_spec;
		if ($$->isConst==UNKNOWN)
			$$->isConst=is_const_decl_spec;

		$$->stg_cls=NONE;
	}
   | declaration_specifiers
	{
		$$=new_SYM("$$$", LEVEL);
		$$->decltor=$1;
		$$->isFunc=is_func_type_spec;
		$$->isConst=is_const_decl_spec;
		$$->stg_cls=NONE;
	}
   | declaration_specifiers abstract_declarator
	{
		$1=str_append($1, " ");
		$1=str_cat($1, $2);
		del_str($2);
		$$=new_SYM("$$$", LEVEL);
		$$->decltor=$1;
		$$->isFunc=is_func_type_spec;
		$$->isConst=is_const_decl_spec;
		$$->stg_cls=NONE;
	}
   ;

type_name
   : type_specifier_list
	{
		last_stg_cls=ERR;
	}
   | type_specifier_list abstract_declarator
   {
		$$ = str_append($1," ");
		$$ = str_cat($$,$2);
		del_str($2);
		last_stg_cls=ERR;
	}
   ;

abstract_declarator
   : pointer
   | direct_abstract_declarator
   | pointer direct_abstract_declarator
 	{
		$$=str_append($1, " ");
		$$ = str_cat($$,$2);
		del_str($2);
	}
   ;

direct_abstract_declarator
   : tk_lparen abstract_declarator tk_rparen
   	{
      	$$ = str_cat($1,$2);
      	$$ = str_cat($$,$3);
			del_str($2);
			del_str($3);
      }
   | tk_lsquare_brace tk_rsquare_brace
   	{
      	$$ = str_cat($1,$2);
			del_str($2);
      }
   | tk_lsquare_brace 
	{
		$<states>$=saveStates();
		last_stg_cls=ERR;
		/* yyerror("last_stg_cls changed to ERR"); */
	}
	constant_expr 
	{
		restoreStates($<states>2);
	}
	tk_rsquare_brace
   {
     	$$ = str_cat($1,$3);
     	$$ = str_cat($$,$5);
		del_str($3);
		del_str($5);
	}
   | direct_abstract_declarator tk_lsquare_brace tk_rsquare_brace
	{
		$$ = str_cat($1,$2);
		$$ = str_cat($$,$3);
		del_str($2);
		del_str($3);
	}
   | direct_abstract_declarator tk_lsquare_brace 
	{
		$<states>$=saveStates();
		last_stg_cls=ERR;
		/* yyerror("last_stg_cls changed to ERR"); */
	}
	constant_expr 
	{
		restoreStates($<states>3);
	}
	tk_rsquare_brace
	{
		$$ = str_cat($1,$2);
		$$ = str_cat($$,$4);
		$$ = str_cat($$,$6);
		del_str($2);
		del_str($4);
		del_str($6);
	}
   | tk_lparen tk_rparen
	{
		$$ = str_cat($1,$2);
		del_str($2);
	}
   | tk_lparen func_decl_intro parameter_type_list tk_rparen
	{
		$3=reverse_symlink($3);
		LEVEL--;
		$$=merge_paralist2(NULL, $1, $3, $4);

		discard_SYM_chain($3);

		/* purge the symname buffer space */
		sb_restore(symnames);

		restoreStates($2);
	}
	| direct_abstract_declarator tk_lparen tk_rparen
	{
		$$ = str_cat($1,$2);
		$$ = str_cat($$,$3);
		del_str($2);
		del_str($3);
	}
   | direct_abstract_declarator tk_lparen func_decl_intro parameter_type_list tk_rparen
	{
		$4=reverse_symlink($4);
		LEVEL--;
		$$=merge_paralist2($1, $2, $4, $5);

		discard_SYM_chain($4);

		/* purge the symname buffer space */
		sb_restore(symnames);

		restoreStates($3);
	}
   ;

initializer
   : assignment_expr
   | tk_lcurly_brace initializer_list tk_rcurly_brace
	{
		$$ = str_cat($1,$2);
		$$ = str_cat($$,$3);
		del_str($2);
		del_str($3);
	}
   | tk_lcurly_brace initializer_list tk_comma tk_rcurly_brace
	{
		$$ = str_cat($1,$2);
		$$ = str_cat($$,$3);
		$$ = str_cat($$,$4);
		del_str($2);
		del_str($3);
		del_str($4);
	}
   ;

initializer_list
   : initializer
   | initializer_list tk_comma initializer
 	{
    	$$ = str_cat($1,$2);
		$$ = str_append($$," ");
		$$ = str_cat($$,$3);
		del_str($2);
		del_str($3);
	}
   ;

statement
   : labeled_statement
   | compound_statement
   | expression_statement
   | selection_statement
   | iteration_statement
   | jump_statement
   ;

labeled_statement
   : identifier  tk_colon
	{
		setIndent(LEVEL-1);
		emitCode("%s%s", last_ident, str_string($2));
		del_str($2);
	}
   statement
   | tk_case constant_expr tk_colon
	{
		setIndent(LEVEL-1);
		emitCode("%s %s%s", str_string($1), str_string($2), str_string($3));
		del_str($1);
		del_str($2);
		del_str($3);
	}
   statement
   | tk_default tk_colon 
	{
		setIndent(LEVEL-1);
		emitCode("%s%s", str_string($1), str_string($2));
		del_str($1);
		del_str($2);
	}
	statement
   ;

compound_statement_intro1: tk_lcurly_brace
	{
		setIndent(LEVEL);
		emitCode("%s", str_string($1));
		del_str($1);
		LEVEL++;
		sb_newframe(symnames);
		ResetStates();
		last_stg_cls=ERR;
	}	
	;

compound_statement_intro2: tk_lcurly_brace
	{
		setIndent(LEVEL);
		emitCode("%s", str_string($1));
		del_str($1);
		LEVEL++;
		sb_newframe(symnames);
		ResetStates();
		last_stg_cls=NONE;
	}	
	;

compound_statement
   : tk_lcurly_brace tk_rcurly_brace
	{
		setIndent(LEVEL-1);
		emitCode("%s", str_string($1));
		setIndent(LEVEL-1);
		emitCode("%s", str_string($2));
		del_str($1);
		del_str($2);
	}
   | compound_statement_intro1 statement_list tk_rcurly_brace
	{
		LEVEL--;
		setIndent(LEVEL);
		emitCode("%s", str_string($3));
		del_str($3);
		sb_restore(symnames);
	}
   | compound_statement_intro2 declaration_list tk_rcurly_brace
	{
		LEVEL--;
		setIndent(LEVEL);
		emitCode("%s", str_string($3));
		del_str($3);
		discard_SYM_chain($2);
		sb_restore(symnames);
		last_stg_cls=ERR;
	}
   | compound_statement_intro2 declaration_list 
	{
		if (toBeTranslated()) {
			/* dumpSYMList($2); */
			init_localstatic($2);
		}
		emitCode("\n");
		last_stg_cls=ERR;
	}
	statement_list tk_rcurly_brace
   {
		LEVEL--;
		setIndent(LEVEL);
		emitCode("%s", str_string($5));
		del_str($5);
		discard_SYM_chain($2);
		sb_restore(symnames);
	}
	;

declaration_list
   : declaration
   {
   		$$=$1;
   		/* dumpSYMList($$); */
   }
   | declaration_list declaration
	{
		/* note declaration is a list */
		if ($2) {
			SYM *s=$1;

			while (s->next) {
				s=s->next;
			}
			s->next=$2;

		}
		$$=$1;
   		/* dumpSYMList($$); */
	}
   ;

statement_list
   : statement
   | statement_list statement
   ;

expression_statement
   : tk_semicolon
	{
		setIndent(LEVEL);
		emitCode("%s", str_string($1));
		del_str($1);
	}
   | expr tk_semicolon
	{
		setIndent(LEVEL);
		emitCode("%s%s", str_string($1), str_string($2));
		del_str($1);
		del_str($2);
	}
   ;

selection_statement
   : tk_if tk_lparen expr tk_rparen
	{
		setIndent(LEVEL);
		emitCode("%s %s %s %s", str_string($1),str_string($2),str_string($3),\
			str_string($4));
		del_str($1);
		del_str($2);
		del_str($3);
		del_str($4);
		LEVEL++;
	}
	statement 
	{
		LEVEL--;
	}
	opt_else
   | tk_switch tk_lparen expr tk_rparen
	{
		setIndent(LEVEL);
		emitCode("%s %s %s %s", str_string($1),str_string($2),str_string($3),\
			str_string($4));
		del_str($1);
		del_str($2);
		del_str($3);
		del_str($4);
		LEVEL++;
	}
	statement
	{
		LEVEL--;
	}
	;

opt_else
   :                              %prec THEN
   | tk_else
	{
		setIndent(LEVEL);
		emitCode("%s", str_string($1));
		LEVEL++;
	}
	statement
	{
		LEVEL--;
	}
	;

iteration_statement
	: tk_while tk_lparen expr tk_rparen
	{
		setIndent(LEVEL);
		emitCode("%s %s %s %s", str_string($1),str_string($2),str_string($3),\
			str_string($4));
		del_str($1);
		del_str($2);
		del_str($3);
		del_str($4);
		LEVEL++;
	}
	statement
	{
		LEVEL--;
	}
	| tk_do
	{
		setIndent(LEVEL);
		emitCode("%s", str_string($1));
		del_str($1);
		LEVEL++;
	}
	statement
	{ 
		LEVEL--;
	}
	tk_while tk_lparen expr tk_rparen tk_semicolon
	{
		setIndent(LEVEL);
		emitCode("%s %s %s %s%s", str_string($5),str_string($6),str_string($7),\
			str_string($8), str_string($9));
		del_str($5);
		del_str($6);
		del_str($7);
		del_str($8);
		del_str($9);
	}
	| tk_for tk_lparen opt_expr tk_semicolon opt_expr tk_semicolon opt_expr tk_rparen
	{
		setIndent(LEVEL);
		emitCode("%s %s %s%s %s%s %s %s", str_string($1),str_string($2),str_string($3),\
			str_string($4), str_string($5), str_string($6), str_string($7), str_string($8));
		del_str($1);
		del_str($2);
		del_str($3);
		del_str($4);
		del_str($5);
		del_str($6);
		del_str($7);
		del_str($8);
		LEVEL++;
	}
	statement
	{
		LEVEL--;
	}
	;

opt_expr
	: { $$ = new_str(" "); }
	| expr
   ;

jump_statement
	: tk_goto identifier { look_for_tag=FALSE; } tk_semicolon
	{
		setIndent(LEVEL);
		emitCode("%s %s%s", str_string($1),last_ident,str_string($4));
		del_str($1);
		del_str($4);
	}
	| tk_continue tk_semicolon
	{
		setIndent(LEVEL);
		emitCode("%s%s", str_string($1),str_string($2));
		del_str($1);
		del_str($2);
	}
	| tk_break tk_semicolon
	{
		setIndent(LEVEL);
		emitCode("%s%s", str_string($1),str_string($2));
		del_str($1);
		del_str($2);
	}
	| tk_return opt_expr tk_semicolon
	{
		setIndent(LEVEL);
		emitCode("%s %s%s", str_string($1),str_string($2),str_string($3));
		del_str($1);
		del_str($2);
		del_str($3);
	}
	;

program
	: translation_unit
	{
		/* need to clean up the translation_unit list */
		/* emitCode("\n\n# define nSKEY %d", staticKEY); */
	}
	;

translation_unit
	: external_declaration
	| translation_unit external_declaration
	{
		if ($2) {
			SYM *s=$1;

			while (s->next) {
				s=s->next;
			}
			s->next=$2;
		}

		$$=$1;
	}
   ;

external_declaration
   : function_definition
	{
		/* because a function definition can start without any specifiers */
		last_stg_cls=NONE;
	}
   | declaration
	{
		/* because a function definition can start without any specifiers */
		last_stg_cls=NONE;
	}
   ;

function_definition
   : { last_stg_cls=NONE; } declarator
	{
		if ($2->isFunc!=TRUE) {
			yyerror("Invalid function definition.");
			exit(-1);
		}
		fixFuncName($2); 	/* replace the first identifier in the */
					/* to be the real name instead of a type name */
		emitCode("\n\n%s", str_string($2->decltor));
		/* in function_body, the function name can be referenced */
		add_SYM($2);
		if ($2->next) { /* put the parameter list into sym_tab */
			add_SYMs_to_table($2->next);
		}
	}
   function_body
  	{
		$$=$2;
		if ($2->next) { /* release the parameter list */
			discard_SYM_chain($2->next);				
			$2->next=NULL;
		}

		/* inserrt code to INS using LGL list */
		if (toBeTranslated())
			lgvw_gen();
		ResetStates();
   }
   | declaration_specifiers declarator
   { 
		if ($2->isFunc!=TRUE) {
			yyerror("Invalid function definition.");
			exit(-1);
		}
		fixFuncName($2); 	/* replace the first identifier in the */
					/* to be the real name instead of a type name */

		emitCode("\n\n%s %s", str_string($1), str_string($2->decltor));
		del_str($1);
		add_SYM($2);
		if ($2->next) { /* put the parameter list into sym_tab */
			add_SYMs_to_table($2->next);
		}
	}
   function_body
  	{
		$$=$2;
		if ($2->next) { /* release the parameter list */
			discard_SYM_chain($2->next);				
			$2->next=NULL;
		}

		if (toBeTranslated())
			lgvw_gen();
		ResetStates();
   }
   ;

lgv_insert1: tk_lcurly_brace 
	{
		LEVEL++;
		emitCode("\n%s", str_string($1));
		if (toBeTranslated())
			lgv_include();
		del_str($1);

		ResetStates();
		last_stg_cls=ERR;
	}
	;

lgv_insert2: tk_lcurly_brace 
	{
		LEVEL++;
		emitCode("\n%s", str_string($1));
		if (toBeTranslated())
			lgv_include();
		del_str($1);

		ResetStates();
		last_stg_cls=NONE;
	}
	;

stg_none
	: /* empty */
	{
		last_stg_cls=NONE;
	}
	;

func_compound_statement
   : lgv_insert1 stg_none tk_rcurly_brace
	{
		LEVEL--;
		emitCode("\n%s", str_string($3));
		del_str($3);
	}
   | lgv_insert1 statement_list stg_none tk_rcurly_brace
	{
		LEVEL--;
		emitCode("\n%s", str_string($4));
		del_str($4);
	}
   | lgv_insert2 declaration_list stg_none tk_rcurly_brace
	{
		LEVEL--;
		emitCode("\n%s", str_string($4));
		del_str($4);
		/* discard the declaration_list */
		discard_SYM_chain($2);
	}
   | lgv_insert2 declaration_list 
	{
		/* create space for static variables */
		if (toBeTranslated()) {
			/* dumpSYMList($2); */
			init_localstatic($2);
		}
		emitCode("\n");
		last_stg_cls=ERR;
	}
		statement_list stg_none tk_rcurly_brace
	{
		LEVEL--;
		emitCode("\n%s", str_string($6));
		del_str($6);
		/* discard the declaration_list */
		discard_SYM_chain($2);
	}
   ;

function_body
   : func_compound_statement
	{
		sb_restore(symnames);
	}
   | 
   {
		sb_newframe(symnames);
		/* create a new scope so that the lexer will generate new SYMs */
		LEVEL++; 
		LEVEL++; 
		/* since is_within_funcdef must be TRUE at this point, */
		/* the following SYM list will not be put into sym_tab.*/

		ResetStates();
		last_stg_cls=NONE;
	}
	declaration_list 
	{
		LEVEL--;
		LEVEL--;

		sb_restore(symnames);

		/* 
			I don't care the symbol link. I just want the declaration 
			to be outputted.
		*/
		discard_SYM_chain($2);
	}
	func_compound_statement
	{
		sb_restore(symnames);
	}
   ;

%%
/* Functions. */

static STR * SafeName(STR *str)
{
	char *Table[][2]= {
		{ "main", "tmain"},
		{ "exit", "thrMPI_exit"},
		{ NULL, NULL}
	};

	int i;

	for (i=0; Table[i][0]; i++) {
		if (strcmp(str_string(str), Table[i][0])==0) {
			del_str(str);
			return new_str(Table[i][1]);
		}
	}

	return str;
}

static void ResetStates(void)
{
	/* reset the states after parsing one declaration list or function def*/
		/* last_stg_cls=ERR; */
		/* yyerror("last_stg_cls changed to ERR"); */
		is_const_decl_spec=UNKNOWN;
		is_func_type_spec=FALSE;
		look_for_type_specifier=TRUE;
}

static SYM * ParseDeclList(SYM *list)
{
	SYM *sym;

	sym=list=reverse_symlink(list);

	while (sym) {
		if (sym->isFunc==UNKNOWN)
			sym->isFunc=is_func_type_spec;
		if (sym->isConst==UNKNOWN)
			sym->isConst=is_const_decl_spec;
		sym=sym->next;
	}

	return list;
}

/**
 * The following fields for SYM are filled during the parsing of
 * the declarator: stg_cls, initstr, decltor, type, wraptype; the 
 * following fields may be filled during the parsing of the 
 * declarator: isFunc, isConst; the following field is filled 
 * during the creation of the SYM structure: LEVEL, name.
 */

/**
 * Same: add "typedef", create wrap structures, create keys.
 * Ignore simple constants and function declarations.
 * Different: for top level declarations, add key creation (
 * for external var) and initialization code in separate functions.
 * For inside function global declarations,  add lgvw creation (
 * for static var) and initialization code after the declaration
 * list for the current block is finished.
 */

/**
 * enum constants are not put into the symbol table. 
 * So it is possible that a none existing symbol to 
 * be an expr alone. 
 */

/**
 * To do: insert strbuf frame creation/restoration code
 * to recycle space. Done!
 */

static void merge_paralist(SYM *decltor, STR *lp, SYM *paralist, STR *rp)
{
	SYM *para=paralist;

	while (para) {
		lp=str_cat(lp, para->decltor);
		del_str(para->decltor);
		para->decltor=NULL;
		if (para->next)
			lp=str_append(lp, ", ");
		para=para->next;
	}
	lp=str_cat(lp, rp);
	del_str(rp);

	decltor->decltor=str_cat(decltor->decltor, lp);
	del_str(lp);

	return;
}

/* another merge function */
static STR *merge_paralist2(STR *opt, STR *lp, SYM *paralist, STR *rp)
{
	SYM *para=paralist;

	while (para) {
		lp=str_cat(lp, para->decltor);
		del_str(para->decltor);
		para->decltor=NULL;
		if (para->next)
			lp=str_append(lp, ", ");
		para=para->next;
	}
	lp=str_cat(lp, rp);
	del_str(rp);
	if (opt) { 
		opt=str_cat(opt, lp);
		del_str(lp);
	}
	else
		opt=lp;

	return opt;
}

/* 
	This will generate an encoding of the following flags: 
	0:1		is_func_type_spec=UNKNOWN;
	2:3		is_const_decl_spec=UNKNOWN;
	4:5		look_for_type_specifier=TRUE;
	6:7		is_within_funcdef=FALSE;
	8:11		last_stg_cls;
	
*/
static unsigned int saveStates()
{
	return is_func_type_spec|(is_const_decl_spec<<2)|
			 (look_for_type_specifier<<4)|
			 /* (is_within_funcdef<<6)| */
			 (last_stg_cls<<8);
}

/* This decode the states */
static void restoreStates(unsigned int states)
{
	const int M1=3;
	const int M2=12;
	const int M3=48;
	/* const int M4=192; */
	const int M5=15*256;

	is_func_type_spec=(states&M1);
	is_const_decl_spec=(states&M2)>>2;
	look_for_type_specifier=(states&M3)>>4;
	/* is_within_funcdef=(states&M4)>>6; */
	last_stg_cls=(states&M5)>>8;
/***
	if (last_stg_cls==AUTO)
		yyerror("auto found");
	if (last_stg_cls==ERR)
		yyerror("last_stg_cls changed to ERR 1");

****/
	return;
}

/* change ... _TYPE... (...) to ... <realname> (...) */
static void fixFuncName(SYM *funcdecl)
{
# define MAXLEN 2000
	static char buf[MAXLEN+1];
	char *str=str_string(funcdecl->decltor);
	int i;
	STR *tmp;
	
	/* buf[0]='\0'; */
	/* find the beginning of the name */
	for (i=0; (str[i])&&(i<MAXLEN); i++) {
		if  (	(str[i]=='_') || isalpha(str[i]) )
			break;

		buf[i]=str[i];
	}

	buf[i]='\0';

	if ( (i==MAXLEN)||(str[i]=='\0') ) {
		yyerror("Declaration error (may be too long)!");
		exit(-1);
	}

	tmp=new_str(SYMSTR(funcdecl->name));

	if (toBeTranslated()) {
		tmp=SafeName(tmp);
	}

	strncat(buf, str_string(tmp), MAXLEN-strlen(buf)-1);
	del_str(tmp);

	/* find the end of the name */
	for (;(str[i])&&(i<MAXLEN); i++) {
		if  ( (str[i]=='_') || isalnum(str[i]) )
			;
		else
			break;
	}

	if (strlen(str+i)>(MAXLEN-strlen(buf)-1)) {
		yyerror("Extraordinarily long declaration!");
		exit(-1);
	}
		
	strcat(buf, str+i);

	del_str(funcdecl->decltor);
	funcdecl->decltor=new_str(buf);

	return;
# undef MAXLEN
}

static Bool toBeTranslated()
{
	static Bool last;
	static char lastFileName[MAX_FILENAME+1]="";
	int i=0;

	if (strcmp(filename, lastFileName)==0) 
		return last;

	last=FALSE;

	for (;i<100;i++) {
		if (strcmp(filename, userFile[i])==0) { 
			last=TRUE;
			break;
		}
	}

	strcpy(lastFileName, filename);

	return last;
}

static void disptok(char *token)
{
	if (token)
		fprintf(stderr, token);
	else 
		fprintf(stderr, "un-catched tokens");
}

static void yyprint1(type)
{

	switch (type) {
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

		default:disptok(NULL); printf("token=%d\n",type);
	}

	return;
}

static Bool isFortranFormatString(char *name)
{
	if ( (name[0]=='f') && (name[1]=='m') && (name[2]=='t') && (name[3]=='_') ) {
		name+=4;
		while (*name) {
			if (!isdigit(*name))
				return FALSE;
			name++;
		}
		return TRUE;
	}
	return FALSE;
}

static Bool isSpecial(SYM *sym)
{
	/* actually we need to add a switch to disable special handling */
	char *name=SYMSTR(sym->name);

	if (isFortranFormatString(name)) {
		sym->isConst=TRUE;
		return TRUE;
	}

	/* more special cases here */
	return FALSE;
}
