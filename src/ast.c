#include <err.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ast.h"

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
new_interface(const char *name, ast_domain_t *d, ast_param_t *p) {
    ast_iface_t *ast_iface;
    if ((ast_iface = calloc(sizeof(ast_iface_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast_iface->name = name;
    ast_iface->domains = d;
    ast_iface->parameters = p;
    return ast_iface;
}

ast_domain_t *
new_domain(const char *name, ast_param_t *p) {
    ast_domain_t *ast_domain;
    if ((ast_domain = calloc(sizeof(ast_domain_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast_domain->name = name;
    ast_domain->parameters = p;
    return ast_domain;
}

ast_param_t *
new_parameter(int ptype, const char *value) {
    ast_param_t *ast_param;
    if ((ast_param = calloc(sizeof(ast_param_t), 1)) == NULL)
        err(EXIT_FAILURE, "calloc(3)");
    ast_param->ptype = ptype;
    ast_param->value = value;
    return ast_param;
}

bool
valid_ast(ast_t *ast) {
    // validate top-level parameters

    // validate interface

    // --> validate interface parameters

    // --> validate interface domain

    // -----> validate domain parameters

    // -----> loop domain parameters

    // --> loop interface domains

    // validate interfaces
    return true;
}
