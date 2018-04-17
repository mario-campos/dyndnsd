#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "cst.h"
#include "die.h"
#include "y.tab.h"

struct cst_node *
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

void
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

struct ast_root *
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

size_t
sllist_counttype(struct cst_list *head, int type)
{
	size_t count = 0;
	struct cst_node *a;
	SLIST_FOREACH(a, head, next)
		if (type == a->type) count++;
	return count;
}
