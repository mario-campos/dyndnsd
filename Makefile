PROG = dyndnsd
SRCS = dyndnsd.c rtm.c parser.c
MAN  = dyndnsd.8 dyndnsd.conf.5

CFLAGS += -std=c99
LDFLAGS += -levent

.include <bsd.prog.mk>
