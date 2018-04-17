%{
#include "ast.h"
#include "cst.h"
#include "die.h"

extern int yylex();
extern int yyerror();
%}

%union {
	char            *string;
	struct cst_node *cst_node;
}

%token USER GROUP INTERFACE DOMAIN RUN
%token <string> STRING
%type <string> strings
%type <cst_node> config config_statements config_statement
%type <cst_node> interface interface_statements interface_statement
%type <cst_node> domain run '\n'

%%

config : config_statements				{
								struct cst_node *cst = cst_node(0, NULL, $1);
								*ast = cst2ast(cst);
								cst_free(cst);
							}
	;

config_statements
	: config_statement
	| config_statements config_statement		{ $$ = $2; SLIST_NEXT($2, next) = $1; }
	;

config_statement
	: '\n'						{ $$ = cst_node('\n', NULL, NULL); }
	| run
	| interface
	;

interface
	: INTERFACE STRING '{' interface_statements '}'	{ $$ = cst_node(INTERFACE, $2, $4); }
	;

interface_statements
	: interface_statement
	| interface_statements interface_statement	{ $$ = $2; SLIST_NEXT($2, next) = $1; }
	;

interface_statement
	: '\n'						{ $$ = cst_node('\n', NULL, NULL); }
	| domain
	;

domain	: DOMAIN STRING 				{ $$ = cst_node(DOMAIN, $2, NULL); }
	;

run	: RUN strings 		 			{ $$ = cst_node(RUN, $2, NULL); }
	;

strings	: STRING
	| strings STRING				{
								if (-1 == asprintf(&$$, "%s %s", $1, $2))
									die(LOG_CRIT, AT("asprintf(3)"));
								free($1);
								free($2);
							}
	;
