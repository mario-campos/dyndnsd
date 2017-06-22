#include <stdlib.h>
#include <string.h>

#include "param.h"

static struct param *addparam(struct param *, const char *, const char *);

struct param *
getparams(const char *url) {
    struct param *p;
    char *r, *s, *field, *value;

    r = s = strdup(url);

    if (strsep(&s, "?"), NULL == s) {
	free(r);
        return NULL;
    }

    p = calloc(1, sizeof(struct param));

    for (struct param *q = p; s; q = q->next) {
        value = strsep(&s, "&");
	field = strsep(&value, "=");
	q->field = strdup(field);
	q->value = strdup(value);
	if (s)
	    q->next = calloc(1, sizeof(struct param));
    }

    free(r);
    return p;
}

struct param *
setparam(struct param *p, const char *field, const char *value) {
    for (struct param *q = p; q; q = q->next) {
	if (0 == strcmp(q->field, field)) {
            free(q->value);
	    q->value = strdup(value);
	    return p;
        }
    }
    return addparam(p, field, value);
}

void
freeparams(struct param *p) {
    for (struct param *q; p; p = q) {
	q = p->next;
        free(p->field);
	free(p->value);
        free(p);
    }
}

char *
mkurl(const char *url, struct param *p) {
    char *r, *s;

    r = s = strdup(url);
    s = strsep(&s, "?");
    char buf[strlen(s) + 2048];
    strlcpy(buf, s, sizeof(buf));
    strlcat(buf, "?", sizeof(buf));
    for (; p; p = p->next) {
	strlcat(buf, p->field, sizeof(buf));
	strlcat(buf, "=", sizeof(buf));
	strlcat(buf, p->value, sizeof(buf));
	if (p->next)
	    strlcat(buf, "&", sizeof(buf));
    }

    free(r);
    return strdup(buf);
}

static struct param * 
addparam(struct param *p, const char *field, const char *value) {
    if (NULL == p) {
        p = calloc(1, sizeof(struct param));
        p->field = strdup(field);
        p->value = strdup(value);
    } else {
        struct param *q;
        for (q = p; q->next; q = q->next);
        q->next = calloc(1, sizeof(struct param));
        q->next->field = strdup(field);
        q->next->value = strdup(value);
    }
    return p;
}
