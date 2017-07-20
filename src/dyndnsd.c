#include <sys/queue.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/route.h>

#include <assert.h>
#include <err.h>
#include <ifaddrs.h>
#include <signal.h>
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

static __dead void usage(void);
static bool 	httpget(CURL *, const char *);
static void 	strsub(char *, size_t, const char *, const char *);
static void 	parse_fqdn(const char *, const char **, const char **, const char **);
static size_t 	httplog(char *, size_t, size_t, void *);

int
main(int argc, char *argv[])
{
	FILE 	       *conf;
	ssize_t 	numread;
	bool 		optd, optn, valid_conf;
	int 		opt, routefd, kq;
	unsigned int 	rtfilter;
	const char     *optf, *hostname, *domain, *tld, *ifname, *ipaddr;
	char 		rtmbuf[RTM_MEM_LIMIT], urlbuf[URL_MEM_LIMIT], logbuf[LOG_MEM_LIMIT];
	CURL 	       *curl;
	struct ast     *ast, *ast_tmp;
	struct ast_iface *aif;
	struct ast_domain *ad;
	struct kevent changes[2];
	struct kevent events[2];

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

	conf = fopen(optf, "r");
	if (NULL == conf)
		err(1, "fopen(\"%s\")", optf);

	valid_conf = ast_load(&ast, conf);
	if (!valid_conf || optn)
		exit(valid_conf ? 0 : 1);

	kq = kqueue();
	if (-1 == kq)
		err(1, "kqueue(2)");

	/* initialize libcurl */
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (NULL == curl)
		err(1, "curl_easy_init(3): failed to initialize libcurl");
	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httplog))
		err(1, "curl_easy_setopt(CURLOPT_WRITEFUNCTION)");
	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, logbuf))
		err(1, "curl_easy_setopt(CURLOPT_WRITEDATA)");

	/* set up route(4) socket */
	routefd = socket(PF_ROUTE, SOCK_RAW, AF_INET);
	if (-1 == routefd)
		err(1, "socket(2)");

	rtfilter = ROUTE_FILTER(RTM_NEWADDR);
	if (-1 == setsockopt(routefd, PF_ROUTE, ROUTE_MSGFILTER,
			     &rtfilter, sizeof(rtfilter)))
		err(1, "setsockopt(2)");

	if (!optd)
		if (-1 == daemon(0, 0))
			err(1, "daemon(3)");

	openlog(__progname, (optd ? LOG_PERROR : 0) | LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "starting dyndnsd-%s", VERSION);

	signal(SIGHUP, SIG_IGN);
	EV_SET(&changes[0], SIGHUP, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);
	EV_SET(&changes[1], routefd, EVFILT_READ, EV_ADD, 0, 0, NULL);

	while (true) {
		int nev;

		nev = kevent(kq, changes, 2, NULL, 0, NULL);
		if (-1 == nev) {
			syslog(LOG_WARNING, "kevent(2): failed to set events: %m");
			continue;
		}

		nev = kevent(kq, NULL, 0, events, 2, NULL);
		if (-1 == nev) {
			syslog(LOG_WARNING, "kevent(2): failed to get events: %m");
			continue;
		}

		assert(nev != 0);

		for (int i = 0; i < nev; i++) {
			if (events[i].ident == SIGHUP) {
				syslog(LOG_INFO, "SIGHUP received; reloading configuration");
				rewind(conf);
				if (ast_load(&ast_tmp, conf)) {
					ast_free(ast);
					ast = ast_tmp;
				} else
					syslog(LOG_ERR, "invalid configuration: unable to load");
			} else {
				numread = read(routefd, rtmbuf, sizeof(rtmbuf));
				if (-1 == numread)
					break;

				ifname = rtm_getifname((struct rt_msghdr *)rtmbuf);
				ipaddr = rtm_getipaddr((struct ifa_msghdr *)rtmbuf);

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
		}
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
parse_fqdn(const char *fqdn, const char **hostname, const char **domain, const char **tld)
{
	const char *i, *j;

	/* parse TLD */
	for (j = fqdn + strlen(fqdn); '.' != *j; j--);
	*tld = strdup(j + 1);

	/* parse 2-label FQDN */
	for (i = fqdn; '.' != *i; i++);
	if (i == j) {
		*hostname = strdup("@");
		*domain = strdup(fqdn);
		return;
	}

	/* parse FQDNs with more than 3 labels */
	*domain = strdup(i + 1);
	*hostname = strndup(fqdn, i - fqdn);
}

static size_t
httplog(char *response, size_t size, size_t nmemb, void *userptr)
{
	char *log;
	size_t realsize, copysize;

	log = (char *)userptr;
	realsize = size * nmemb;
	copysize = realsize < (LOG_MEM_LIMIT - 1) ? realsize : (LOG_MEM_LIMIT - 1);

	memcpy(log, response, copysize);
	log[copysize] = '\0';

	return realsize;
}
