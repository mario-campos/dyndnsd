#include <stdarg.h>
#include <stdlib.h>
#include <syslog.h>

#include "die.h"

__dead void
die(int priority, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vsyslog(priority, fmt, argp);
	va_end(argp);
	exit(EXIT_FAILURE);
}
