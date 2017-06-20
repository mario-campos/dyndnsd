#pragma once

#include <stdbool.h>

struct ast_domain {
    const char        *name;
    struct ast_domain *next;
    const char        *url;
};

struct ast_iface {
    struct ast_domain *domains;
    const char        *name;
    struct ast_iface  *next;
    const char        *url;
};

struct ast {
    struct ast_iface	*interfaces;
    const char		*url;
};

/*
 * Allocate an Abstract Syntax Tree (AST) structure.
 */
struct ast *
new_ast(struct ast_iface *, const char *);

/*
 * Allocate a network-interface node for an AST.
 */
struct ast_iface *
new_ast_iface(const char *, struct ast_domain *, const char *);

/*
 * Allocate a domain-name node for an AST.
 */
struct ast_domain *
new_ast_domain(const char *, const char *);

/*
 * Validate an AST.
 */
bool
valid_ast(struct ast *);
