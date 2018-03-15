#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdbool.h>

struct ast_iface {
	char *if_name;
	size_t domain_len;
	char *domain[];
};

struct ast_root {
	char *cmd;
	size_t iface_len;
	struct ast_iface *iface[];
};

struct ast_iface *ast_iface_new(char *, size_t);
struct ast_iface *ast_iface_find(struct ast_root *, char *);
bool ast_load(struct ast_root **, FILE *);

#endif /* AST_H */
