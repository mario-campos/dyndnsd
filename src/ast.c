#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "ast.h"
#include "parser.h"

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

	yyin = file;
	return yyparse(ast);
}

bool
ast_reload(struct ast_root **ast, FILE *file)
{
	assert(ast);
	assert(file);

	struct ast_root *tmp;
	rewind(file);
	if (ast_load(&tmp, file)) {
		ast_free(*ast);
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
	if (!ad) {
		syslog(LOG_ERR, "malloc(3): %m");
		exit(1);
	}
	ad->domain = domain;
	ad->url = url;
	return ad;
}

struct ast_iface *
ast_iface_new(char *name, int len)
{
	assert(name);
	assert(len > 0);

	struct ast_iface *aif = calloc(sizeof(*aif) + len * sizeof(aif), 1);
	if (!aif) {
		syslog(LOG_ERR, "calloc(3): %m");
		exit(1);
	}
	aif->if_name = name;
	return aif;
}
