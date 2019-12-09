PROG = dyndnsd
SRCS = dyndnsd.c rtm.c parser.c
MAN  = dyndnsd.8 dyndnsd.conf.5

CFLAGS += -std=c99

CPPFLAGS += -DDYNDNSD_CONF_PATH=\"$(CONF_PATH)\"
CPPFLAGS += -DDYNDNSD_USER=\"$(USER)\"
CPPFLAGS += -DDYNDNSD_GROUP=\"$(GROUP)\"

LDFLAGS += -levent

.include <bsd.prog.mk>
