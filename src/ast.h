#ifndef AST_H
#define AST_H

#include <stdio.h>

struct ast_iface {
	unsigned int if_index;
	char *if_name;
	size_t domain_len;
	char *domain[];
};

struct ast_root {
	char *cmd;
	size_t iface_len;
	struct ast_iface *iface[];
};

extern struct ast_root *ast;

#endif /* AST_H */
