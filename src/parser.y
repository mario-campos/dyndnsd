%{
#include <sys/queue.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "die.h"

extern int yylex();
extern int yyerror();
extern bool ast_error;

SLIST_HEAD(cst_list, cst_node);

struct cst_node {
	int type;
	size_t len;
	SLIST_ENTRY(cst_node) next;
	char *string;
	struct cst_node *child[];
};

static int count(struct cst_list *);
static bool cst_valid(struct cst_node *);
static struct cst_node *cst_node_new(int, char *, int);
static struct cst_node *prepend(struct cst_node *, struct cst_node *);
static void cst_free(struct cst_node *);
static struct ast_root *cst2ast(struct cst_node *);
%}

%union {
	char            *string;
	struct cst_node *cst_node;
}

%token USER GROUP INTERFACE DOMAIN RUN
%token <string> STRING
%type <cst_node> config config_statements config_statement
%type <cst_node> interface interface_statements interface_statement
%type <cst_node> domain run user group

%locations
%parse-param {struct ast_root **ast}

%%

config : config_statements				{
								size_t i;
								struct cst_list list;
								struct cst_node *cst, *x;

								i = 0;
								SLIST_FIRST(&list) = $1;
								cst = cst_node_new(0, NULL, count(&list));
								SLIST_FOREACH(x, &list, next)
									cst->child[i++] = x;

								if (!cst_valid(cst))
									return false;

								*ast = cst2ast(cst);
								cst_free(cst);
								return true;
							}
	;

config_statements
	: config_statement
	| config_statements config_statement		{ $$ = prepend($2, $1); }
	;

config_statement
	: interface
	| run
	| user
	| group
	;

user	: USER STRING					{ $$ = cst_node_new(USER, $2, 0); }
	;

group	: GROUP STRING					{ $$ = cst_node_new(GROUP, $2, 0); }
	;

interface
	: INTERFACE STRING '{' interface_statements '}'	{
								size_t i;
								struct cst_list list;
								struct cst_node *iface, *x;

								i = 0;
								SLIST_FIRST(&list) = $4;
								iface = cst_node_new(INTERFACE, $2, count(&list));
								SLIST_FOREACH(x, &list, next)
									iface->child[i++] = x;

								$$ = iface;
							}
	;

interface_statements
	: interface_statement
	| interface_statements interface_statement	{ $$ = prepend($2, $1); }
	;

interface_statement
	: domain
	| run
	;

domain	: DOMAIN STRING					{ $$ = cst_node_new(DOMAIN, $2, 0); }
	| DOMAIN STRING '{' run '}'		        {
								$$ = cst_node_new(DOMAIN, $2, 1); 
								$$->child[0] = $4;
							}
	;

run	: RUN STRING					{ $$ = cst_node_new(RUN, $2, 0); }
	;

%%

static int
count(struct cst_list *list)
{
	size_t count = 0;
	struct cst_node *node;
	SLIST_FOREACH(node, list, next)
		count++;
	return count;
}

static bool
cst_valid(struct cst_node *cst)
{
	bool cst_valid = true;
	uint8_t topcount[300] = { 0 };

	/* check for extraneous top-scope nodes */
	for (size_t i = 0; i < cst->len; i++) {
		switch (cst->child[i]->type) {
		case RUN:
			topcount[RUN]++; break;
		case USER:
			topcount[USER]++; break;
		case GROUP:
			topcount[GROUP]++; break;
		default: continue; 
		}
	}

	if (topcount[RUN] > 1) {
		cst_valid = false;
		fputs("error: too many 'run' statements in top scope; limit one.", stderr);
	}
	if (topcount[USER] > 1) {
		cst_valid = false;
		fputs("error: too many 'user' statements; limit one.", stderr);
	}
	if (topcount[GROUP] > 1) {
		cst_valid = false;
		fputs("error: too many 'group' statements; limit one.", stderr);
	}

	for (size_t i = 0, n = 0; i < cst->len; i++, n = 0) {
		if (INTERFACE != cst->child[i]->type)
			continue;

		const struct cst_node *node = cst->child[i];
		for (size_t j = 0; j < node->len; j++)
			if (RUN == node->child[j]->type) n++;

		if (n > 1) {
			cst_valid = false;
			fprintf(stderr, "error: too many 'run' statements in interface (%s) scope; limit one.", node->string);
		}
	}

	return cst_valid;
}

static struct cst_node *
cst_node_new(int type, char *string, int len)
{
	struct cst_node *node = calloc(sizeof(*node) + len * sizeof(node), (size_t)1);
	if (NULL == node) die(LOG_CRIT, AT("calloc(3): %m"));
	node->type = type;
	node->string = string;
	node->len = len;
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
	for (size_t i = 0; i < cst->len; i++)
		cst_free(cst->child[i]);
	free(cst->string);
	free(cst);
}

static struct ast_root *
cst2ast(struct cst_node *cst)
{
	struct ast_root *ast;
	char *run1, *run2, *run3;

	run1 = run2 = run3 = NULL;

	ast = calloc(sizeof(*ast) + cst->len * sizeof(struct ast_iface *), (size_t)1);
	if (NULL == ast) die(LOG_CRIT, AT("calloc(3): %m"));

	for (size_t i = 0; i < cst->len; i++) {
		if (USER == cst->child[i]->type)
			ast->user = strdup(cst->child[i]->string);
		else if (GROUP == cst->child[i]->type)
			ast->group = strdup(cst->child[i]->string);
		else if (RUN == cst->child[i]->type)
			run1 = strdup(cst->child[i]->string);
	}

	for (size_t i = 0; i < cst->len; i++) {
		if (INTERFACE != cst->child[i]->type)
			continue;

		struct ast_iface *aif;
		struct cst_node *inode;

		inode = cst->child[i];
		aif = ast_iface_new(strdup(inode->string), inode->len);
		ast->iface[ast->iface_len++] = aif;

		for (size_t j = 0; j < inode->len; j++) {
			if (RUN == inode->child[j]->type)
				run2 = inode->child[j]->string;
			else {
				struct ast_domain *ad;
				struct cst_node *dnode;

				dnode = inode->child[j];
				if (1 == dnode->len)
					run3 = dnode->child[0]->string; 
				ad = ast_domain_new(strdup(dnode->string), strdup(run3 ?: run2 ?: run1));
				aif->domain[aif->domain_len++] = ad;
				run3 = NULL;
			}
		}

		run2 = NULL;
	}

	return ast;
}
