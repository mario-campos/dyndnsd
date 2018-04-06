#ifndef PROCESS_H
#define PROCESS_H

#include <unistd.h>

pid_t spawn(char *, int);
void set_dyndnsd_env(char *, char *);
char *getshell(void);

#endif /* PROCESS_H */
