#ifndef CST_H
#define CST_H
#include <sys/queue.h>

SLIST_HEAD(cst_list, cst_node);

struct cst_node {
	int type;
	char *string;
	SLIST_ENTRY(cst_node) next;
	struct cst_list children;
};

struct cst_node *cst_node(int, char *, struct cst_node *);
void cst_free(struct cst_node *);
struct ast_root *cst2ast(struct cst_node *);
size_t sllist_counttype(struct cst_list *, int);

#endif /* CST_H */
