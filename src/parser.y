%{
#include <stddef.h>
#include "ast.h"
extern int yylex();
extern int yyerror();
%}

%union {
	char               *str;
	struct ast         *ast;
	struct ast_ifaces  *ast_ifaces;
	struct ast_domains *ast_domains;
}

%token INTERFACE
%token DOMAIN
%token HTTP_GET
%token <str> STRING

%type <ast> configuration
%type <ast_ifaces> interfaces interface
%type <ast_domains> domains domain
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
	| interfaces interface				{ $$ = ast_merge_ifaces($2, $1); }
	;

interface
	: INTERFACE STRING '{' domains '}'		{ $$ = ast_new_iface($2, $4, NULL); }
	| INTERFACE STRING '{' url domains '}'	        { $$ = ast_new_iface($2, $5, $4); }
	;

domains : domain	
	| domains domain				{ $$ = ast_merge_domains($2, $1); }
	;

domain	: DOMAIN STRING					{ $$ = ast_new_domain($2, NULL); }
	| DOMAIN STRING '{' url '}'		        { $$ = ast_new_domain($2, $4); }
	;

url     : HTTP_GET STRING				{ $$ = $2; }
	;
