%{
#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <syslog.h>

#include "ast.h"

extern int yylex();
extern int yyerror();
extern bool ast_error;

SLIST_HEAD(cst_root, cst_node);

struct cst_node {
	int nodetype;
	const char *strval;
	SLIST_HEAD(, cst_node) children;
	SLIST_ENTRY(cst_node)  next;
};

static struct cst_node *cst_node_new(int, const char *, struct cst_node *);
static struct cst_node *prepend(struct cst_node *, struct cst_node *);
static void cst_free(struct cst_node *);
static struct ast_root *ast_new(struct cst_root *);
static struct ast_domain *ast_domain_new(const char *, const char *);
static struct ast_iface *ast_iface_new(const char *);
%}

%union {
	char            *string;
	struct cst_node *cst_node;
}

%token USER
%token GROUP
%token INTERFACE
%token DOMAIN
%token UPDATE
%token <string> STRING

%type <cst_node> config config_statements config_statement
%type <cst_node> interface interface_statements interface_statement
%type <cst_node> domain update user group

%locations
%parse-param {struct ast_root **ast}

%%

config : config_statements				{
								if (ast_error)
									exit(1);
								struct cst_root cst = SLIST_HEAD_INITIALIZER(cst);
								SLIST_INSERT_HEAD(&cst, $1, next);
								*ast = ast_new(&cst);
							}
	;

config_statements
	: config_statement
	| config_statements config_statement		{ $$ = prepend($2, $1); }
	;

config_statement
	: interface
	| update
	| user
	| group
	;

user	: USER STRING					{ $$ = cst_node_new(USER, $2, NULL); }
	;

group	: GROUP STRING					{ $$ = cst_node_new(GROUP, $2, NULL); }
	;

interface
	: INTERFACE STRING '{' interface_statements '}'	{
								if (!if_nametoindex($2)) {
									ast_error = true;
									syslog(LOG_ERR, "error: this interface does not exist (%s)", $2);
								}
								$$ = cst_node_new(INTERFACE, $2, $4);
							}
	;

interface_statements
	: interface_statement
	| interface_statements interface_statement	{ $$ = prepend($2, $1); }
	;

interface_statement
	: domain
	| update
	;

domain	: DOMAIN STRING					{ $$ = cst_node_new(DOMAIN, $2, NULL); }
	| DOMAIN STRING '{' update '}'		        { $$ = cst_node_new(DOMAIN, $2,   $4); }
	;

update	: UPDATE STRING					{ $$ = cst_node_new(UPDATE, $2, NULL); }
	;

%%

static struct cst_node *
cst_node_new(int nodetype, const char *strval, struct cst_node *children)
{
	struct cst_node *node = malloc(sizeof(*node));
	if (NULL == node) {
		syslog(LOG_ERR, "malloc(3): %m");
		exit(1);
	}
	node->nodetype = nodetype;
	node->strval = strval;
	node->children.slh_first = children;
	SLIST_NEXT(node, next) = NULL;
	return node;
}

static struct cst_node *
prepend(struct cst_node *head, struct cst_node *tail)
{
	SLIST_NEXT(head, next) = tail;
	return head;
}

static void
cst_free(struct cst_node *cst)
{
	struct cst_node *child;

	while (!SLIST_EMPTY(&cst->children)) {
		child = SLIST_FIRST(&cst->children);
		SLIST_REMOVE_HEAD(&cst->children, next);
		cst_free(child);
	}

	free((char *)cst->strval);
	free(cst);
}

static struct ast_root *
ast_new(struct cst_root *cst)
{
	const char *url1, *url2, *url3;
	struct cst_node *child1, *child2;
	struct ast_domain *ad;
	struct ast_iface *aif;
	struct ast_root *ast;

	url1 = url2 = url3 = NULL;

	ast = malloc(sizeof(*ast));
	if (NULL == ast) {
		syslog(LOG_ERR, "malloc(3): %m");
		exit(1);
	}

	SLIST_FOREACH(child1, cst, next)
		if (UPDATE == child1->nodetype)
			url1 = child1->strval;

	SLIST_FOREACH(child1, cst, next) {
		if (INTERFACE == child1->nodetype) {
			aif = ast_iface_new(child1->strval);

			SLIST_FOREACH(child2, &child1->children, next)
				if (UPDATE == child2->nodetype)
					url2 = child2->strval;

			SLIST_FOREACH(child2, &child1->children, next) {
				if (DOMAIN == child2->nodetype) {
					if (!SLIST_EMPTY(&child2->children))
						url3 = SLIST_FIRST(&child2->children)->strval;
					ad = ast_domain_new(child2->strval, url3 ?: url2 ?: url1);
					url3 = NULL;
					SLIST_INSERT_HEAD(&aif->domains, ad, next);
				}
			}

			url2 = NULL;
			SLIST_INSERT_HEAD(&ast->interfaces, aif, next);
		}
		else if (USER == child1->nodetype) {
			ast->user = child1->strval;
		}
		else if (GROUP == child1->nodetype) {
			ast->group = child1->strval;
		}
	}

	return ast;
}

static struct ast_domain *
ast_domain_new(const char *domain, const char *url)
{
	struct ast_domain *ad = malloc(sizeof(*ad));
	if (NULL == ad) {
		syslog(LOG_ERR, "malloc(3): %m");
		exit(1);
	}
	ad->domain = domain;
	ad->url = url;
	SLIST_NEXT(ad, next) = NULL;
	return ad;
}

static struct ast_iface *
ast_iface_new(const char *name)
{
	struct ast_iface *aif = malloc(sizeof(*aif));
	if (NULL == aif) {
		syslog(LOG_ERR, "malloc(3): %m");
		exit(1);
	}
	aif->if_name = name;
	SLIST_NEXT(aif, next) = NULL;
	return aif;
}
