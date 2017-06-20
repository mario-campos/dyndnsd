#pragma once

#include <stdbool.h>

typedef struct ast_domain {
    const char        *name;
    struct ast_domain *next;
    const char        *url;
} ast_domain_t;

typedef struct ast_iface {
    ast_domain_t     *domains;
    const char       *name;
    struct ast_iface *next;
    const char       *url;
} ast_iface_t;

typedef struct ast {
    ast_iface_t *interfaces;
    const char  *url;
} ast_t;

ast_t		*new_ast(ast_iface_t *, const char *);
ast_iface_t	*new_ast_iface(const char *, ast_domain_t *, const char *);
ast_domain_t	*new_ast_domain(const char *, const char *);
bool		 valid_ast(ast_t *);
