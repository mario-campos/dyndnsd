#include <sys/types.h>
#include <sys/event.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <err.h>
#include <fcntl.h>
#include <grp.h>
#include <ifaddrs.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "ast.h"
#include "dyndnsd.h"

#define ROUNDUP(a) \
	    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

static void __dead usage(void);
static void drop_privilege(char *, char *);
static int rtm_socket();
static char *rtm_getifname(void*);
static char *rtm_getipaddr(void*);
static struct sockaddr *rtm_getsa(uint8_t*, int);
static pid_t spawn(char *, int);
static void set_dyndnsd_env(char *, char *, char *);
static char *getshell(void);
static void parse_fqdn(char *, char **, char **, char **);
static struct ast_iface *find_iface(struct ast_root *, char *);

char *filename;
struct ast_root *ast;

int
main(int argc, char *argv[])
{
	FILE		*etcfstream;
	int 		 opt, routefd, kq, etcfd, devnull;
	unsigned	 opts;
	struct ast_root *ast;
	struct kevent    changes[3];
	struct kevent    events[3];

	ast = NULL;
	opts = 0;
	filename = DYNDNSD_CONF_PATH;

	openlog(getprogname(), LOG_PERROR | LOG_PID, LOG_DAEMON);

	/* allocate route(4) socket before pledge(2) */
	routefd = rtm_socket();
	if (-1 == routefd)
		errx(1, "cannot create route(4) socket");

	if (-1 == pledge("stdio rpath proc exec id getpw inet", NULL))
		err(1, "pledge");

	while (-1 != (opt = getopt(argc, argv, "hdnvf:"))) {
		switch (opt) {
		case 'd':
			opts |= DYNDNSD_DEBUG_MODE;
			break;
		case 'h':
			usage();
		case 'f':
			filename = optarg;
			break;
		case 'n':
			opts |= DYNDNSD_VALID_MODE;
			break;
		case 'v':
			puts(DYNDNSD_VERSION);
			exit(0);
		default:
			usage();
		}
	}

	devnull = open(_PATH_DEVNULL, O_WRONLY|O_CLOEXEC);
	if (-1 == devnull)
		err(1, "open");

	etcfd = open(filename, O_RDONLY|O_CLOEXEC);
	if (-1 == etcfd)
		err(1, "open");

	etcfstream = fdopen(etcfd, "r");
	if (NULL == etcfstream)
		err(1, "fdopen");

	yyin = etcfstream;
	if (1 == yyparse())
		exit(1);

	if (opts & DYNDNSD_VALID_MODE)
		exit(0);

	if (0 == getuid())
		drop_privilege(DYNDNSD_USER, DYNDNSD_GROUP);

	if (!(opts & DYNDNSD_DEBUG_MODE) && -1 == daemon(0, 0))
		err(1, "daemon");

	if (-1 == pledge("stdio proc exec", NULL)) {
		syslog(LOG_ERR, "pledge: %m");
		exit(1);
	}

	/* set up event handler */
	struct sigaction sa = { .sa_handler = SIG_IGN, 0, 0 };
	if (-1 == sigaction(SIGTERM, &sa, NULL)	||
	    -1 == sigaction(SIGCHLD, &sa, NULL))  { // implying SA_NOCLDWAIT
		syslog(LOG_CRIT, "sigaction: %m");
		exit(1);
	}

	kq = kqueue();
	if (-1 == kq) {
		syslog(LOG_CRIT, "kqueue: %m");
		exit(1);
	}

	EV_SET(&changes[0], SIGHUP, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);
	EV_SET(&changes[1], routefd, EVFILT_READ, EV_ADD, 0, 0, NULL);
	EV_SET(&changes[2], SIGTERM, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);

	syslog(LOG_INFO, "starting dyndnsd-%s", DYNDNSD_VERSION);

	while (true) {
		int nev;

		if (-1 == kevent(kq, changes, 3, NULL, 0, NULL) ||
		    -1 == (nev = kevent(kq, NULL, 0, events, 3, NULL))) {
			syslog(LOG_CRIT, "kevent: %m");
			exit(1);
		}

		for (ssize_t i = 0; i < nev; i++) {

			/* SIGTERM event */
			if (events[i].ident == SIGTERM) {
				syslog(LOG_NOTICE, "SIGTERM received. Terminating...");
				goto end;
			}

			/* RTM_NEWADDR event */
			else {
				ssize_t numread;
				char *ifname, *ipaddr;
				char rtmbuf[1024];
				struct ast_iface *aif;

				numread = read(routefd, rtmbuf, sizeof(rtmbuf));
				if (-1 == numread)
					break;

				ifname = rtm_getifname(rtmbuf);
				ipaddr = rtm_getipaddr(rtmbuf);

				aif = find_iface(ast, ifname);
				if (NULL == aif)
					continue;

				free(ifname);

				for(size_t j = 0; j < aif->domain_len; j++) {
					pid_t pid;

					set_dyndnsd_env(aif->domain[j], ipaddr, aif->if_name);

					pid = spawn(ast->cmd, devnull);
					if (-1 == pid)
						syslog(LOG_ERR, "cannot run command: %m");
					else
						syslog(LOG_INFO, "%s %s %s %d", aif->domain[j], aif->if_name, ipaddr, pid);
				}
			}
		}
	}

end:
	fclose(etcfstream);
	close(devnull);
	close(routefd);
	closelog();
}

static void __dead
usage(void)
{
	fprintf(stderr, "usage: %s [-dhnv] [-f file]\n", getprogname());
	exit(0);
}

static void
drop_privilege(char *username, char *groupname)
{
	struct passwd *newuser;
	struct group *newgroup;

	if (NULL == (newgroup = getgrnam(groupname)))
		err(1, "getgrnam");
	if (-1 == setgid(newgroup->gr_gid))
		err(1, "setgid");
	if (-1 == setgroups(1, &newgroup->gr_gid))
		err(1, "setgroups");
	if (NULL == (newuser = getpwnam(username)))
		err(1, "getpwnam");
	if (-1 == setuid(newuser->pw_uid))
		err(1, "setuid");
}

static int
rtm_socket()
{
	int routefd;
	unsigned int rtfilter;

	routefd = socket(PF_ROUTE, SOCK_CLOEXEC|SOCK_RAW, AF_INET);
	if (-1 == routefd)
		return -1;

	rtfilter = ROUTE_FILTER(RTM_NEWADDR);
	if (-1 == setsockopt(routefd, PF_ROUTE, ROUTE_MSGFILTER,
			     &rtfilter, (socklen_t)sizeof(rtfilter)))
		return -1;

	return routefd;
}

static char *
rtm_getifname(void *ptr)
{
	struct ifa_msghdr *ifam = ptr;
	char *buf = malloc((size_t)IF_NAMESIZE);
	return if_indextoname(ifam->ifam_index, buf);
}

static char *
rtm_getipaddr(void *ptr)
{
	struct in_addr addr;
	struct sockaddr *sa;
	struct sockaddr_in *sin;
	struct ifa_msghdr *ifam;

	ifam = ptr;
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

static pid_t
spawn(char *cmd, int fd)
{
	pid_t pid;

	pid = fork();
	if (-1 == pid) {
		syslog(LOG_DEBUG, "fork: %m");
		return -1;
	}

	if (0 == pid) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		setsid();
		execl(getshell(), getshell(), "-c", cmd, NULL);
	}

	return pid;
}

static char *
getshell(void)
{
	return getenv("SHELL") ?: "/bin/sh";
}

static void
set_dyndnsd_env(char *fqdn, char *ipaddr, char *iface)
{
	char *hostname, *domain, *tld;
	parse_fqdn(fqdn, &hostname, &domain, &tld);

	if (-1 == setenv("DYNDNSD_FQDN", fqdn, true))
		syslog(LOG_WARNING, "cannot set DYNDNSD_FQDN: setenv(3): %m");
	if (-1 == setenv("DYNDNSD_HOSTNAME", hostname, true))
		syslog(LOG_WARNING, "cannot set DYNDNSD_HOSTNAME: setenv(3): %m");
	if (-1 == setenv("DYNDNSD_DOMAIN", domain, true))
		syslog(LOG_WARNING, "cannot set DYNDNSD_DOMAIN: setenv(3): %m");
	if (-1 == setenv("DYNDNSD_TLD", tld, true))
		syslog(LOG_WARNING, "cannot set DYNDNSD_TLD: setenv(3): %m");
	if (-1 == setenv("DYNDNSD_IPADDR", ipaddr, true))
		syslog(LOG_WARNING, "cannot set DYNDNSD_IPADDR: setenv(3): %m");
	if (-1 == setenv("DYNDNSD_INTERFACE", iface, true))
		syslog(LOG_WARNING, "cannot set DYNDNSD_INTERFACE: setenv(3): %m");

	free(hostname);
	free(domain);
	free(tld);
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

static struct ast_iface *
find_iface(struct ast_root *ast, char *ifname)
{
	for(size_t i = 0; i < ast->iface_len; i++) {
		struct ast_iface *aif = ast->iface[i];
		if (0 == strcmp(ifname, aif->if_name))
			return aif;
	}
	return NULL;
}
