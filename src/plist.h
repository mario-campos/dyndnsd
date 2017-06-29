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
struct plist *getparams(const char *);

/*
 * Define a query parameter, new or existing.
 */
void setparam(struct plist *, const char *, const char *);

/*
 * Construct a new URL from an old URL and the new
 * query parameters.
 */
char *mkurl(const char *, struct plist *);

/*
 * Deallocate query parameters.
 */
void freeparams(struct plist *);

#endif /* PLIST_H */
