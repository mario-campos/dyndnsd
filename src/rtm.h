#ifndef RTM_H
#define RTM_H

#include <net/route.h>

/*
 * Parse a Route Message for an interface name.
 */
char *rtm_getifname(struct rt_msghdr *);

/*
 * Parse a Route Message for an IP address.
 */
char *rtm_getipaddr(struct ifa_msghdr *);

#endif /* RTM_H */
