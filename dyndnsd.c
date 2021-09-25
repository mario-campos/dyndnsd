#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include <assert.h>
#include <err.h>
#include <event.h>
#include <fcntl.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>

#include "dyndnsd.h"
#include "parser.h"
#include "rtm.h"

inline static void _free(char **);
static void usage(void);
static void handle_sigterm(int, short, void *);
static void handle_sigchld(int, short, void *);
static void process_event(int, short, void *);
static void drop_privilege(char *, char *);
static pid_t spawn(const char *, int, char *, char *, char *);

int
main(int argc, char *argv[])
{
	int 		opt;
	unsigned 	opts;
	char           *filename;
	struct event	ev_route, ev_signal;
	struct dyndnsd	this;

	opts = 0;
	filename = DYNDNSD_CONF_PATH;

	openlog(getprogname(), LOG_PERROR | LOG_PID, LOG_DAEMON);

	/* allocate route(4) socket before pledge(2) */
	this.routefd = rtm_socket();
	if (-1 == this.routefd)
		errx(EX_UNAVAILABLE, "cannot create route(4) socket");

#ifdef __OpenBSD__
	if (-1 == pledge("stdio rpath proc exec id getpw inet", NULL))
		err(EX_OSERR, "pledge");
#endif

	while (-1 != (opt = getopt(argc, argv, "hdnvf:"))) {
		switch (opt) {
		case 'd':
			opts |= DYNDNSD_DEBUG_MODE;
			break;
		case 'h':
			usage();
			exit(EX_OK);
		case 'f':
			filename = optarg;
			break;
		case 'n':
			opts |= DYNDNSD_VALID_MODE;
			break;
		case 'v':
			puts(DYNDNSD_VERSION);
			exit(EX_OK);
		default:
			usage();
			exit(EX_USAGE);
		}
	}

	if (-1 == (this.devnull = open(_PATH_DEVNULL, O_WRONLY|O_CLOEXEC)))
		err(EX_OSFILE, "open");

	if (NULL == (this.ast = parse(filename)))
		errx(EX_DATAERR, "%s\n", parser_error);

	if (opts & DYNDNSD_VALID_MODE)
		exit(EX_OK);

	if (0 == getuid())
		drop_privilege(DYNDNSD_USER, DYNDNSD_GROUP);

	if (!(opts & DYNDNSD_DEBUG_MODE) && -1 == daemon(0, 0))
		err(EX_OSERR, "daemon");

#ifdef __OpenBSD__
	if (-1 == pledge("stdio proc exec", NULL)) {
		syslog(LOG_ERR, "pledge: %m");
		exit(EX_OSERR);
	}
	syslog(LOG_DEBUG, "pledge: stdio proc exec");
#endif

	event_init();

	event_set(&ev_route, this.routefd, EV_READ|EV_PERSIST, process_event, &this);
	event_add(&ev_route, NULL);

	signal_set(&ev_signal, SIGTERM, handle_sigterm, &this);
	signal_set(&ev_signal, SIGCHLD, handle_sigchld, NULL);
	signal_add(&ev_signal, NULL);

	event_dispatch();
}

inline static void
_free(char **p)
{
	free(*p);
}

static void
process_event(int sig, short event, void *arg)
{
	pid_t pid;
	struct ast_if *aif;
	struct ast_dn *adn;
	struct rtm_newaddr rtm;
	struct dyndnsd *this = arg;

	if (-1 == rtm_consume(this->routefd, &rtm))
		return;

	SLIST_FOREACH(aif, &this->ast->aif_head, next) {
		aif = SLIST_FIRST(&this->ast->aif_head);
		if (strcmp(rtm.rtm_ifname, aif->name))
			continue;
		SLIST_FOREACH(adn, &aif->adn_head, next) {
			pid = spawn(this->ast->run, this->devnull, adn->name, inet_ntoa(rtm.rtm_ifaddr), aif->name);
			syslog(LOG_INFO, "%s %s %s %u", aif->name, inet_ntoa(rtm.rtm_ifaddr), adn->name, pid);
		}
	}
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-dhnv] [-f file]\n", getprogname());
}

static void __dead
handle_sigterm(int sig, short event, void *arg)
{
	assert(arg);

	struct dyndnsd *this = arg;

	close(this->devnull);
	close(this->routefd);
	closelog();
	exit(EX_OK);
}

static void
handle_sigchld(int sig, short event, void *arg)
{
	wait(NULL);
}

static void
drop_privilege(char *username, char *groupname)
{
	assert(username);
	assert(groupname);

	struct passwd *newuser;
	struct group *newgroup;

	if (NULL == (newgroup = getgrnam(groupname)))
		err(EX_NOUSER, "getgrnam");
	if (-1 == setgid(newgroup->gr_gid))
		err(EX_OSERR, "setgid");
	if (-1 == setgroups(1, &newgroup->gr_gid))
		err(EX_OSERR, "setgroups");
	if (NULL == (newuser = getpwnam(username)))
		err(EX_NOUSER, "getpwnam");
	if (-1 == setuid(newuser->pw_uid))
		err(EX_OSERR, "setuid");
}

static pid_t
spawn(const char *cmd, int fd, char *domain, char *ipaddr, char *iface)
{
	assert(cmd);
	assert(fd >= 0);
	assert(domain);
	assert(ipaddr);
	assert(iface);

	pid_t pid;
	char *envvar0 __attribute__((cleanup(_free)));
	char *envvar1 __attribute__((cleanup(_free)));
	char *envvar2 __attribute__((cleanup(_free)));

	if (-1 == asprintf(&envvar0, "DYNDNSD_DOMAIN=%s", domain) ||
	    -1 == asprintf(&envvar1, "DYNDNSD_IPADDR=%s", ipaddr) ||
	    -1 == asprintf(&envvar2, "DYNDNSD_INTERFACE=%s", iface)) {
		syslog(LOG_DEBUG, "asprintf: %m");
		return -1;
	}

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
		execle(_PATH_BSHELL, _PATH_BSHELL, "-c", cmd, NULL, (char*[]){envvar0, envvar1, envvar2, NULL});
	}

	return pid;
}
