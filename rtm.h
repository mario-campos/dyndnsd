#ifndef RTM_H
#define RTM_H

#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>

struct rtm_newaddr {
	char           rtm_ifname[IF_NAMESIZE];
	struct in_addr rtm_ifaddr;
	struct in_addr rtm_ifmask;
	struct in_addr rtm_ifbcast;
};

int rtm_socket(void);
ssize_t rtm_consume(int, struct rtm_newaddr *);

#endif /* RTM_H */
