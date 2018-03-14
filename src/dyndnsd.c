#include <sys/types.h>
#include <sys/event.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/route.h>

#include <err.h>
#include <fcntl.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ast.h"
#include "config.h"
#include "die.h"
#include "rtm.h"

#include "dyndnsd.h"

static pid_t	spawn(char *, int);
static void __dead usage(void);
static void 	parse_fqdn(char *, char **, char **, char **);
static void	drop_privilege(char *, char *);
static void	set_dyndnsd_env(char *, char *);
static struct ast_iface *find_ast_iface(struct ast_root *, char *);
static char    *getshell(void);

int
main(int argc, char *argv[])
{
	FILE		*etcfstream;
	int 		 opt, routefd, kq, etcfd, devnull;
	unsigned	 opts;
	const char      *optf;
	struct ast_root *ast;
	struct kevent    changes[3];
	struct kevent    events[3];

	ast = NULL;
	opts = 0;
	optf = DYNDNSD_CONF_PATH;

	openlog(getprogname(), LOG_PERROR | LOG_PID, LOG_DAEMON);

	/* allocate route(4) socket first... */
	routefd = rtm_socket();
	if (-1 == routefd)
		errx(EXIT_FAILURE, "cannot create route(4) socket");

	/* ...to pledge(2) ASAP */
	if (-1 == pledge("stdio rpath proc exec id getpw", NULL))
		err(EXIT_FAILURE, "pledge(2)");

	while (-1 != (opt = getopt(argc, argv, "hdnvf:"))) {
		switch (opt) {
		case 'd':
			opts |= DYNDNSD_DEBUG_MODE;
			break;
		case 'h':
			usage();
		case 'f':
			optf = optarg;
			break;
		case 'n':
			opts |= DYNDNSD_VALID_MODE;
			break;
		case 'v':
			puts(VERSION);
			exit(EXIT_SUCCESS);
		default:
			usage();
		}
	}

	devnull = open(_PATH_DEVNULL, O_WRONLY|O_CLOEXEC);
	if (-1 == devnull)
		err(EXIT_FAILURE, "cannopt open file '/dev/null': open(2)");

	etcfd = open(optf, O_RDONLY|O_CLOEXEC);
	if (-1 == etcfd)
		err(EXIT_FAILURE, "cannot open file '%s': open(2)", optf);

	etcfstream = fdopen(etcfd, "r");
	if (NULL == etcfstream)
		err(EXIT_FAILURE, "cannot open file '%s': fdopen(3)", optf);

	if (!ast_load(&ast, etcfstream))
		errx(EXIT_FAILURE, "cannot parse configuration file");

	if (opts & DYNDNSD_VALID_MODE)
		exit(EXIT_SUCCESS);

	if (0 == getuid())
		drop_privilege(DYNDNSD_USER, DYNDNSD_GROUP);

	if (!(opts & DYNDNSD_DEBUG_MODE) && -1 == daemon(0, 0))
		err(EXIT_FAILURE, AT("daemon(3)"));

	if (-1 == pledge("stdio proc exec", NULL))
		die(LOG_ERR, "pledge(2): %m");

	/* set up event handler */
	struct sigaction sa = { .sa_handler = SIG_IGN, 0, 0 };
	if (-1 == sigaction(SIGHUP, &sa, NULL)	||
	    -1 == sigaction(SIGTERM, &sa, NULL)	||
	    -1 == sigaction(SIGCHLD, &sa, NULL))  // implying SA_NOCLDWAIT
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
				ast_load(&ast, etcfstream);
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

				ifname = rtm_getifname((struct rt_msghdr *)rtmbuf);
				ipaddr = rtm_getipaddr((struct ifa_msghdr *)rtmbuf);

				aif = find_ast_iface(ast, ifname);
				if (NULL == aif)
					continue;

				free(ifname);

				for(size_t j = 0; j < aif->domain_len; j++) {
					pid_t pid;

					set_dyndnsd_env(aif->domain[j], ipaddr);

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
	    -1 == setenv("DYNDNSD_IPADDR", ipaddr, true))
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
spawn(char *cmd, int fd)
{
	pid_t pid;

	pid = fork();
	if (-1 == pid) {
		syslog(LOG_DEBUG, AT("fork(2): %m"));
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
