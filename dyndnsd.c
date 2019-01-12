#include <sys/types.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <assert.h>
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
#include <syslog.h>
#include <unistd.h>

#include "ast.h"
#include "rtm.h"

#define DYNDNSD_DEBUG_MODE 0x0001
#define DYNDNSD_VALID_MODE 0x0002

extern FILE *yyin;
extern int   yyparse();

static void __dead usage(void);
static void drop_privilege(char *, char *);
static pid_t spawn(char *, int, char *, char *, char *);

char *filename;
struct ast_root *ast;

int
main(int argc, char *argv[])
{
	FILE		*etcfstream;
	int 		 opt, routefd, kq, etcfd, devnull, nev;
	unsigned	 opts;
	struct kevent    changes[2];
	struct kevent    events[2];

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

	EV_SET(&changes[0], SIGTERM, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);
	EV_SET(&changes[1], routefd, EVFILT_READ, EV_ADD, 0, 0, NULL);

	while (true) {
		nev = kevent(kq, changes, 2, events, 2, NULL);
		if (-1 == nev) {
			syslog(LOG_CRIT, "kevent: %m");
			exit(1);
		}

		for (ssize_t i = 0; i < nev; i++) {

			/* SIGTERM event */
			if (SIGTERM == events[i].ident) {
				syslog(LOG_NOTICE, "SIGTERM received. Terminating...");
				goto end;
			}

			/* RTM_NEWADDR event */
			else {
				struct ast_iface *aif;
				struct rtm_newaddr rtm;

				if (-1 == rtm_consume(routefd, &rtm))
					break;

				for (size_t j = 0; j < ast->iface_len; j++) {
					aif = ast->iface[j];
					if (strcmp(rtm.rtm_ifname, aif->if_name))
						continue;
					for (size_t k = 0; k < aif->domain_len; k++)
						spawn(ast->cmd, devnull, aif->domain[k], inet_ntoa(rtm.rtm_ifaddr), aif->if_name);
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
	assert(username);
	assert(groupname);

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

static pid_t
spawn(char *cmd, int fd, char *domain, char *ipaddr, char *iface)
{
	assert(cmd);
	assert(fd >= 0);
	assert(domain);
	assert(ipaddr);
	assert(iface);

	pid_t pid;
	char *shell;

	shell = getenv("SHELL") ? getenv("SHELL") : _PATH_BSHELL;

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
		setenv("DYNDNSD_DOMAIN", domain, true);
		setenv("DYNDNSD_IPADDR", ipaddr, true);
		setenv("DYNDNSD_INTERFACE", iface, true);
		execl(shell, shell, "-c", cmd, NULL);
	}

	return pid;
}
