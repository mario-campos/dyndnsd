#ifndef AST_H
#define AST_H

#include <stdio.h>

struct ast_domain {
	char *domain;
	char *url;
};

struct ast_iface {
	char *if_name;
	size_t domain_len;
	struct ast_domain *domain[];
};

struct ast_root {
	char *user;
	char *group;
	size_t iface_len;
	struct ast_iface *iface[];
};

struct ast_domain *ast_domain_new(char *, char *);
struct ast_iface *ast_iface_new(char *, int);
bool ast_load(struct ast_root **, FILE *);
bool ast_reload(struct ast_root **, FILE *);

#endif /* AST_H */
