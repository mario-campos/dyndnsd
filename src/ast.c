#include <err.h>
#include <search.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include "ast.h"

#define HASH_TABLE_SIZE 10
#define BITS(x) (sizeof(x) * 8)

ast_t *
new_ast(ast_iface_t *i, ast_param_t *p) {
    ast_t *ast;
    if ((ast = calloc(sizeof(ast_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast->interfaces = i;
    ast->parameters = p;
    return ast;
}

ast_iface_t *
new_ast_iface(const char *name, ast_domain_t *d, ast_param_t *p) {
    ast_iface_t *ast_iface;
    if ((ast_iface = calloc(sizeof(ast_iface_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast_iface->name = name;
    ast_iface->domains = d;
    ast_iface->parameters = p;
    return ast_iface;
}

ast_domain_t *
new_ast_domain(const char *name, ast_param_t *p) {
    ast_domain_t *ast_domain;
    if ((ast_domain = calloc(sizeof(ast_domain_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast_domain->name = name;
    ast_domain->parameters = p;
    return ast_domain;
}

ast_param_t *
new_ast_param(int ptype, const char *value) {
    ast_param_t *ast_param;
    if ((ast_param = calloc(sizeof(ast_param_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast_param->ptype = ptype;
    ast_param->value = value;
    return ast_param;
}

bool
valid_ast(ast_t *ast) {
    hcreate(HASH_TABLE_SIZE);

    for (ast_iface_t *aif = ast->interfaces; aif; aif = aif->next) {
        // ensure interface exists
        if (!if_nametoindex(aif->name))
            return false;

        // ensure interface isn't repeated
        char *key = strdup(aif->name);
        if (hsearch((ENTRY){key, NULL}, FIND))
            return false;
        hsearch((ENTRY){key, NULL}, ENTER);

        for (ast_domain_t *ad = aif->domains; ad; ad = ad->next) {
            // ensure domain name isn't repeated
            key = strdup(ad->name);
            if (hsearch((ENTRY){key, NULL}, FIND))
                return false;
            hsearch((ENTRY){key, NULL}, ENTER);
        }
    }

    hdestroy();
    return true;
}
