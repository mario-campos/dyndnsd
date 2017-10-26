#ifndef AST_H
#define AST_H

#include <sys/queue.h>
#include <stdio.h>

struct ast_domain {
	const char *domain;
	const char *url;
	SLIST_ENTRY(ast_domain) next;
};

struct ast_iface {
	const char *if_name;
	SLIST_HEAD(, ast_domain) domains;
	SLIST_ENTRY(ast_iface) next;
};

struct ast_root {
	const char *user;
	const char *group;
	SLIST_HEAD(, ast_iface) interfaces;
};


void ast_free(struct ast_root *);
bool ast_load(struct ast_root **, FILE *);
bool ast_reload(struct ast_root **, FILE *);

#endif /* AST_H */
