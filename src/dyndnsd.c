#include <sys/types.h>
#include <sys/event.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <err.h>
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

#include "ast.h"
#include "config.h"
#include "die.h"

#define RTM_MEM_LIMIT 1024
#define RUN_BUF_LIMIT 1024
#define ROUNDUP(a) \
	    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

extern char    *__progname;

static int	rtm_socket(unsigned int);
static char    *rtm_getifname(struct rt_msghdr *);
static char    *rtm_getipaddr(struct ifa_msghdr *);
static struct sockaddr *rtm_getsa(uint8_t *, int);
static pid_t	spawn(char *);
static void __dead usage(void);
static void 	parse_fqdn(char *, char **, char **, char **);
static void	drop_privilege(char *, char *);
static void	set_dyndnsd_env(char *, char *);
static struct ast_iface *find_ast_iface(struct ast_root *, char *);
static char *	getshell(void);

int
main(int argc, char *argv[])
{
	FILE 	        *conf;
	bool 		 optd, optn;
	int 		 opt, routefd, kq;
	const char      *optf;
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
		errx(EXIT_FAILURE, "cannot create route(4) socket");

	/* ...to pledge(2) ASAP */
	if (-1 == pledge("stdio rpath proc exec id getpw", NULL))
		err(EXIT_FAILURE, "pledge(2)");

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
			exit(EXIT_SUCCESS);
		default:
			usage();
		}
	}

	conf = fopen(optf, "r");
	if (NULL == conf)
		err(EXIT_FAILURE, "cannot open file '%s': fopen(3)", optf);

	if (!ast_load(&ast, conf))
		errx(EXIT_FAILURE, "cannot parse configuration file");

	if (optn)
		exit(EXIT_SUCCESS);

	if (0 == getuid())
		drop_privilege(ast->user ?: DYNDNSD_USER, ast->group ?: DYNDNSD_GROUP);

	if (!optd && -1 == daemon(0, 0))
		err(EXIT_FAILURE, AT("daemon(3)"));

	if (-1 == pledge("stdio proc exec", NULL))
		die(LOG_ERR, "pledge(2): %m");

	/* set up event handler */
	struct sigaction sa = { .sa_handler = SIG_IGN, 0, 0 };
	if (-1 == sigaction(SIGHUP, &sa, NULL)	||
	    -1 == sigaction(SIGTERM, &sa, NULL)	||
	    -1 == sigaction(SIGCHLD, &sa, NULL))
		die(LOG_CRIT, AT("sigaction(2): %m"));

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

		for (ssize_t i = 0; i < nev; i++) {

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
				char *ifname, *ipaddr;
				char rtmbuf[RTM_MEM_LIMIT];
				struct ast_iface *aif;

				numread = read(routefd, rtmbuf, sizeof(rtmbuf));
				if (-1 == numread)
					break;

				ifname = rtm_getifname((struct rt_msghdr *)rtmbuf);
				ipaddr = rtm_getipaddr((struct ifa_msghdr *)rtmbuf);

				aif = find_ast_iface(ast, ifname);
				if (NULL == aif)
					continue;

				free(ifname);

				for(size_t j = 0; j < aif->domain_len; j++) {
					pid_t pid;
					struct ast_domain *ad;

					ad = aif->domain[j];
					set_dyndnsd_env(ad->domain, ipaddr);

					pid = spawn(ad->run);
					if (-1 == pid)
						syslog(LOG_ERR, "cannot run command: %m");
					else
						syslog(LOG_INFO, "%s %s %s %d", ad->domain, aif->if_name, ipaddr, pid);
				}
			}
		}
	}

end:
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

static void __dead
usage(void)
{
	fprintf(stderr, "usage: %s [-dhnv] [-f file]\n", __progname);
	exit(EXIT_SUCCESS);
}

static void
parse_fqdn(char *fqdn, char **hostname, char **domain, char **tld)
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

static void
drop_privilege(char *username, char *groupname)
{
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

static void
set_dyndnsd_env(char *fqdn, char *ipaddr)
{
	char *hostname, *domain, *tld;
	parse_fqdn(fqdn, &hostname, &domain, &tld);

	if (-1 == setenv("DYNDNSD_FQDN", fqdn, true) 		||
	    -1 == setenv("DYNDNSD_HOSTNAME", hostname, true) 	||
	    -1 == setenv("DYNDNSD_DOMAIN", domain, true) 	||
	    -1 == setenv("DYNDNSD_TLD", tld, true)		||
	    -1 == setenv("DYNDNSD_IPv4_ADDRESS", ipaddr, true))
		die(LOG_CRIT, AT("setenv(3): %m"));

	free(hostname);
	free(domain);
	free(tld);
}

static struct ast_iface *
find_ast_iface(struct ast_root *ast, char *ifname)
{
	for(size_t i = 0; i < ast->iface_len; i++) {
		struct ast_iface *aif = ast->iface[i];
		if (0 == strcmp(ifname, aif->if_name))
			return aif;
	}
	return NULL;
}

static char *
getshell(void)
{
	return getenv("SHELL") ?: "/bin/sh";
}

static pid_t
spawn(char *cmd)
{
	pid_t pid;

	pid = fork();
	if (-1 == pid) {
		syslog(LOG_DEBUG, AT("fork(2): %m"));
		return -1;
	}

	if (0 == pid &&
	   -1 == execl(getshell(), getshell(), "-c", cmd, NULL)) {
		syslog(LOG_DEBUG, AT("execl(3): %m"));
		return -1;
	}

	return pid;
}
