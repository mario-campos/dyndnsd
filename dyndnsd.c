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
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>

#include "ast.h"
#include "dyndnsd.h"
#include "rtm.h"

extern FILE *yyin;
extern int   yyparse();

static void usage(void);
static void sig_handler(int, short, void *);
static void process_event(int, short, void *);
static void drop_privilege(char *, char *);
static pid_t spawn(char *, int, char *, char *, char *);

char *filename;
struct ast_root *ast;

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
		errx(EX_UNAVAILABLE, "cannot create route(4) socket");

	if (-1 == pledge("stdio rpath proc exec id getpw inet", NULL))
		err(EX_OSERR, "pledge");

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

	this.devnull = open(_PATH_DEVNULL, O_WRONLY|O_CLOEXEC);
	if (-1 == this.devnull)
		err(EX_OSFILE, "open");

	this.etcfd = open(filename, O_RDONLY|O_CLOEXEC);
	if (-1 == this.etcfd)
		err(EX_OSFILE, "open");

	this.etcfstream = fdopen(this.etcfd, "r");
	if (NULL == this.etcfstream)
		err(EX_OSFILE, "fdopen");

	yyin = this.etcfstream;
	if (1 == yyparse())
		exit(EX_DATAERR);

	if (opts & DYNDNSD_VALID_MODE)
		exit(EX_OK);

	if (0 == getuid())
		drop_privilege(DYNDNSD_USER, DYNDNSD_GROUP);

	if (!(opts & DYNDNSD_DEBUG_MODE) && -1 == daemon(0, 0))
		err(EX_OSERR, "daemon");

	if (-1 == pledge("stdio proc exec", NULL)) {
		syslog(LOG_ERR, "pledge: %m");
		exit(EX_OSERR);
	}

	if (-1 == sigaction(SIGCHLD, &(struct sigaction){SIG_IGN, 0, SA_NOCLDWAIT}, NULL))
		syslog(LOG_WARNING, "sigaction: %m");

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

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-dhnv] [-f file]\n", getprogname());
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
	exit(EX_OK);
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
spawn(char *cmd, int fd, char *domain, char *ipaddr, char *iface)
{
	assert(cmd);
	assert(fd >= 0);
	assert(domain);
	assert(ipaddr);
	assert(iface);

	pid_t pid;
	char envvar0[2 + strlen("DYNDNSD_DOMAIN") + strlen(domain)]; // +2 bytes for '=', '\0'
	char envvar1[2 + strlen("DYNDNSD_IPADDR") + strlen(ipaddr)];
	char envvar2[2 + strlen("DYNDNSD_INTERFACE") + strlen(iface)];

	snprintf(envvar0, sizeof(envvar0), "DYNDNSD_DOMAIN=%s", domain);
	snprintf(envvar1, sizeof(envvar1), "DYNDNSD_IPADDR=%s", ipaddr);
	snprintf(envvar2, sizeof(envvar2), "DYNDNSD_INTERFACE=%s", iface);

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
