#ifndef RTM_H
#define RTM_H

#include <net/route.h>

char *rtm_getifname(struct rt_msghdr *);

char *rtm_getipaddr(struct ifa_msghdr *);

#endif /* RTM_H */
