#ifndef PARSER_H
#define PARSER_H

#include <net/if.h>
#include <sys/queue.h>

struct ast_dn {
	SLIST_ENTRY(ast_dn) next;
	char name[];
};

struct ast_if {
	SLIST_ENTRY(ast_if) next;
	SLIST_HEAD(, ast_dn) adn_head;
	char name[IFNAMSIZ];
};

struct ast {
	const char *run;
	SLIST_HEAD(, ast_if) aif_head;
};

struct ast *parse(const char *);
extern char parser_error[];

#endif /* PARSER_H */
