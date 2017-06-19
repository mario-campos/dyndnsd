#include <string.h>
#include <stdlib.h>
#include "param.h"

param_t *
getparams(const char *url) {
    char *r, *s;
    r = s = strdup(url);

    if (strsep(&s, "?"), !s) {
	free(r);
        return NULL;
    }

    param_t *p = calloc(1, sizeof(param_t));

    for (param_t *q = p; s; q = q->next) {
        char *value = strsep(&s, "&");
	char *field = strsep(&value, "=");
	q->field = strdup(field);
	q->value = strdup(value);
	if (s) q->next = calloc(1, sizeof(param_t));
    }

    free(r);
    return p;
}

static param_t * 
addparam(param_t *p, const char *field, const char *value) {
    if (!p) {
        p = calloc(1, sizeof(param_t));
        p->field = strdup(field);
        p->value = strdup(value);
    } else {
        param_t *q;
        for (q = p; q->next; q = q->next);
        q->next = calloc(1, sizeof(param_t));
        q->next->field = strdup(field);
        q->next->value = strdup(value);
    }
    return p;
}

param_t *
setparam(param_t *p, const char *field, const char *value) {
    for (param_t *q = p; q; q = q->next) {
	if (!strcmp(q->field, field)) {
            free((char *)q->value);
	    q->value = strdup(value);
	    return p;
        }
    }
    return addparam(p, field, value);
}

void
freeparams(param_t *p) {
    for (param_t *q; p; p = q) {
	q = p->next;
        free((char *)p->field);
	free((char *)p->value);
        free(p);
    }
}

char *
mkurl(const char *url, param_t *p) {
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
	if (p->next) strlcat(buf, "&", sizeof(buf));
    }

    free(r);
    return strdup(buf);
}
