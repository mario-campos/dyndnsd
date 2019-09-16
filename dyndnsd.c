#include <arpa/inet.h>
#include <net/if.h>

#include <assert.h>
#include <err.h>
#include <event.h>
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
#include "dyndnsd.h"
#include "rtm.h"

#define DYNDNSD_VERSION "0.1.0"
#define DYNDNSD_DEBUG_MODE 0x0001
#define DYNDNSD_VALID_MODE 0x0002

extern FILE *yyin;
extern int   yyparse();

static void __dead usage(void);
static void sig_handler(int, short, void *);
static void process_event(int, short, void *);
static void drop_privilege(char *, char *);
static pid_t spawn(char *, int, char *, char *, char *);

char *filename;
struct ast_root *ast;

struct dyndnsd {
	FILE   *etcfstream;
	int	routefd;
	int	etcfd;
	int 	devnull;
};

int
main(int argc, char *argv[])
{
	int 		opt;
	unsigned 	opts;
	struct event	ev_route, ev_signal;
	struct dyndnsd	this;

	ast = NULL;
	opts = 0;
	filename = DYNDNSD_CONF_PATH;

	openlog(getprogname(), LOG_PERROR | LOG_PID, LOG_DAEMON);

	/* allocate route(4) socket before pledge(2) */
	this.routefd = rtm_socket();
	if (-1 == this.routefd)
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

	this.devnull = open(_PATH_DEVNULL, O_WRONLY|O_CLOEXEC);
	if (-1 == this.devnull)
		err(1, "open");

	this.etcfd = open(filename, O_RDONLY|O_CLOEXEC);
	if (-1 == this.etcfd)
		err(1, "open");

	this.etcfstream = fdopen(this.etcfd, "r");
	if (NULL == this.etcfstream)
		err(1, "fdopen");

	yyin = this.etcfstream;
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

	if (-1 == sigaction(SIGCHLD, &(struct sigaction){SIG_IGN, 0, SA_NOCLDWAIT}, NULL)) {
		syslog(LOG_WARNING, "sigaction: %m");
		exit(1);
	}

	event_init();

	event_set(&ev_route, this.routefd, EV_READ|EV_PERSIST, process_event, &this);
	event_add(&ev_route, NULL);

	signal_set(&ev_signal, SIGTERM, sig_handler, &this);
	signal_add(&ev_signal, NULL);

	event_dispatch();
}

static void
process_event(int sig, short event, void *arg)
{
	pid_t pid;
	struct ast_iface *aif;
	struct rtm_newaddr rtm;
	struct dyndnsd *this = arg;

	if (-1 == rtm_consume(this->routefd, &rtm))
		return;

	for (size_t j = 0; j < ast->iface_len; j++) {
		aif = ast->iface[j];
		if (strcmp(rtm.rtm_ifname, aif->if_name))
			continue;
		for (size_t k = 0; k < aif->domain_len; k++) {
			pid = spawn(ast->cmd, this->devnull, aif->domain[k], inet_ntoa(rtm.rtm_ifaddr), aif->if_name);
			syslog(LOG_INFO, "%s %s %s %u", aif->if_name, inet_ntoa(rtm.rtm_ifaddr), aif->domain[k], pid);
		}
	}
}

static void __dead
usage(void)
{
	fprintf(stderr, "usage: %s [-dhnv] [-f file]\n", getprogname());
	exit(0);
}

static void __dead
sig_handler(int sig, short event, void *arg)
{
	assert(arg);

	struct dyndnsd *this = arg;

	fclose(this->etcfstream);
	close(this->devnull);
	close(this->routefd);
	closelog();
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
