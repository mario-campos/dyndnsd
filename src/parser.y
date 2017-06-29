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
}

%token INTERFACE
%token DOMAIN
%token HTTP_GET
%token <str> STRING

%type <ast> configuration
%type <ast_iface> interfaces interface
%type <ast_domain> domains domain
%type <str> url 

%locations
%parse-param {struct ast **ast}

%%

configuration
	: interfaces					{ *ast = ast_new($1, NULL); }
	| url interfaces				{ *ast = ast_new($2, $1); }
	;

interfaces
	: interface
	| interfaces interface				{ $$ = $2; $$->next = $1; }
	;

interface
	: INTERFACE STRING '{' domains '}'		{ $$ = ast_new_iface($2, $4, NULL); }
	| INTERFACE STRING '{' url domains '}'	        { $$ = ast_new_iface($2, $5, $4); }
	;

domains : domain	
	| domains domain				{ $$ = $2; $$->next = $1; }
	;

domain	: DOMAIN STRING					{ $$ = ast_new_domain($2, NULL); }
	| DOMAIN STRING '{' url '}'		        { $$ = ast_new_domain($2, $4); }
	;

url     : HTTP_GET STRING				{ $$ = $2; }
	;
