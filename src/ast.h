#ifndef AST_H
#define AST_H

#include <sys/queue.h>
#include <stdbool.h>
#include <stdio.h>

#define IFNAMSIZ		16 // also in net/if.h
#define OPENBSD_USERNAME_SIZE 	32
#define OPENBSD_GROUPNAME_SIZE 	32

struct ast_domain {
	const char *domain;
	const char *url;
	SLIST_ENTRY(ast_domain) next;
};

struct ast_iface {
	char if_name[IFNAMSIZ];
	SLIST_HEAD(, ast_domain) domains;
	SLIST_ENTRY(ast_iface) next;
};

struct ast_root {
	char user[OPENBSD_USERNAME_SIZE];
	char group[OPENBSD_GROUPNAME_SIZE];
	SLIST_HEAD(, ast_iface) interfaces;
};

/*
 * Deallocate Abstract Syntax Tree.
 */
void ast_free(struct ast_root *);

/*
 * Parse an Abstract Syntax Tree from a configuration file.
 */
bool ast_load(struct ast_root **, FILE *);

#endif /* AST_H */
