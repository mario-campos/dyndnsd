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

ast_t		*new_ast(ast_iface_t *, const char *);
ast_iface_t	*new_ast_iface(const char *, ast_domain_t *, const char *);
ast_domain_t	*new_ast_domain(const char *, const char *);
bool		 valid_ast(ast_t *);
