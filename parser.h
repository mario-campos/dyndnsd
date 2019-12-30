#ifndef PARSER_H
#define PARSER_H

#include <net/if.h>
#include <sys/queue.h>

struct ast_domain {
	SLIST_ENTRY(ast_domain) next;
	char domain[];
};

struct ast_iface {
	char if_name[IFNAMSIZ];
	SLIST_HEAD(, ast_domain) domains;
	SLIST_ENTRY(ast_iface) next;
};

struct ast {
	const char *cmd;
	SLIST_HEAD(, ast_iface) ifaces;
};

struct ast *parse(const char *);
extern char parser_error[];

#endif /* PARSER_H */
