#ifndef PARSER_H
#define PARSER_H

#include <net/if.h>
#include <sys/queue.h>

struct cst_domain {
	SLIST_ENTRY(cst_domain) next;
	char domain[];
};

struct cst_iface {
	char if_name[IFNAMSIZ];
	SLIST_HEAD(, cst_domain) domains;
	SLIST_ENTRY(cst_iface) next;
};

struct cst {
	const char *cmd;
	SLIST_HEAD(, cst_iface) ifaces;
};

struct cst *parse(const char *);
extern char parser_error[];

#endif /* PARSER_H */
