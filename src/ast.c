#include <stdbool.h>
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

	free(ast->user);
	free(ast->group);

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
