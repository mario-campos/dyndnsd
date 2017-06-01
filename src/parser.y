%{
#include <stddef.h>
#include "ast.h"
extern int yylex();
extern int yyerror();
%}

%union {
	char              *str;
	struct ast        *ast;
	struct ast_iface  *ast_iface;
	struct ast_domain *ast_domain;
	struct ast_param  *ast_param;
}

%token INTERFACE
%token DOMAIN
%token HTTP_GET
%token <str> STRING

%type <ast> configuration
%type <ast_iface> interfaces interface
%type <ast_domain> domains domain
%type <ast_param> parameter

%locations
%parse-param {struct ast **ast}

%%

configuration
	: interfaces					{ *ast = new_ast($1, NULL); }
	| parameter interfaces				{ *ast = new_ast($2, $1); }
	;

interfaces
	: interface
	| interfaces interface				{ $$ = $2; $$->next = $1; }
	;

interface
	: INTERFACE STRING '{' domains '}'		{ $$ = new_ast_iface($2, $4, NULL); }
	| INTERFACE STRING '{' parameter domains '}'	{ $$ = new_ast_iface($2, $5, $4); }
	;

domains : domain	
	| domains domain				{ $$ = $2; $$->next = $1; }
	;

domain	: DOMAIN STRING					{ $$ = new_ast_domain($2, NULL); }
	| DOMAIN STRING '{' parameter '}'		{ $$ = new_ast_domain($2, $4); }
	;

parameter 
	: HTTP_GET STRING				{ $$ = new_ast_param(HTTP_GET, $2); }
	;
