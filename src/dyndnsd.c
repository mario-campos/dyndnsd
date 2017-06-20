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

extern FILE *yyin;
extern int yyparse();

const char const *usage = "usage: " EXENAME " [-dn] [-f file]\n"
			  "       " EXENAME " -v\n"
			  "       " EXENAME " -h";

static bool
inaddreq(struct in_addr a, struct in_addr b) {
    return memcmp(&a, &b, sizeof(struct in_addr)) == 0;
}

static void
httpget(CURL *curl, const char *url) {
    CURLcode err;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if ((err = curl_easy_perform(curl)))
        syslog(LOG_ERR, "curl_easy_perform(3): %s", curl_easy_strerror(err));
}

static struct sockaddr_in *
find_sa(struct ifaddrs *ifap, const char *ifname) {
    struct ifaddrs *ifa = ifap;
    while (ifa) {
        if (!strcmp(ifa->ifa_name, ifname))
            return (struct sockaddr_in *)ifa->ifa_addr;
        ifa = ifa->ifa_next;
    } 
    return NULL;
}

int
main(int argc, char *argv[]) {
    int opt;
    bool optn = false, optd = false;
    char *optf = ETCFILE;
    while ((opt = getopt(argc, argv, "hdnvf:")) != -1) {
        switch (opt) {
        default:
        case 'h':
            puts(usage);
            exit(EXIT_SUCCESS);
        case 'v':
            puts(VERSION);
            exit(EXIT_SUCCESS);
        case 'f':
            optf = optarg;
            break;
        case 'n':
            optn = true;
            break;
        case 'd':
            optd = true;
            break;
        }
    }

    if (!(yyin = fopen(optf, "r")))
        err(EXIT_FAILURE, "fopen(\"%s\")", optf);

    struct ast *ast;
    int parse_err = yyparse(&ast);
    fclose(yyin);

    if (!valid_ast(ast) || optn)
        exit(parse_err);

    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    if (!(curl = curl_easy_init()))
        err(EXIT_FAILURE, "curl_easy_init(3): failed to initialize libcurl");

    if (!optd)
        daemon(0, 0);

    openlog(NULL, (optd ? LOG_PERROR : 0) | LOG_PID, LOG_DAEMON);

    /*
    if (pledge("stdio", NULL) == -1)
        syslog(LOG_ERR, "pledge(2): %m");
    */

    struct ifaddrs *ifap_old, *ifap_new;
    getifaddrs(&ifap_old);

    while (true) {
        if (getifaddrs(&ifap_new) == -1) {
            syslog(LOG_ERR, "getifaddrs(3): %m");
            goto sleep;
        }

        for (ast_iface_t *aif = ast->interfaces; aif->next; aif = aif->next) {
            struct sockaddr_in *sa_old = find_sa(ifap_old, aif->name);
            struct sockaddr_in *sa_new = find_sa(ifap_new, aif->name);

            if (!inaddreq(sa_old->sin_addr, sa_new->sin_addr))
                continue;

            param_t *p = getparams(aif->url);
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
