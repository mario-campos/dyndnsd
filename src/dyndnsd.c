#include <sys/types.h>
#include <sys/event.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <assert.h>
#include <grp.h>
#include <ifaddrs.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>

#include "ast.h"
#include "config.h"
#include "die.h"

#define LOG_MEM_LIMIT 1024
#define RTM_MEM_LIMIT 1024
#define URL_MEM_LIMIT 1024
#define ROUNDUP(a) \
	    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

extern char    *__progname;

static int	rtm_socket(unsigned int);
static char    *rtm_getifname(struct rt_msghdr *);
static char    *rtm_getipaddr(struct ifa_msghdr *);
static struct sockaddr *rtm_getsa(uint8_t *, int);

static __dead void usage(void);
static bool 	httpget(CURL *, const char *);
static void 	strsub(char *, size_t, const char *, const char *);
static void 	parse_fqdn(const char *, const char **, const char **, const char **);
static size_t 	httplog(char *, size_t, size_t, void *);
static void	drop_privilege(char *, char *);

int
main(int argc, char *argv[])
{
	FILE 	        *conf;
	bool 		 optd, optn;
	int 		 opt, routefd, kq;
	const char      *optf;
	char 		 logbuf[LOG_MEM_LIMIT];
	CURL 	        *curl;
	struct ast_root *ast;
	struct kevent    changes[3];
	struct kevent    events[3];

	ast = NULL;
	optn = false;
	optd = false;
	optf = DYNDNSD_CONF_PATH;

	openlog(__progname, LOG_PERROR | LOG_PID, LOG_DAEMON);

	/* allocate route(4) socket first... */
	routefd = rtm_socket(RTM_NEWADDR);
	if (-1 == routefd)
		die(LOG_ERR, "cannot create route(4) socket");

	/* ...to pledge(2) ASAP */
	if (-1 == pledge("stdio rpath inet dns proc id getpw", NULL))
		die(LOG_ERR, "pledge(2): %m");

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
		die(LOG_ERR, "cannot open file '%s': fopen(3): %m", optf);

	if (!ast_load(&ast, conf))
		die(LOG_ERR, "cannot parse configuration file");

	if (optn)
		exit(0);

	if (0 == getuid())
		drop_privilege(ast->user ?: DYNDNSD_USER, ast->group ?: DYNDNSD_GROUP);

	/* initialize libcurl */
	curl_global_init((long)CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (NULL == curl)
		die(LOG_CRIT, AT("curl_easy_init(3): failed to initialize libcurl"));
	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httplog))
		die(LOG_CRIT, AT("curl_easy_setopt(3): %m"));
	if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, logbuf))
		die(LOG_CRIT, AT("curl_easy_setopt(3): %m"));

	if (!optd)
	if (-1 == daemon(0, 0))
		die(LOG_CRIT, AT("daemon(3): %m"));

	if (-1 == pledge("stdio rpath inet dns", NULL))
		die(LOG_ERR, "pledge(2): %m");

	/* set up event handler */
	signal(SIGHUP, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	kq = kqueue();
	if (-1 == kq)
		die(LOG_CRIT, AT("kqueue(2): %m"));

	EV_SET(&changes[0], SIGHUP, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);
	EV_SET(&changes[1], routefd, EVFILT_READ, EV_ADD, 0, 0, NULL);
	EV_SET(&changes[2], SIGTERM, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);

	syslog(LOG_INFO, "starting dyndnsd-%s", VERSION);

	while (true) {
		int nev;

		nev = kevent(kq, changes, 3, NULL, 0, NULL);
		if (-1 == nev)
			die(LOG_CRIT, AT("kevent(2): %m"));

		nev = kevent(kq, NULL, 0, events, 3, NULL);
		if (-1 == nev)
			die(LOG_CRIT, AT("kevent(2): %m"));

		assert(nev != 0);

		for (int i = 0; i < nev; i++) {

			/* SIGTERM event */
			if (events[i].ident == SIGTERM) {
				syslog(LOG_NOTICE, "SIGTERM received. Terminating...");
				goto end;
			}

			/* SIGHUP event */
			else if (events[i].ident == SIGHUP) {
				syslog(LOG_NOTICE, "SIGHUP received. Reloading the configuration file...");
				rewind(conf);
				ast_load(&ast, conf);
			}

			/* RTM_NEWADDR event */
			else {
				ssize_t numread;
				char *ifname;
				const char *ipaddr, *hostname, *domain, *tld;
				char rtmbuf[RTM_MEM_LIMIT];
				char urlbuf[URL_MEM_LIMIT];

				numread = read(routefd, rtmbuf, sizeof(rtmbuf));
				if (-1 == numread)
					break;

				ifname = rtm_getifname((struct rt_msghdr *)rtmbuf);
				ipaddr = rtm_getipaddr((struct ifa_msghdr *)rtmbuf);

				for(size_t i = 0; i < ast->iface_len; i++) {
					struct ast_iface *aif = ast->iface[i];

					if (0 != strcmp(aif->if_name, ifname))
						continue;

					for(size_t j = 0; j < aif->domain_len; j++) {
						struct ast_domain *ad = aif->domain[j];

						strlcpy(urlbuf, ad->url, sizeof(urlbuf));
						parse_fqdn(ad->domain, &hostname, &domain, &tld);
						strsub(urlbuf, sizeof(urlbuf), "${FQDN}", ad->domain);
						strsub(urlbuf, sizeof(urlbuf), "${HOSTNAME}", hostname);
						strsub(urlbuf, sizeof(urlbuf), "${DOMAIN}", domain);
						strsub(urlbuf, sizeof(urlbuf), "${TLD}", tld);
						strsub(urlbuf, sizeof(urlbuf), "${IPv4_ADDRESS}", ipaddr);

						if (httpget(curl, urlbuf))
							syslog(LOG_INFO, "%s %s %s %s", ifname, ad->domain, ipaddr, logbuf);
					}
				}

				free(ifname);
			}
		}
	}

end:
	curl_easy_cleanup(curl);
	close(routefd);
	closelog();
}

static int
rtm_socket(unsigned int flags)
{
	int routefd;
	unsigned int rtfilter;

	routefd = socket(PF_ROUTE, SOCK_RAW, AF_INET);
	if (-1 == routefd)
		return -1;

	rtfilter = ROUTE_FILTER(flags);
	if (-1 == setsockopt(routefd, PF_ROUTE, ROUTE_MSGFILTER,
			     &rtfilter, (socklen_t)sizeof(rtfilter)))
		return -1;

	return routefd;
}

static char *
rtm_getifname(struct rt_msghdr *rtm)
{
	struct ifa_msghdr *ifam = (struct ifa_msghdr *)rtm;
	char *buf = malloc((size_t)IF_NAMESIZE);
	return if_indextoname(ifam->ifam_index, buf);
}

static char *
rtm_getipaddr(struct ifa_msghdr *ifam)
{
	struct in_addr addr;
	struct sockaddr *sa;
	struct sockaddr_in *sin;

	sa = rtm_getsa((uint8_t *)ifam + ifam->ifam_hdrlen, ifam->ifam_addrs);
	sin = (struct sockaddr_in *)sa;

	memcpy(&addr, &sin->sin_addr, sizeof(addr));

	return inet_ntoa(addr);
}

static struct sockaddr *
rtm_getsa(uint8_t *cp, int flags)
{
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
		syslog(LOG_ERR, AT("curl_easy_perform(3): %s"), curl_easy_strerror(err));
		return false;
	}
	return true;
}

static void
strsub(char *scope, size_t len, const char *search, const char *replace)
{
	size_t n;
	char *start, *end;
	char buf[len];

	start = scope;
	explicit_bzero(buf, len);

	while ((end = strstr(start, search))) {
		n = end - start;

		/* copy the substring before the search string */
		strncat(buf, start, n);
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
	size_t n;
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
	n = i - fqdn;
	*domain = strdup(i + 1);
	*hostname = strndup(fqdn, n);
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

static void
drop_privilege(char *username, char *groupname)
{
	assert(username);
	assert(groupname);

	struct passwd *newuser;
	struct group *newgroup;

	if (NULL == (newgroup = getgrnam(groupname)))
		die(LOG_ERR, "cannot set GID: getgrnam(3): %m");
	if (-1 == setgid(newgroup->gr_gid))
		die(LOG_ERR, "cannot set GID: setgid(2): %m");
	if (-1 == setgroups(1, &newgroup->gr_gid))
		die(LOG_ERR, "cannot set groups: setgroups(2): %m");
	if (NULL == (newuser = getpwnam(username)))
		die(LOG_ERR, "cannot set UID: getpwnam(3): %m");
	if (-1 == setuid(newuser->pw_uid))
		die(LOG_ERR, "cannot set UID: setuid(2): %m");
}
