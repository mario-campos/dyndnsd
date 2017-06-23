#pragma once

struct param {
	char           *field;
	struct param   *next;
	char           *value;
};

/*
 * Parse the HTTP query parameters.
 */
struct param   *getparams(const char *);

/*
 * Define a query parameter, new or existing.
 */
struct param   *setparam(struct param *, const char *, const char *);

/*
 * Construct a new URL from an old URL and the new
 * query parameters.
 */
char           *mkurl(const char *, struct param *);

/*
 * Deallocate query parameters.
 */
void 		freeparams(struct param *);
