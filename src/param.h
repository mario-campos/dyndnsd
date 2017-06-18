#pragma once

#include <stdbool.h>

typedef struct param {
    const char *field;
    const char *value;
    struct param *next;
} param_t;

param_t *
parse_params(const char *url);

bool
has_param(param_t *p, const char *field);

void
add_param(param_t *p, const char *field, const char *value);

void
free_params(param_t *p);

char *
build_url(const char *url, param_t *p);
