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
ast_new(struct ast_iface *i, const char *u)
{
	struct ast *ast = calloc(1, sizeof(struct ast));
	if (NULL == ast)
		err(1, "calloc(3)");
	ast->interfaces = i;
	ast->url = u;
	return ast;
}

struct ast_iface *
ast_new_iface(const char *name, struct ast_domain *d, const char *u)
{
	struct ast_iface *ast_iface = calloc(1, sizeof(struct ast_iface));
	if (NULL == ast_iface)
		err(1, "calloc(3)");
	ast_iface->name = name;
	ast_iface->domains = d;
	ast_iface->url = u;
	return ast_iface;
}

struct ast_domain *
ast_new_domain(const char *name, const char *u)
{
	struct ast_domain *ast_domain = calloc(1, sizeof(struct ast_domain));
	if (NULL == ast_domain)
		err(1, "calloc(3)");
	ast_domain->name = name;
	ast_domain->url = u;
	return ast_domain;
}

bool
ast_is_valid(struct ast *ast)
{
	bool valid = true;
	bool has_local0_url = ast->url != NULL;

	hcreate(HASH_TABLE_SIZE);

	for (struct ast_iface *aif = ast->interfaces; aif; aif = aif->next) {
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

		for (struct ast_domain * ad = aif->domains; ad; ad = ad->next) {
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
