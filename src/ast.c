#include <sys/socket.h>
#include <net/if.h>

#include <err.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

#define HASH_TABLE_SIZE 10
#define BITS(x) (sizeof(x) * 8)

struct ast *
new_ast(struct ast_iface *i, const char *u) {
    struct ast *ast;
    if ((ast = calloc(sizeof(struct ast), 1)) == NULL)
        err(1, "calloc(3)");
    ast->interfaces = i;
    ast->url = u;
    return ast;
}

struct ast_iface *
new_ast_iface(const char *name, struct ast_domain *d, const char *u) {
    struct ast_iface *ast_iface;
    if ((ast_iface = calloc(sizeof(struct ast_iface), 1)) == NULL)
        err(1, "calloc(3)");
    ast_iface->name = name;
    ast_iface->domains = d;
    ast_iface->url = u;
    return ast_iface;
}

struct ast_domain *
new_ast_domain(const char *name, const char *u) {
    struct ast_domain *ast_domain;
    if ((ast_domain = calloc(sizeof(struct ast_domain), 1)) == NULL)
        err(1, "calloc(3)");
    ast_domain->name = name;
    ast_domain->url = u;
    return ast_domain;
}

bool
valid_ast(struct ast *ast) {
    bool valid = true;
    const bool has_local0_url = ast->url != NULL;

    if(hcreate(HASH_TABLE_SIZE) == 0)
        err(1, "hcreate(3)");

    for (struct ast_iface *aif = ast->interfaces; aif; aif = aif->next) {
        const bool has_local1_url = aif->url != NULL;

        // check that the specified interface exists
        if (if_nametoindex(aif->name) == 0) {
            valid = false;
            fprintf(stderr, "error: interface \"%s\" not found\n", aif->name);
        }

        // check for duplicate interface declarations
        char *key = strdup(aif->name);
        if (hsearch((ENTRY){key, NULL}, FIND)) {
            valid = false;
            fprintf(stderr, "error: duplicate interface \"%s\" detected\n", aif->name);
        }
        if(hsearch((ENTRY){key, NULL}, ENTER) == NULL)
            err(1, "hsearch(3)");

        for (struct ast_domain *ad = aif->domains; ad; ad = ad->next) {
            const bool has_local2_url = ad->url != NULL;

            // check for missing required URL statement
            if (!has_local0_url &&
                !has_local1_url &&
                !has_local2_url) {
                valid = false;
                fprintf(stderr, "error: no `http-get` statement in scope of domain \"%s\"\n", ad->name);
            }

            // check for duplicate domain names
            key = strdup(ad->name);
            if (hsearch((ENTRY){key, NULL}, FIND)) {
                valid = false;
                fprintf(stderr, "error: duplicate domain \"%s\" detected\n", ad->name);
            }
            if(hsearch((ENTRY){key, NULL}, ENTER) == NULL)
                err(1, "hsearch(3)");
        }
    }

    hdestroy();
    return valid;
}
