#ifndef AST_H
#define AST_H

#include <sys/queue.h>

#include <stdbool.h>
#include <stdio.h>

SLIST_HEAD(ast_domains, ast_domain);

struct ast_domain {
	const char     *name;
	const char     *url;
	SLIST_ENTRY(ast_domain) next;
};

SLIST_HEAD(ast_ifaces, ast_iface);

struct ast_iface {
	const char     *name;
	const char     *url;
	struct ast_domains *domains;
	SLIST_ENTRY(ast_iface) next;
};

struct ast {
	const char     *url;
	struct ast_ifaces *interfaces;
};


/*
 * Allocate a new Abstract Syntax Tree (AST).
 */
struct ast *ast_new(struct ast_ifaces *, const char *);

/*
 * Allocate a new AST node for interfaces.
 */
struct ast_ifaces *ast_new_iface(const char *, struct ast_domains *, const char *);

/*
 * Allocate a new AST node for domain names.
 */
struct ast_domains *ast_new_domain(const char *, const char *);

/*
 * Combine two Singly-Linked Lists of interfaces.
 */
struct ast_ifaces *ast_merge_ifaces(struct ast_ifaces *, struct ast_ifaces *);

/*
 * Combine two Singly-Linked Lists of domains.
 */
struct ast_domains *ast_merge_domains(struct ast_domains *, struct ast_domains *);

/*
 * Deallocate Abstract Syntax Tree.
 */
void ast_free(struct ast *);

/*
 * Parse an Abstract Syntax Tree from a configuration file.
 */
int ast_load(struct ast **, FILE *);

#endif /* AST_H */
