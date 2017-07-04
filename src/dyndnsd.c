#include <sys/queue.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

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

extern char    *__progname;
extern FILE    *yyin;
extern int 	yyparse();

#define ROUNDUP(a) \
		((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

static __dead void usage(void);
static bool 	httpget(CURL *, const char *);
char	       *strsub(const char *, const char *, const char *);
struct sockaddr *get_sa(char *, int);

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

	if (!ast_is_valid(ast) || optn)
		exit(error);

	/* initialize libcurl */
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();
	if (NULL == curl)
		err(1, "curl_easy_init(3): failed to initialize libcurl");

	/* set up route(4) socket */
	int routefd = socket(PF_ROUTE, SOCK_RAW, AF_INET);
	if (-1 == routefd)
		err(1, "socket(2)");

	unsigned int rtfilter = ROUTE_FILTER(RTM_NEWADDR);
	if (-1 == setsockopt(routefd, PF_ROUTE, ROUTE_MSGFILTER,
			     &rtfilter, sizeof(rtfilter)))
		err(1, "setsockopt(2)");

	if (!optd)
		daemon(0, 0);

	openlog(__progname, (optd ? LOG_PERROR : 0) | LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "starting dyndnsd-%s...", VERSION);

	while (true) {
		char buf[2048];

		ssize_t numread = read(routefd, buf, sizeof(buf));
		if (-1 == numread)
			err(1, "read(2)");

		struct rt_msghdr *rtm = (struct rt_msghdr *)buf;

		assert(numread <= rtm->rtm_msglen);
		assert(rtm->rtm_version == RTM_VERSION);
		assert(rtm->rtm_type == RTM_NEWADDR);

		struct ifa_msghdr *ifam = (struct ifa_msghdr *)rtm;
		char ifname[IF_NAMESIZE];
		if_indextoname(ifam->ifam_index, ifname);

		struct sockaddr *sa = get_sa((char *)ifam + ifam->ifam_hdrlen, ifam->ifam_addrs);
		struct sockaddr_in *sain = (struct sockaddr_in *)sa;

		assert(sain->sin_family == AF_INET);

		struct in_addr a;
		memcpy(&a, &sain->sin_addr, sizeof(a));

		assert(a.s_addr != INADDR_ANY);

		char *ipaddr = inet_ntoa(a);

		struct ast_iface *aif;
		SLIST_FOREACH(aif, ast->interfaces, next) {
			if (0 != strcmp(aif->name, ifname))
				continue;

			struct ast_domain *ad;
			SLIST_FOREACH(ad, aif->domains, next) {
				char *url1 = strsub(aif->url, "$domain", ad->name);
				char *url2 = strsub(url1, "$ip_address", ipaddr);

				if (httpget(curl, url2)) {
					long status;
					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
					syslog(LOG_INFO, "%s %s %s %ld", aif->name, ipaddr, url2, status);
				}

				free(url1);
				free(url2);
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

char *
strsub(const char *src, const char *search, const char *replace)
{
	char *end;
	char buf[URL_MEM_LIMIT] = {0};

	while ((end = strstr(src, search))) {
		/* copy the substring before the search string */
		strncat(buf, src, end - src);
		src = end;

		/* insert the replacement string */
		strlcat(buf, replace, sizeof(buf));
		src += strlen(search);
	}

	strlcat(buf, src, sizeof(buf));

	return strdup(buf);
}

struct sockaddr *
get_sa(char *cp, int flags) {
	int i;
	struct sockaddr *sa;
	if (0 == flags)
		return NULL;
	for (i = 1; i; i <<= 1) {
		if (flags & i) {
			sa = (struct sockaddr *)cp;
			if (i == RTA_IFA)
				return sa;
			ADVANCE(cp, sa);
		}
	}
	return NULL;
}
