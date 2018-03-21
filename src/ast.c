#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "y.tab.h"
#include "die.h"

extern FILE *yyin;
extern int   yyparse();

struct ast_root **ast;

static void
ast_free(struct ast_root *ast)
{
	assert(ast);

	for(size_t i = 0; i < ast->iface_len; i++) {
		struct ast_iface *aif = ast->iface[i];

		for(size_t j = 0; j < aif->domain_len; j++)
			free(aif->domain[j]);
		free(aif->if_name);
		free(aif);
	}

	free(ast);
}

bool
ast_load(struct ast_root **ast, FILE *file)
{
	assert(ast);
	assert(file);

	struct ast_root *tmp;

	rewind(file);
	yyin = file;
	if (0 == yyparse(&tmp)) { // successful
		if (*ast) ast_free(*ast);
		*ast = tmp;
		return true;
	}

	return false;
}

struct ast_iface *
ast_iface_new(char *name, size_t ndomains)
{
	assert(name);
	assert(ndomains > 0);

	struct ast_iface *aif;

	aif = calloc(sizeof(*aif) + ndomains * sizeof(aif), (size_t)1);
	if (NULL == aif)
		die(LOG_CRIT, AT("calloc(3): %m"));

	aif->if_name = name;
	return aif;
}
