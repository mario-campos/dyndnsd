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
 * Allocate a new Abstract Syntax Tree (AST).
 */
struct ast *new_ast(struct ast_iface *, const char *);

/*
 * Allocate a new AST node for interfaces.
 */
struct ast_iface *new_ast_iface(const char *, struct ast_domain *, const char *);

/*
 * Allocate a new AST node for domain names.
 */
struct ast_domain *new_ast_domain(const char *, const char *);

/*
 * Check the AST for illogical configurations.
 */
bool valid_ast(struct ast *);
