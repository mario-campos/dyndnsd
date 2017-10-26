#ifndef AST_H
#define AST_H

#include <sys/queue.h>

#define IFNAMSIZ		16 // also in net/if.h

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
	const char *user;
	const char *group;
	SLIST_HEAD(, ast_iface) interfaces;
};

#endif /* AST_H */
