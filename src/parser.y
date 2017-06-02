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
	struct ast_url    *ast_url;
}

%token INTERFACE
%token DOMAIN
%token HTTP_GET
%token <str> STRING

%type <ast> configuration
%type <ast_iface> interfaces interface
%type <ast_domain> domains domain
%type <ast_url> url 

%locations
%parse-param {struct ast **ast}

%%

configuration
	: interfaces					{ *ast = new_ast($1, NULL); }
	| url interfaces				{ *ast = new_ast($2, $1); }
	;

interfaces
	: interface
	| interfaces interface				{ $$ = $2; $$->next = $1; }
	;

interface
	: INTERFACE STRING '{' domains '}'		{ $$ = new_ast_iface($2, $4, NULL); }
	| INTERFACE STRING '{' url domains '}'	        { $$ = new_ast_iface($2, $5, $4); }
	;

domains : domain	
	| domains domain				{ $$ = $2; $$->next = $1; }
	;

domain	: DOMAIN STRING					{ $$ = new_ast_domain($2, NULL); }
	| DOMAIN STRING '{' url '}'		        { $$ = new_ast_domain($2, $4); }
	;

url     : HTTP_GET STRING				{ $$ = new_ast_url($2); }
	;
