#ifndef RTM_H
#define RTM_H

#include <net/if.h>
#include <net/route.h>

/*
 * Create a route(4) socket.
 */
int rtm_socket(unsigned int);

/*
 * Parse a Route Message for an interface name.
 */
char *rtm_getifname(struct rt_msghdr *);

/*
 * Parse a Route Message for an IP address.
 */
char *rtm_getipaddr(struct ifa_msghdr *);

#endif /* RTM_H */
