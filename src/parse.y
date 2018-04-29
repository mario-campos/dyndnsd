%{
#include "ast.h"
#include "cst.h"

extern int yylex();
extern int yyerror();
%}

%union {
	char            *string;
	struct cst_node *cst_node;
}

%token USER GROUP INTERFACE DOMAIN RUN
%token <string> STRING QUOTE
%type <cst_node> config config_statements config_statement
%type <cst_node> interface interface_statements interface_statement
%type <cst_node> domain run exprs expr '\n'

%%

config : config_statements				{
								struct cst_node *cst = cst_node(0, NULL, NULL, $1);
								ast = cst_convert(cst);
								cst_free(cst);
							}
	;

config_statements
	: config_statement
	| config_statements config_statement		{ $$ = $2; SLIST_NEXT($2, next) = $1; }
	;

config_statement
	: '\n'						{ $$ = cst_node('\n', NULL, NULL, NULL); }
	| run
	| interface
	;

interface
	: INTERFACE expr '{' interface_statements '}'	{ $$ = cst_node(INTERFACE, NULL, $2, $4); }
	;

interface_statements
	: interface_statement
	| interface_statements interface_statement	{ $$ = $2; SLIST_NEXT($2, next) = $1; }
	;

interface_statement
	: '\n'						{ $$ = cst_node('\n', NULL, NULL, NULL); }
	| domain
	;

domain	: DOMAIN expr					{ $$ = cst_node(DOMAIN, NULL, $2, NULL); }
	;

run	: RUN exprs					{ $$ = cst_node(RUN, NULL, $2, NULL); }
	;

exprs	: expr
	| exprs expr					{ $$ = $2; SLIST_NEXT($2, next) = $1; }
	;

expr	: STRING					{ $$ = cst_node(STRING, $1, NULL, NULL); }
	| QUOTE						{ $$ = cst_node(QUOTE, $1, NULL, NULL); }
	;
