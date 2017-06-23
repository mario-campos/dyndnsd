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

extern FILE    *yyin;
extern int 	yyparse();

static __dead void usage(void);
static bool 	inaddreq(struct in_addr, struct in_addr);
static bool 	httpget(CURL *, const char *);
static struct sockaddr_in *find_sa(struct ifaddrs *, const char *);

int
main(int argc, char *argv[])
{
	int opt, error;
	bool optd, optn;
	char *optf;

	optn = false;
	optd = false;
	optf = _PATH_DYNDNSD_CONF;

	while (-1 != (opt = getopt(argc, argv, "hdnvf:"))) {
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
	if (NULL == yyin)
		err(1, "fopen(\"%s\")", optf);

	struct ast *ast;
	error = yyparse(&ast);
	fclose(yyin);

	if (!valid_ast(ast) || optn)
		exit(error);

	/* initialize libcurl */
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();
	if (NULL == curl)
		err(1, "curl_easy_init(3): failed to initialize libcurl");

	if (!optd)
		daemon(0, 0);

	openlog(NULL, (optd ? LOG_PERROR : 0) | LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "starting dyndnsd-%s...", VERSION);

	struct ifaddrs *ifap_old, *ifap_new;
	getifaddrs(&ifap_old);

	while (true) {
		if (-1 == getifaddrs(&ifap_new)) {
			syslog(LOG_ERR, "getifaddrs(3): %m");
			goto sleep;
		}

		for (struct ast_iface *aif = ast->interfaces; aif->next; aif = aif->next) {
			char *ipaddr;
			struct sockaddr_in *sa_old, *sa_new;
			sa_old = find_sa(ifap_old, aif->name);
			sa_new = find_sa(ifap_new, aif->name);

			if (!inaddreq(sa_old->sin_addr, sa_new->sin_addr))
				continue;

			ipaddr = inet_ntoa(sa_new->sin_addr);

			struct param *p;
			p = getparams(aif->url);
			p = setparam(p, "myip", ipaddr);
			p = setparam(p, "hostname", aif->domains->name);
			char *url = mkurl(aif->url, p);

			if (httpget(curl, url)) {
				long status;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
				syslog(LOG_INFO, "%s %s %s %ld", aif->name, ipaddr, url, status);
			}

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

static __dead void
usage(void)
{
	extern char *__progname;
	fprintf(stderr, "usage: %s [-dhnv] [-f file]\n", __progname);
	exit(0);
}

static bool
inaddreq(struct in_addr a, struct in_addr b)
{
	return memcmp(&a, &b, sizeof(struct in_addr)) == 0;
}

static bool
httpget(CURL *curl, const char *url)
{
	curl_easy_setopt(curl, CURLOPT_URL, url);
	CURLcode err = curl_easy_perform(curl);
	if (err) {
		syslog(LOG_ERR, "curl_easy_perform(3): %s", curl_easy_strerror(err));
		return false;
	}
	return true;
}

static struct sockaddr_in *
find_sa(struct ifaddrs *ifa, const char *ifname)
{
	while (ifa) {
		if (0 == strcmp(ifa->ifa_name, ifname))
			return (struct sockaddr_in *)ifa->ifa_addr;
		ifa = ifa->ifa_next;
	}
	return NULL;
}
