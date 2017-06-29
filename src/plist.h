#ifndef PLIST_H
#define PLIST_H

#include <sys/queue.h>

SLIST_HEAD(plist, pnode);

struct pnode {
	char *field;
	char *value;
	SLIST_ENTRY(pnode) next;
};

/*
 * Parse the HTTP query parameters.
 */
struct plist *plist_parse(const char *);

/*
 * Define a query parameter, new or existing.
 */
void plist_setparam(struct plist *, const char *, const char *);

/*
 * Construct a new URL from an old URL and the new
 * query parameters.
 */
char *plist_mkurl(const char *, const struct plist *);

/*
 * Deallocate query parameters.
 */
void plist_free(struct plist *);

#endif /* PLIST_H */
