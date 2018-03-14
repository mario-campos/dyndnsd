#ifndef RTM_H
#define RTM_H

#define ROUNDUP(a) \
	    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

int rtm_socket();
char *rtm_getifname(void*);
char *rtm_getipaddr(void*);
struct sockaddr *rtm_getsa(uint8_t*, int);

#endif /* RTM_H */
