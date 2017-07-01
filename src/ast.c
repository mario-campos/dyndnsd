#include <sys/socket.h>

#include <net/if.h>

#include <err.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

#define HASH_TABLE_SIZE 10

struct ast *
ast_new(const struct ast_ifaces *ifaces, const char *url)
{
	struct ast *ast = malloc(sizeof(struct ast));
	if (NULL == ast)
		err(1, "malloc(3)");
	ast->interfaces = ifaces;
	ast->url = url;
	return ast;
}

struct ast_ifaces *
ast_new_iface(const char *name, const struct ast_domains *domains, const char *url)
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

bool
ast_is_valid(struct ast *ast)
{
	struct ast_iface *aif;
	struct ast_domain *ad;

	bool valid = true;
	bool has_local0_url = ast->url != NULL;

	hcreate(HASH_TABLE_SIZE);

	SLIST_FOREACH(aif, ast->interfaces, next) {
		bool has_local1_url = aif->url != NULL;

		/* check that the specified interface exists */
		if (0 == if_nametoindex(aif->name)) {
			valid = false;
			fprintf(stderr, "error: interface \"%s\" not found\n", aif->name);
		}

		/* check for duplicate interfaces */
		char *key = strdup(aif->name);
		if (hsearch((ENTRY) {key, NULL}, FIND)) {
			valid = false;
			fprintf(stderr, "error: duplicate interface \"%s\" detected\n", aif->name);
		}
		hsearch((ENTRY) {key, NULL}, ENTER);

		SLIST_FOREACH(ad, aif->domains, next) {
			bool has_local2_url = ad->url != NULL;

			/* check fo rmissing required URL statement */
			if (!has_local0_url &&
			    !has_local1_url &&
			    !has_local2_url) {
				valid = false;
				fprintf(stderr, "error: no `http-get` statement in scope of domain \"%s\"\n", ad->name);
			}

			/* check for duplidate domain names */
			key = strdup(ad->name);
			if (hsearch((ENTRY) {key, NULL}, FIND)) {
				valid = false;
				fprintf(stderr, "error: duplicate domain \"%s\" detected\n", ad->name);
			}
			hsearch((ENTRY) {key, NULL}, ENTER);
		}
	}

	hdestroy();
	return valid;
}
