#include <sys/socket.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "rtm.h"

#define ROUNDUP(a) \
    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

static struct sockaddr *get_sa(uint8_t *, int);

char *rtm_getifname(struct rt_msghdr *rtm)
{
	struct ifa_msghdr *ifam = (struct ifa_msghdr *)rtm;
	char *buf = malloc(IF_NAMESIZE);
	return if_indextoname(ifam->ifam_index, buf);
}

char *rtm_getipaddr(struct ifa_msghdr *ifam)
{
	struct in_addr addr;
	struct sockaddr *sa;
	struct sockaddr_in *sin;

	sa = get_sa((uint8_t *)ifam + ifam->ifam_hdrlen, ifam->ifam_addrs);
	sin = (struct sockaddr_in *)sa;

	assert(AF_INET == sin->sin_family);

	memcpy(&addr, &sin->sin_addr, sizeof(addr));

	assert(INADDR_ANY != addr.s_addr);

	return inet_ntoa(addr);
}

static struct sockaddr *
get_sa(uint8_t *cp, int flags) {
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
