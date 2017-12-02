#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"
#include "die.h"

extern FILE *yyin;
extern int   yyparse();

static void
ast_free(struct ast_root *ast)
{
	assert(ast);

	for(size_t i = 0; i < ast->iface_len; i++) {
		struct ast_iface *aif = ast->iface[i];

		for(size_t j = 0; j < aif->domain_len; j++) {
			struct ast_domain *ad = aif->domain[j];
			free(ad->domain);
			free(ad->url);
			free(ad);
		}

		free(aif->if_name);
		free(aif);
	}

	free(ast->user);
	free(ast->group);
	free(ast);
}

bool
ast_load(struct ast_root **ast, FILE *file)
{
	assert(ast);
	assert(file);

	struct ast_root *tmp;

	yyin = file;
	if (yyparse(&tmp)) {
		if (*ast) ast_free(*ast);
		*ast = tmp;
		return true;
	}

	return false;
}

struct ast_domain *
ast_domain_new(char *domain, char *url)
{
	assert(domain);
	assert(url);

	struct ast_domain *ad = malloc(sizeof(*ad));
	if (NULL == ad) die(LOG_CRIT, AT("malloc(3)"));
	ad->domain = domain;
	ad->url = url;
	return ad;
}

struct ast_iface *
ast_iface_new(char *name, size_t ndomains)
{
	assert(name);
	assert(ndomains > 0);

	struct ast_iface *aif = calloc(sizeof(*aif) + ndomains * sizeof(aif), (size_t)1);
	if (NULL == aif) die(LOG_CRIT, AT("calloc(3)"));
	aif->if_name = name;
	return aif;
}
