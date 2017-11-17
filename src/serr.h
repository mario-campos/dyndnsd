#ifndef SERR_H
#define SERR_H

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT(x) "[" __FILE__ ":" TOSTRING(__LINE__) "] " x

__dead void serr(int, int, const char *);
__dead void serrx(int, int, const char *);

#endif /* SERR_H */
