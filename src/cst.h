#ifndef CST_H
#define CST_H
#include <sys/queue.h>

SLIST_HEAD(cst_list, cst_node);

struct cst_node {
	int type;
	char *string;
	struct cst_list label;
	struct cst_list children;
	SLIST_ENTRY(cst_node) next;
};

struct cst_node *cst_node(int, char *, struct cst_node *, struct cst_node *);
void cst_free(struct cst_node *);
struct ast_root *cst_convert(struct cst_node *);

#endif /* CST_H */
