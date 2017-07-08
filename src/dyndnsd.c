#include <sys/queue.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/route.h>

#include <assert.h>
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
#include "limits.h"
#include "pathnames.h"
#include "rtm.h"

extern char    *__progname;
extern FILE    *yyin;
extern int 	yyparse();

static __dead void usage(void);
static bool 	httpget(CURL *, const char *);
static void	strsub(char *, size_t, const char *, const char *);

int
main(int argc, char *argv[])
{
	int opt;
	bool optd, optn;
	char *optf;
	char buf[READ_MEM_LIMIT];
	char url[URL_MEM_LIMIT];
	long http_status;

	struct ast *ast;
	struct ast_iface *aif;
	struct ast_domain *ad;

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

	FILE *devnull = fopen("/dev/null", "w+");
	if (NULL == devnull)
		err(1, "fopen(\"/dev/null\")");

	int error = yyparse(&ast);
	fclose(yyin);

	if (!ast_is_valid(ast) || optn)
		exit(error);

	/* initialize libcurl */
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();
	if (NULL == curl)
		err(1, "curl_easy_init(3): failed to initialize libcurl");
	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL))
		err(1, "curl_easy_setopt(CURLOPT_WRITEFUNCTION)");
	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, devnull))
		err(1, "curl_easy_setopt(CURLOPT_WRITEDATA)");

	/* set up route(4) socket */
	int routefd = socket(PF_ROUTE, SOCK_RAW, AF_INET);
	if (-1 == routefd)
		err(1, "socket(2)");

	unsigned int rtfilter = ROUTE_FILTER(RTM_NEWADDR);
	if (-1 == setsockopt(routefd, PF_ROUTE, ROUTE_MSGFILTER,
			     &rtfilter, sizeof(rtfilter)))
		err(1, "setsockopt(2)");

	openlog(__progname, (optd ? LOG_PERROR : 0) | LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "starting dyndnsd-%s", VERSION);

	if (!optd)
		daemon(0, 0);

	while (true) {
		ssize_t numread = read(routefd, buf, sizeof(buf));
		if (-1 == numread)
			err(1, "read(2)");

		const char *ifname = rtm_getifname((struct rt_msghdr *)buf);
		const char *ipaddr = rtm_getipaddr((struct ifa_msghdr *)buf);

		SLIST_FOREACH(aif, ast->interfaces, next) {
			if (0 != strcmp(aif->name, ifname))
				continue;
			SLIST_FOREACH(ad, aif->domains, next) {
				strlcpy(url, ad->url ?: aif->url ?: ast->url, sizeof(url));
				strsub(url, sizeof(url), "$domain", ad->name);
				strsub(url, sizeof(url), "$ip_address", ipaddr);

				if (httpget(curl, url)) {
					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
					syslog(LOG_INFO, "%s %s %s %ld", ifname, ipaddr, url, http_status);
				}
			}
		}

		free(ifname);
	}

	curl_easy_cleanup(curl);
}

static __dead void
usage(void)
{
	fprintf(stderr, "usage: %s [-dhnv] [-f file]\n", __progname);
	exit(0);
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

static void
strsub(char *scope, size_t len, const char *search, const char *replace)
{
	char *start, *end;
	char buf[len];

	start = scope;
	explicit_bzero(buf, len);

	while ((end = strstr(start, search))) {
		/* copy the substring before the search string */
		strncat(buf, start, end - start);
		start = end;

		/* insert the replacement string */
		strlcat(buf, replace, len);
		start += strlen(search);
	}

	strlcat(buf, start, len);
	memcpy(scope, buf, len);
}
