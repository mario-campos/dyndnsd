#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "cst.h"
#include "y.tab.h"

#define RUNBUFSIZ 1024

static char *cst_convert_run(struct cst_node *);
static char *cst_convert_domain(struct cst_node *);
static struct ast_iface *cst_convert_iface(struct cst_node *);

char *
dequote(char *quoted)
{
	assert(quoted);

	char *dequoted;
       
	dequoted = strndup(quoted+1, strlen(quoted)-2);
	if (NULL == dequoted)
		err(1, NULL);

	return dequoted;
}

static char *
cst_convert_run(struct cst_node *node)
{
	assert(node);

	char *buf;
	size_t len;
	struct cst_node *i;

	len = 0;
	SLIST_FOREACH(i, &node->label, next)
		len += strlen(i->string) + 1;

	buf = calloc(len, (size_t)1);
	if (NULL == buf)
		err(1, NULL);

	SLIST_FOREACH(i, &node->label, next) {
		(void)strlcat(buf, i->string, len);
		(void)strlcat(buf, " ", len);
	}

	return buf;
}

static char *
cst_convert_domain(struct cst_node *node)
{
	assert(node);

	char *domain;
	struct cst_node *label;

	label = SLIST_FIRST(&node->label);

	switch (label->type) {
	case QUOTE:
		domain = dequote(label->string);
		free(label->string);
		label->string = NULL;
		break;
	case STRING:
		domain = label->string;
		label->string = NULL;
		break;
	}

	return domain;
}

static struct ast_iface *
cst_convert_iface(struct cst_node *node)
{
	assert(node);

	size_t n;
	struct cst_node *i, *label;
	struct ast_iface *aif;

	n = 0;
	SLIST_FOREACH(i, &node->children, next)
		if (DOMAIN == i->type) n++;

	aif = calloc(sizeof(*aif) + n * sizeof(aif), (size_t)1);
	if (NULL == aif)
		err(1, NULL);

	label = SLIST_FIRST(&node->label);
	switch (label->type) {
	case QUOTE:
		aif->if_name = dequote(label->string);
		free(label->string);
		label->string = NULL;
		break;
	case STRING:
		aif->if_name = label->string;
		label->string = NULL;
		break;
	}

	SLIST_FOREACH(i, &node->children, next)
		if (DOMAIN == i->type)
			aif->domain[aif->domain_len++] = cst_convert_domain(i);

	return aif;
}

struct cst_node *
cst_node(int type, char *string, struct cst_node *label, struct cst_node *children)
{
	assert(string);
	assert(label);
	assert(children);

	struct cst_node *node;

	node = malloc(sizeof(*node));
	if (NULL == node)
		err(1, NULL);

	node->type = type;
	node->string = string;
	SLIST_NEXT(node, next) = NULL;
	SLIST_FIRST(&node->label) = label;
	SLIST_FIRST(&node->children) = children;
	return node;
}

void
cst_free(struct cst_node *cst)
{
	assert(cst);

	struct cst_node *n;

	while (!SLIST_EMPTY(&cst->children)) {
		n = SLIST_FIRST(&cst->children);
		SLIST_REMOVE_HEAD(&cst->children, next);
		cst_free(n);
	}

	while (!SLIST_EMPTY(&cst->label)) {
		n = SLIST_FIRST(&cst->label);
		SLIST_REMOVE_HEAD(&cst->label, next);
		cst_free(n);
	}

	free(cst->string);
	free(cst);
}

struct ast_root *
cst_convert(struct cst_node *cst)
{
	assert(cst);

	size_t n;
	struct cst_node *i;
	struct ast_root *ast;

	n = 0;
	SLIST_FOREACH(i, &cst->children, next)
		if (INTERFACE == i->type) n++;

	ast = calloc(sizeof(*ast) + n*sizeof(ast), (size_t)1);
	if (NULL == ast)
		err(1, NULL);

	SLIST_FOREACH(i, &cst->children, next) {
		if (RUN == i->type)
			ast->cmd = cst_convert_run(i);
		if (INTERFACE == i->type)
			ast->iface[ast->iface_len++] = cst_convert_iface(i);
	}

	return ast;
}
