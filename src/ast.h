#ifndef AST_H
#define AST_H

#include <stdio.h>

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
bool ast_load(struct ast_root **, FILE *);

extern struct ast_root **ast;

#endif /* AST_H */
