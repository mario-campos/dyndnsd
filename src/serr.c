#include <stdarg.h>
#include <stdlib.h>
#include <syslog.h>

#include "serr.h"

__dead void
serr(int exitcode, int priority, const char *msg)
{
	syslog(priority, "%s: %m", msg);
	exit(exitcode);
}

__dead void
serrx(int exitcode, int priority, const char *msg)
{
	syslog(priority, "%s", msg);
	exit(exitcode);
}
