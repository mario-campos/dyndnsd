#include <sys/socket.h>
#include <net/if.h>
#include <stdlib.h>

#include "ast.h"

extern FILE *yyin;
extern int   yyparse();

static bool isvalid(struct ast *);
static void free_domains(struct ast_domains *);

void
ast_free(struct ast *ast)
{
	while (!SLIST_EMPTY(ast->interfaces)) {
		struct ast_iface *aif = SLIST_FIRST(ast->interfaces);
		SLIST_REMOVE_HEAD(ast->interfaces, next);
		free_domains(aif->domains);
		free(aif->domains);
		free((char *)aif->name);
		free((char *)aif->url);
		free(aif);
	}
	free((char *)ast->url);
	free(ast);
}

bool
ast_load(struct ast **ast, FILE *file)
{
	yyin = file;
	if (yyparse(ast))
		return false;
	if (!isvalid(*ast))
		return false;
	return true;
}

static bool
isvalid(struct ast *ast)
{
	struct ast_iface *aif;
	struct ast_domain *ad;

	bool valid = true;
	bool has_local0_url = ast->url != NULL;

	SLIST_FOREACH(aif, ast->interfaces, next) {
		bool has_local1_url = aif->url != NULL;

		/* check that the specified interface exists */
		if (0 == if_nametoindex(aif->name)) {
			valid = false;
			fprintf(stderr, "error: interface \"%s\" not found\n", aif->name);
		}

		SLIST_FOREACH(ad, aif->domains, next) {
			bool has_local2_url = ad->url != NULL;

			/* check fo rmissing required URL statement */
			if (!has_local0_url &&
			    !has_local1_url &&
			    !has_local2_url) {
				valid = false;
				fprintf(stderr, "error: no `update` statement in scope of domain \"%s\"\n", ad->name);
			}
		}
	}

	return valid;
}

static void
free_domains(struct ast_domains *list)
{
	while (!SLIST_EMPTY(list)) {
		struct ast_domain *ad = SLIST_FIRST(list);
		SLIST_REMOVE_HEAD(list, next);
		free((char *)ad->name);
		free((char *)ad->url);
		free(ad);
	}
}
