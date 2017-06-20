#pragma once

struct param {
    char	 *field;
    struct param *next;
    char	 *value;
};

/*
 * Parse the given URL for its query parameters.
 */
struct param *
getparams(const char *);

/*
 * Set the query parameter by the field 'field' to
 * a value of 'value'. If the parameter does not exist,
 * define it.
 */
struct param *
setparam(struct param *, const char *, const char *);

/*
 * Construct a new URL using the given query parameters
 * instead of the query parameters in the URL, if any.
 */
char *
mkurl(const char *, struct param *);

/*
 * Deallocate a list of query parameters.
 */
void
freeparams(struct param *);
