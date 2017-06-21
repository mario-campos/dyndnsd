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

struct ast *new_ast(struct ast_iface *, const char *);
struct ast_iface *new_ast_iface(const char *, struct ast_domain *, const char *);
struct ast_domain *new_ast_domain(const char *, const char *);
bool valid_ast(struct ast *);
