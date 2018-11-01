#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "rtm.h"

#define RTM_BUF_SIZE 512
#define ROUNDUP(a) \
	    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

int
rtm_socket(void)
{
	int routefd;
	unsigned int rtfilter;

	routefd = socket(PF_ROUTE, SOCK_CLOEXEC|SOCK_RAW, AF_INET);
	if (-1 == routefd)
		return -1;

	rtfilter = ROUTE_FILTER(RTM_NEWADDR);
	if (-1 == setsockopt(routefd, PF_ROUTE, ROUTE_MSGFILTER,
			     &rtfilter, (socklen_t)sizeof(rtfilter))) {
		close(routefd);
		return -1;
	}

	return routefd;
}

ssize_t
rtm_consume(int routefd, struct rtm_newaddr *dst)
{
	assert(dst);

	uint8_t buf[RTM_BUF_SIZE], *p;
	struct sockaddr **sa;
	struct sockaddr_dl **sdl;
	struct sockaddr_in **sin;
	struct ifa_msghdr *ifam;

	if (-1 == read(routefd, buf, sizeof(buf)))
		return -1;

	p = buf;
	sa = (struct sockaddr **)&p;
	sdl = (struct sockaddr_dl **)&p;
	sin = (struct sockaddr_in **)&p;
	ifam = (struct ifa_msghdr *)buf;

	p += ifam->ifam_hdrlen;

	for (int i = 1; i; i <<= 1) {
		if (!(ifam->ifam_addrs & i))
			continue;

		if (RTA_NETMASK == i) {
			dst->rtm_ifmask.s_addr = (*sin)->sin_addr.s_addr;
		}
		else if (RTA_IFP == i) {
			strncpy(&dst->rtm_ifname[0], (*sdl)->sdl_data, (*sdl)->sdl_nlen);
			dst->rtm_ifname[(*sdl)->sdl_nlen] = '\0';
		}
		else if (RTA_IFA == i) {
			dst->rtm_ifaddr.s_addr = (*sin)->sin_addr.s_addr;
		}
		else if (RTA_BRD == i) {
			dst->rtm_ifbcast.s_addr = (*sin)->sin_addr.s_addr;
		}

		p += ROUNDUP((*sa)->sa_len);
	}

	return p - buf;
}
