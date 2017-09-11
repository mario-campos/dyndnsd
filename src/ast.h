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
 * Deallocate Abstract Syntax Tree.
 */
void ast_free(struct ast *);

/*
 * Parse an Abstract Syntax Tree from a configuration file.
 */
bool ast_load(struct ast **, FILE *);

#endif /* AST_H */
