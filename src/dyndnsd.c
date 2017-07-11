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
static void 	parse_fqdn(const char *, char **, char **, char **);
static size_t	httplog(char *, size_t, size_t, void *);

int
main(int argc, char *argv[])
{
	int opt;
	bool optd, optn;
	char *optf, *hostname, *domain, *tld;
	char rtmbuf[READ_MEM_LIMIT], urlbuf[URL_MEM_LIMIT], logbuf[LOG_MEM_LIMIT];

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

	int error = yyparse(&ast);
	fclose(yyin);

	if (!ast_is_valid(ast) || optn)
		exit(error);

	/* initialize libcurl */
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();
	if (NULL == curl)
		err(1, "curl_easy_init(3): failed to initialize libcurl");
	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httplog))
		err(1, "curl_easy_setopt(CURLOPT_WRITEFUNCTION)");
	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, logbuf))
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
		ssize_t numread = read(routefd, rtmbuf, sizeof(rtmbuf));
		if (-1 == numread)
			err(1, "read(2)");

		const char *ifname = rtm_getifname((struct rt_msghdr *)rtmbuf);
		const char *ipaddr = rtm_getipaddr((struct ifa_msghdr *)rtmbuf);

		SLIST_FOREACH(aif, ast->interfaces, next) {
			if (0 != strcmp(aif->name, ifname))
				continue;
			SLIST_FOREACH(ad, aif->domains, next) {
				strlcpy(urlbuf, ad->url ?: aif->url ?: ast->url, sizeof(urlbuf));
				parse_fqdn(ad->name, &hostname, &domain, &tld);
				strsub(urlbuf, sizeof(urlbuf), "$fqdn", ad->name);
				strsub(urlbuf, sizeof(urlbuf), "$hostname", hostname);
				strsub(urlbuf, sizeof(urlbuf), "$domain", domain);
				strsub(urlbuf, sizeof(urlbuf), "$tld", tld);
				strsub(urlbuf, sizeof(urlbuf), "$ip_address", ipaddr);

				if (httpget(curl, urlbuf))
					syslog(LOG_INFO, "%s %s %s %s", ifname, ad->name, ipaddr, logbuf);
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

static void
parse_fqdn(const char *fqdn, char **hostname, char **domain, char **tld)
{
	const char *i, *j;

	/* parse TLD */
	for (j = fqdn + strlen(fqdn); '.' != *j; j--);
	*tld = strdup(j+1);

	/* parse 2-label FQDN */
	for (i = fqdn; '.' != *i; i++);
	if (i == j) {
		*hostname = strdup("@");
		*domain = strdup(fqdn);
		return;
	}

	/* parse FQDNs with more than 3 labels */
	*domain = strdup(i+1);
	*hostname = malloc(i-fqdn+1);
	strlcpy(*hostname, fqdn, i-fqdn+1);
}

static size_t
httplog(char *response, size_t size, size_t nmemb, void *userptr)
{
	char *log;
	size_t realsize, copysize;

	log = (char *)userptr;
	realsize = size * nmemb;
	copysize = realsize < (LOG_MEM_LIMIT-1) ? realsize : (LOG_MEM_LIMIT-1);

	memcpy(log, response, copysize);
	log[copysize] = '\0';

	return realsize;
}
