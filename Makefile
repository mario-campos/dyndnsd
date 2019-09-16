PROG = dyndnsd
SRCS = dyndnsd.c cst.c rtm.c parse.y lex.l
MAN  = dyndnsd.8 dyndnsd.conf.5

CPPFLAGS += -DDYNDNSD_CONF_PATH=\"$(CONF_PATH)\"
CPPFLAGS += -DDYNDNSD_USER=\"$(USER)\"
CPPFLAGS += -DDYNDNSD_GROUP=\"$(GROUP)\"

LDFLAGS += -levent

.include <bsd.prog.mk>
