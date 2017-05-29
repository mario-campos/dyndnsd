%{
#include <stddef.h>
#include "ast.h"
extern int yylex();
extern int yyerror();
%}

%union {
	char *str;
	struct ast *ast;
	struct ast_interface *ast_iface;
	struct ast_domain *ast_domain;
	struct ast_parameter *ast_param;
}

%token INTERFACE
%token DOMAIN
%token HTTP_GET
%token <str> STRING

%type <ast> configuration
%type <ast_iface> interfaces interface
%type <ast_domain> domains domain
%type <ast_param> parameter

%parse-param {struct ast *ast}

%%

configuration
	: interfaces					{ $$ = new_ast($1, NULL); }
	| parameter interfaces				{ $$ = new_ast($2, $1); }
	;

interfaces
	: interface
	| interfaces interface				{ $$ = $2; $$->next = $1; }
	;

interface
	: INTERFACE STRING '{' domains '}'		{ $$ = new_interface($2, $4, NULL); }
	| INTERFACE STRING '{' parameter domains '}'	{ $$ = new_interface($2, $5, $4); }
	;

domains : domain	
	| domains domain				{ $$ = $2; $$->next = $1; }
	;

domain	: DOMAIN STRING					{ $$ = new_domain($2, NULL); }
	| DOMAIN STRING '{' parameter '}'		{ $$ = new_domain($2, $4); }
	;

parameter 
	: HTTP_GET STRING				{ $$ = new_parameter(HTTP_GET, $2); }
	;
