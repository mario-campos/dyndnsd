#ifndef AST_H
#define AST_H

#include <stdbool.h>

struct ast_domain {
	const char     *name;
	struct ast_domain *next;
	const char     *url;
};

struct ast_iface {
	struct ast_domain *domains;
	const char     *name;
	struct ast_iface *next;
	const char     *url;
};

struct ast {
	struct ast_iface *interfaces;
	const char     *url;
};

/*
 * Allocate a new Abstract Syntax Tree (AST).
 */
struct ast     *ast_new(struct ast_iface *, const char *);

/*
 * Allocate a new AST node for interfaces.
 */
struct ast_iface *ast_new_iface(const char *, struct ast_domain *, const char *);

/*
 * Allocate a new AST node for domain names.
 */
struct ast_domain *ast_new_domain(const char *, const char *);

/*
 * Check the AST for illogical configurations.
 */
bool 		ast_is_valid(struct ast *);

#endif /* AST_H */
