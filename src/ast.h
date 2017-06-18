#pragma once

#include <stdbool.h>

typedef struct ast_domain {
    const char        *name;
    const char        *url;
    struct ast_domain *next;
} ast_domain_t;

typedef struct ast_iface {
    const char       *name;
    const char       *url;
    ast_domain_t     *domains;
    struct ast_iface *next;
} ast_iface_t;

typedef struct ast {
    const char  *url;
    ast_iface_t *interfaces;
} ast_t;

ast_t *
new_ast(ast_iface_t *i, const char *u);

ast_iface_t *
new_ast_iface(const char *name, ast_domain_t *d, const char *u);

ast_domain_t *
new_ast_domain(const char *name, const char *u);

bool
valid_ast(ast_t *ast);
