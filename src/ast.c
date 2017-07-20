#include <sys/socket.h>

#include <net/if.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

extern FILE    *yyin;
extern int 	yyparse();

static bool isvalid(struct ast *);
static void free_domains(struct ast_domains *);

struct ast *
ast_new(struct ast_ifaces *ifaces, const char *url)
{
	struct ast *ast = malloc(sizeof(struct ast));
	if (NULL == ast)
		err(1, "malloc(3)");
	ast->interfaces = ifaces;
	ast->url = url;
	return ast;
}

struct ast_ifaces *
ast_new_iface(const char *name, struct ast_domains *domains, const char *url)
{
	struct ast_ifaces *list = malloc(sizeof(struct ast_ifaces));
	if (NULL == list)
		err(1, "malloc(3)");
	SLIST_INIT(list);

	struct ast_iface *iface = malloc(sizeof(struct ast_iface));
	if (NULL == iface)
		err(1, "malloc(3)");
	iface->name = name;
	iface->domains = domains;
	iface->url = url;

	SLIST_INSERT_HEAD(list, iface, next);

	return list;
}

struct ast_domains *
ast_new_domain(const char *name, const char *url)
{
	struct ast_domains *list = malloc(sizeof(struct ast_domains));
	if (NULL == list)
		err(1, "malloc(3)");
	SLIST_INIT(list);

	struct ast_domain *domain = malloc(sizeof(struct ast_domain));
	if (NULL == domain)
		err(1, "malloc(3)");
	domain->name = name;
	domain->url = url;

	SLIST_INSERT_HEAD(list, domain, next);

	return list;
}

struct ast_ifaces *
ast_merge_ifaces(struct ast_ifaces *lista, struct ast_ifaces *listb)
{
	while (!SLIST_EMPTY(listb)) {
		struct ast_iface *iface = SLIST_FIRST(listb);
		SLIST_REMOVE_HEAD(listb, next);
		SLIST_INSERT_HEAD(lista, iface, next);
	}
	free(listb);
	return lista;
}

struct ast_domains *
ast_merge_domains(struct ast_domains *lista, struct ast_domains *listb)
{
	while (!SLIST_EMPTY(listb)) {
		struct ast_domain *domain = SLIST_FIRST(listb);
		SLIST_REMOVE_HEAD(listb, next);
		SLIST_INSERT_HEAD(lista, domain, next);
	}
	free(listb);
	return lista;
}

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
