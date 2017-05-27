#include "config.h"
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include <sys/socket.h>

extern FILE *yyin;
extern int yyparse();

const char const *usage = "usage: " EXENAME " [-dn] [-f file]\n"
			  "       " EXENAME " -v\n"
			  "       " EXENAME " -h";

static uint8_t
min(uint8_t a, uint8_t b) {
    return a < b ? a : b;
}

static struct sockaddr *
find_sa(struct ifaddrs **ifap, char *ifname) {
    struct ifaddrs *ifa = *ifap;
    while (ifa) {
        if (strncmp(ifa->ifa_name, ifname, strlen(ifname)) == 0)
            return ifa->ifa_addr;
        ifa = ifa->ifa_next;
    } 
    return NULL;
}

int
main(int argc, char *argv[]) {
    int opt;
    bool optn = false;
    char *conf = ETCFILE;

    while ((opt = getopt(argc, argv, "hnvf:")) != -1) {
        switch (opt) {
        default:
        case 'h':
            puts(usage);
            exit(EXIT_SUCCESS);
        case 'v':
            puts(VERSION);
            exit(EXIT_SUCCESS);
        case 'f':
            conf = optarg;
            break;
        case 'n':
            optn = true;
            break;
        }
    }

    if (!(yyin = fopen(conf, "r")))
        err(EXIT_FAILURE, "fopen(\"%s\")", conf);

    int parse_status = yyparse();
    fclose(yyin);

    if (optn) exit(parse_status);

    /*
    if (daemon(0, 0) == -1)
        err(EXIT_FAILURE, "daemon(3)");

    //TODO: set up libcurl

    openlog(NULL, LOG_PID, LOG_DAEMON);

    if (pledge("stdio", NULL) == -1)
        syslog(LOG_ERR, "pledge(2): %m");
    */

    struct ifaddrs **ifap0, **ifap1;
    struct sockaddr *sa0, *sa1;

    if (getifaddrs(ifap0) == -1)
        syslog(LOG_ERR, "getifaddrs(3): %m");

    while (true) {
        if (getifaddrs(ifap1) == -1) {
            syslog(LOG_ERR, "getifaddrs(3): %m");
            goto sleep;
        }

        // loop over tracked interfaces

        sa0 = find_sa(ifap0, "em0");
        sa1 = find_sa(ifap1, "em0");

        // if different, call libcurl
        if (memcmp(sa0, sa1, min(sa0->sa_len, sa1->sa_len)) != 0) {

        }

        freeifaddrs(*ifap0);
        ifap0 = ifap1;
sleep:
        sleep(60);
    }
}
