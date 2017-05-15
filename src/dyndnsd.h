#pragma once

typedef struct ast_parameter {
    int ptype;
    const char *value;
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
