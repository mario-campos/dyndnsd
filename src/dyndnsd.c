#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <ifaddrs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <curl/curl.h>

#include "ast.h"
#include "config.h"
#include "param.h"
#include "pathnames.h"

extern FILE *yyin;
extern int yyparse();

static void __dead usage(void);

/*
 * Determine whether two IPv4 address structures
 * are equal.
 */
static bool inaddreq(struct in_addr, struct in_addr);

/*
 * Set up the curl request, execute, and log
 * and errors.
 */
static void httpget(CURL *, const char *);

/*
 * Search the list of interface structures for
 * by the given interface name and return a
 * pointer to it.
 */
static struct sockaddr_in *find_sa(struct ifaddrs *, const char *);

/*
 * Check network interfaces for IPv4 address changes
 * every minute. When an interface receives a new IP
 * address, send an HTTP request to update a DNS
 * hosting provider.
 */
int
main(int argc, char *argv[]) {
    int opt, error;
    bool optd, optn;
    char *optf;

    optn = false;
    optd = false;
    optf = _PATH_DYNDNSD_CONF;

    while ((opt = getopt(argc, argv, "hdnvf:")) != -1) {
        switch (opt) {
        case 'd':
            optd = true;
            break;
        case 'h':
	    usage();
        case 'f':
            optf = optarg;
            break;
        case 'n':
            optn = true;
            break;
        case 'v':
            puts(VERSION);
            exit(0);
        default:
	    usage();
        }
    }

    yyin = fopen(optf, "r");
    if (yyin == NULL)
        err(1, "fopen(\"%s\")", optf);

    struct ast *ast;
    error = yyparse(&ast);
    fclose(yyin);

    if (!valid_ast(ast) || optn)
        exit(error);

    // initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    if (curl == NULL)
        err(1, "curl_easy_init(3): failed to initialize libcurl");

    if (!optd)
        daemon(0, 0);

    openlog(NULL, (optd ? LOG_PERROR : 0) | LOG_PID, LOG_DAEMON);

    struct ifaddrs *ifap_old, *ifap_new;
    getifaddrs(&ifap_old);

    while (true) {
        if (getifaddrs(&ifap_new) == -1) {
            syslog(LOG_ERR, "getifaddrs(3): %m");
            goto sleep;
        }

        for (struct ast_iface *aif = ast->interfaces; aif->next; aif = aif->next) {
            struct sockaddr_in *sa_old, *sa_new;
            sa_old = find_sa(ifap_old, aif->name);
            sa_new = find_sa(ifap_new, aif->name);

            if (!inaddreq(sa_old->sin_addr, sa_new->sin_addr))
                continue;

            struct param *p;
	    p = getparams(aif->url);
            p = setparam(p, "myip", inet_ntoa(sa_new->sin_addr));
            p = setparam(p, "hostname", aif->domains->name);
            char *url = mkurl(aif->url, p);

            httpget(curl, url);

	    free(url);
	    freeparams(p);
        }

        freeifaddrs(ifap_old);
        ifap_old = ifap_new;
sleep:
        sleep(60);
    }

    curl_easy_cleanup(curl);
}

static void __dead
usage(void) {
    extern char *__progname;
    fprintf(stderr, "usage: %s [-dhnv] [-f file]\n", __progname);
    exit(0);
}

static bool
inaddreq(struct in_addr a, struct in_addr b) {
    return memcmp(&a, &b, sizeof(struct in_addr)) == 0;
}

static void
httpget(CURL *curl, const char *url) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    CURLcode err = curl_easy_perform(curl);
    if (err)
        syslog(LOG_ERR, "curl_easy_perform(3): %s", curl_easy_strerror(err));
}

static struct sockaddr_in *
find_sa(struct ifaddrs *ifa, const char *ifname) {
    while (ifa) {
        if (strcmp(ifa->ifa_name, ifname) == 0)
            return (struct sockaddr_in *)ifa->ifa_addr;
        ifa = ifa->ifa_next;
    }
    return NULL;
}
