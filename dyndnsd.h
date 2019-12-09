#ifndef DYNDNSD_H
#define DYNDNSD_H

#define DYNDNSD_VERSION "0.1.0"

#define DYNDNSD_DEBUG_MODE 0x0001
#define DYNDNSD_VALID_MODE 0x0002

struct dyndnsd {
	int	routefd;
	int 	devnull;
	struct ast *ast;
};

#endif /* DYNDNSD_H */
