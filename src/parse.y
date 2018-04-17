%{
#include <sys/queue.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "die.h"

extern int yylex();
extern int yyerror();

SLIST_HEAD(cst_list, cst_node);

struct cst_node {
	int type;
	char *string;
	SLIST_ENTRY(cst_node) next;
	struct cst_list children;
};

static struct cst_node *cst_node(int, char *, struct cst_node *);
static void cst_free(struct cst_node *);
static struct ast_root *cst2ast(struct cst_node *);
static size_t sllist_counttype(struct cst_list *, int);
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

%%

static struct cst_node *
cst_node(int type, char *string, struct cst_node *children)
{
	struct cst_node *node;

	node = malloc(sizeof(*node));
	if (NULL == node)
		die(LOG_CRIT, AT("malloc(3): %m"));

	node->type = type;
	node->string = string;
	SLIST_NEXT(node, next) = NULL;
	SLIST_FIRST(&node->children) = children;
	return node;
}

static void
cst_free(struct cst_node *cst)
{
	struct cst_node *n;
	while (!SLIST_EMPTY(&cst->children)) {
		n = SLIST_FIRST(&cst->children);
		SLIST_REMOVE_HEAD(&cst->children, next);
		cst_free(n);
	}

	free(cst->string);
	free(cst);
}

static struct ast_root *
cst2ast(struct cst_node *cst)
{
	size_t n;
	struct ast_root *ast;
	struct ast_iface *aif;
	struct cst_node *a, *b;

	n = sllist_counttype(&cst->children, INTERFACE);
	ast = calloc(sizeof(*ast) + n * sizeof(struct ast_iface *), (size_t)1);

	SLIST_FOREACH(a, &cst->children, next)
		if (RUN == a->type) {
			ast->cmd = strdup(a->string);
			break;
		}

	SLIST_FOREACH(a, &cst->children, next) {
		if (INTERFACE == a->type) {
			n = sllist_counttype(&a->children, DOMAIN);
			aif = ast_iface_new(strdup(a->string), n);
			ast->iface[ast->iface_len++] = aif;

			SLIST_FOREACH(b, &a->children, next)
				if (DOMAIN == b->type)
					aif->domain[aif->domain_len++] = strdup(b->string);
		}
	}

	return ast;
}

static size_t
sllist_counttype(struct cst_list *head, int type)
{
	size_t count = 0;
	struct cst_node *a;
	SLIST_FOREACH(a, head, next)
		if (type == a->type) count++;
	return count;
}
