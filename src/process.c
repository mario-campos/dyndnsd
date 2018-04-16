#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "die.h"
#include "process.h"

static void parse_fqdn(char *, char **, char **, char **);

pid_t
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

char *
getshell(void)
{
	return getenv("SHELL") ?: "/bin/sh";
}

void
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
