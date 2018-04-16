#ifndef DIE_H
#define DIE_H

#include <syslog.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT(x) "[" __FILE__ ":" TOSTRING(__LINE__) "] " x

__dead void die(int, const char *, ...);

#endif /* DIE_H */
