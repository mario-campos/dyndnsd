#pragma once

struct param {
    const char   *field;
    struct param *next;
    const char   *value;
};

struct param	*getparams(const char *);
struct param	*setparam(struct param *, const char *, const char *);
char		*mkurl(const char *, struct param *);
void		 freeparams(struct param *);
