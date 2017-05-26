%{
#include <err.h>
#include "dyndnsd.h"

extern int yylex();
extern int yyerror();

static ast_t *
new_ast(ast_iface_t *i, ast_param_t *p);

static ast_iface_t *
new_interface(const char *name, ast_domain_t *d, ast_param_t *p);

static ast_domain_t *
new_domain(const char *name, ast_param_t *p);

static ast_param_t *
new_parameter(int parameter_type, const char *value);
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

%%

static ast_t *
new_ast(ast_iface_t *i, ast_param_t *p) {
    ast_t *ast;
    if ((ast = calloc(sizeof(ast_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast->interfaces = i;
    ast->parameters = p;
    return ast;
}

static ast_iface_t *
new_interface(const char *name, ast_domain_t *d, ast_param_t *p) {
    ast_iface_t *ast_iface;
    if ((ast_iface = calloc(sizeof(ast_iface_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast_iface->name = name;
    ast_iface->domains = d;
    ast_iface->parameters = p;
    return ast_iface;
}

static ast_domain_t *
new_domain(const char *name, ast_param_t *p) {
    ast_domain_t *ast_domain;
    if ((ast_domain = calloc(sizeof(ast_domain_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast_domain->name = name;
    ast_domain->parameters = p;
    return ast_domain;
}

static ast_param_t *
new_parameter(int ptype, const char *value) {
    ast_param_t *ast_param;
    if ((ast_param = calloc(sizeof(ast_param_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast_param->ptype = ptype;
    ast_param->value = value;
    return ast_param;
}
