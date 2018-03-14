#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>

#include "rtm.h"

#define ROUNDUP(a) \
	    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

int
rtm_socket()
{
	int routefd;
	unsigned int rtfilter;

	routefd = socket(PF_ROUTE, SOCK_CLOEXEC|SOCK_RAW, AF_INET);
	if (-1 == routefd)
		return -1;

	rtfilter = ROUTE_FILTER(RTM_NEWADDR);
	if (-1 == setsockopt(routefd, PF_ROUTE, ROUTE_MSGFILTER,
			     &rtfilter, (socklen_t)sizeof(rtfilter)))
		return -1;

	return routefd;
}

char *
rtm_getifname(struct rt_msghdr *rtm)
{
	struct ifa_msghdr *ifam = (struct ifa_msghdr *)rtm;
	char *buf = malloc((size_t)IF_NAMESIZE);
	return if_indextoname(ifam->ifam_index, buf);
}

char *
rtm_getipaddr(struct ifa_msghdr *ifam)
{
	struct in_addr addr;
	struct sockaddr *sa;
	struct sockaddr_in *sin;

	sa = rtm_getsa((uint8_t *)ifam + ifam->ifam_hdrlen, ifam->ifam_addrs);
	sin = (struct sockaddr_in *)sa;

	memcpy(&addr, &sin->sin_addr, sizeof(addr));

	return inet_ntoa(addr);
}

struct sockaddr *
rtm_getsa(uint8_t *cp, int flags)
{
	int i;
	struct sockaddr *sa;

	if (0 == flags)
		return NULL;

	for (i = 1; i; i <<= 1) {
		if (flags & i) {
			sa = (struct sockaddr *)cp;
			if (i == RTA_IFA)
				return sa;
			ADVANCE(cp, sa);
		}
	}

	return NULL;
}
