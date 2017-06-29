#include <sys/socket.h>
#include <net/route.h>

#include <err.h>

#include "route.h"

int
rt_socket()
{
	int routefd = socket(PF_ROUTE, SOCK_RAW, AF_INET);
	if (-1 == routefd)
		err(1, "socket(PF_ROUTE)");

	unsigned int rtfilter = ROUTE_FILTER(RTM_NEWADDR);
	if (-1 == setsockopt(routefd, PF_ROUTE, ROUTE_MSGFILTER,
			     &rtfilter, sizeof(rtfilter)))
		err(1, "setsockopt(ROUTE_MSGFILTER)");

	return routefd;
}
