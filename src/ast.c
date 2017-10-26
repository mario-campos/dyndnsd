#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"

extern FILE *yyin;
extern int   yyparse();
	
void
ast_free(struct ast_root *ast)
{
	struct ast_iface *aif;
	struct ast_domain *ad;

	free((char *)ast->user);
	free((char *)ast->group);

	while (!SLIST_EMPTY(&ast->interfaces)) {
		aif = SLIST_FIRST(&ast->interfaces);	
		SLIST_REMOVE_HEAD(&ast->interfaces, next);
		while (!SLIST_EMPTY(&aif->domains)) {
			ad = SLIST_FIRST(&aif->domains);
			SLIST_REMOVE_HEAD(&aif->domains, next);
			free((char *)ad->domain);
			free((char *)ad->url);
			free(ad);
		}
		free(aif);
	}

	free(ast);
}

bool
ast_load(struct ast_root **ast, FILE *file)
{
	yyin = file;
	if (yyparse(ast))
		return false;
	return true;
}

bool
ast_reload(struct ast_root **ast, FILE *file)
{
	struct ast_root *tmp;
	rewind(file);
	if (ast_load(&tmp, file)) {
		ast_free(*ast);
		*ast = tmp;
		return true;
	}
	return false;
}
