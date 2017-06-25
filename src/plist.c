#include <sys/queue.h>

#include <stdlib.h>
#include <string.h>

#include "plist.h"

static void addparam(struct plist *, const char *, const char *);

struct plist *
getparams(const char *url)
{
	struct plist   *list;
	char           *r, *s, *field, *value;

	r = s = strdup(url);
	list = malloc(sizeof(struct plist));
	SLIST_INIT(list);

	if (strsep(&s, "?"), NULL == s)
		goto end;

	while (s) {
		value = strsep(&s, "&");
		field = strsep(&value, "=");

		struct pnode *p = malloc(sizeof(struct pnode));
		p->field = strdup(field);
		p->value = strdup(value);
		SLIST_INSERT_HEAD(list, p, next);
	}

end:
	free(r);
	return list;
}

void
setparam(struct plist *list, const char *field, const char *value)
{
	struct pnode *p;
	SLIST_FOREACH(p, list, next) {
		if (0 == strcmp(p->field, field)) {
			free(p->value);
			p->value = strdup(value);
			return;
		}
	}
	addparam(list, field, value);
}

void
freeparams(struct plist *list)
{
	while (!SLIST_EMPTY(list)) {
		struct pnode *p = SLIST_FIRST(list);
		SLIST_REMOVE_HEAD(list, next);
		free(p->field);
		free(p->value);
		free(p);
	}
}

char *
mkurl(const char *url, struct plist *list)
{
	struct pnode   *p;
	char           *r, *s;

	r = s = strdup(url);
	s = strsep(&s, "?");
	char buf[strlen(s) + 2048];
	strlcpy(buf, s, sizeof(buf));
	strlcat(buf, "?", sizeof(buf));
	SLIST_FOREACH(p, list, next) {
		strlcat(buf, p->field, sizeof(buf));
		strlcat(buf, "=", sizeof(buf));
		strlcat(buf, p->value, sizeof(buf));
		if (SLIST_NEXT(p, next))
			strlcat(buf, "&", sizeof(buf));
	}

	free(r);
	return strdup(buf);
}

static void
addparam(struct plist *list, const char *field, const char *value)
{
	struct pnode *p = malloc(sizeof(struct pnode));
	p->field = strdup(field);
	p->value = strdup(value);
	SLIST_INSERT_HEAD(list, p, next);
}
