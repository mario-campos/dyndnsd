#pragma once

#include <stdbool.h>

typedef struct param {
    const char *field;
    const char *value;
    struct param *next;
} param_t;

param_t *
getparams(const char *url);

void
setparam(param_t *p, const char *field, const char *value);

void
freeparams(param_t *p);

char *
mkurl(const char *url, param_t *p);
