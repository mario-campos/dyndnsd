#pragma once

#include <stdbool.h>

typedef struct ast_url {
    const char       *value;
} ast_url_t;

typedef struct ast_domain {
    const char        *name;
    ast_url_t         *url;
    struct ast_domain *next;
} ast_domain_t;

typedef struct ast_iface {
    const char       *name;
    ast_domain_t     *domains;
    ast_url_t        *url;
    struct ast_iface *next;
} ast_iface_t;

typedef struct ast {
    ast_iface_t *interfaces;
    ast_url_t   *url;
} ast_t;

ast_t *
new_ast(ast_iface_t *i, ast_url_t *u);

ast_iface_t *
new_ast_iface(const char *name, ast_domain_t *d, ast_url_t *u);

ast_domain_t *
new_ast_domain(const char *name, ast_url_t *u);

ast_url_t *
new_ast_url(const char *value);

bool
valid_ast(ast_t *ast);
