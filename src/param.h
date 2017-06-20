#pragma once

typedef struct param {
    const char *field;
    const char *value;
    struct param *next;
} param_t;

param_t	*getparams(const char *);
param_t *setparam(param_t *, const char *, const char *);
char	*mkurl(const char *, param_t *);
void 	 freeparams(param_t *);
