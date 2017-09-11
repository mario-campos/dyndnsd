%{
#include <err.h>
#include "ast.h"

extern int yylex();
extern int yyerror();

static struct ast *ast_new(struct ast_ifaces *, const char *);
static struct ast_ifaces *ast_new_iface(const char *, struct ast_domains *, const char *);
static struct ast_domains *ast_new_domain(const char *, const char *);
static struct ast_ifaces *ast_merge_ifaces(struct ast_ifaces *, struct ast_ifaces *);
static struct ast_domains *ast_merge_domains(struct ast_domains *, struct ast_domains *);
%}

%union {
	char               *string;
	struct ast         *ast;
	struct ast_ifaces  *ast_ifaces;
	struct ast_domains *ast_domains;
}

%token INTERFACE
%token DOMAIN
%token UPDATE
%token <string> STRING

%type <ast> configuration
%type <ast_ifaces> interfaces interface
%type <ast_domains> domains domain
%type <string> update

%locations
%parse-param {struct ast **ast}

%%

configuration
	: interfaces					{ *ast = ast_new($1, NULL); }
	| update interfaces				{ *ast = ast_new($2, $1); }
	;

interfaces
	: interface
	| interfaces interface				{ $$ = ast_merge_ifaces($2, $1); }
	;

interface
	: INTERFACE STRING '{' domains '}'		{ $$ = ast_new_iface($2, $4, NULL); }
	| INTERFACE STRING '{' update domains '}'	{ $$ = ast_new_iface($2, $5, $4); }
	;

domains : domain	
	| domains domain				{ $$ = ast_merge_domains($2, $1); }
	;

domain	: DOMAIN STRING					{ $$ = ast_new_domain($2, NULL); }
	| DOMAIN STRING '{' update '}'		        { $$ = ast_new_domain($2, $4); }
	;

update	: UPDATE STRING					{ $$ = $2; }
	;

%%

static struct ast *
ast_new(struct ast_ifaces *ifaces, const char *url)
{
	struct ast *ast = malloc(sizeof(struct ast));
	if (NULL == ast)
		err(1, "malloc(3)");
	ast->interfaces = ifaces;
	ast->url = url;
	return ast;
}

static struct ast_ifaces *
ast_new_iface(const char *name, struct ast_domains *domains, const char *url)
{
	struct ast_ifaces *list = malloc(sizeof(struct ast_ifaces));
	if (NULL == list)
		err(1, "malloc(3)");
	SLIST_INIT(list);

	struct ast_iface *iface = malloc(sizeof(struct ast_iface));
	if (NULL == iface)
		err(1, "malloc(3)");
	iface->name = name;
	iface->domains = domains;
	iface->url = url;

	SLIST_INSERT_HEAD(list, iface, next);

	return list;
}

static struct ast_domains *
ast_new_domain(const char *name, const char *url)
{
	struct ast_domains *list = malloc(sizeof(struct ast_domains));
	if (NULL == list)
		err(1, "malloc(3)");
	SLIST_INIT(list);

	struct ast_domain *domain = malloc(sizeof(struct ast_domain));
	if (NULL == domain)
		err(1, "malloc(3)");
	domain->name = name;
	domain->url = url;

	SLIST_INSERT_HEAD(list, domain, next);

	return list;
}

static struct ast_ifaces *
ast_merge_ifaces(struct ast_ifaces *lista, struct ast_ifaces *listb)
{
	while (!SLIST_EMPTY(listb)) {
		struct ast_iface *iface = SLIST_FIRST(listb);
		SLIST_REMOVE_HEAD(listb, next);
		SLIST_INSERT_HEAD(lista, iface, next);
	}
	free(listb);
	return lista;
}

static struct ast_domains *
ast_merge_domains(struct ast_domains *lista, struct ast_domains *listb)
{
	while (!SLIST_EMPTY(listb)) {
		struct ast_domain *domain = SLIST_FIRST(listb);
		SLIST_REMOVE_HEAD(listb, next);
		SLIST_INSERT_HEAD(lista, domain, next);
	}
	free(listb);
	return lista;
}
