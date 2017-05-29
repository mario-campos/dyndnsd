#pragma once

#include <stdbool.h>

typedef struct ast_parameter {
    int ptype;
    const char *value;
    struct ast_parameter *next;
} ast_param_t;

typedef struct ast_domain {
    const char *name;
    ast_param_t *parameters;
    struct ast_domain *next;
} ast_domain_t;

typedef struct ast_interface {
    const char *name;
    ast_domain_t *domains;
    ast_param_t *parameters;
    struct ast_interface *next;
} ast_iface_t;

typedef struct ast {
    ast_iface_t *interfaces;
    ast_param_t *parameters;
} ast_t;

ast_t *
new_ast(ast_iface_t *i, ast_param_t *p);

ast_iface_t *
new_interface(const char *name, ast_domain_t *d, ast_param_t *p);

ast_domain_t *
new_domain(const char *name, ast_param_t *p);

ast_param_t *
new_parameter(int parameter_type, const char *value);

bool
valid_ast(ast_t *ast);
