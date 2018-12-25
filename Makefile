DYNDNSD_VERSION = 0.1.0
DIST_FILES	= LICENSE \
		  README.md \
		  Makefile \
		  parse.y \
		  dyndnsd.8 \
		  dyndnsd.conf.5 \
		  parse.y \
		  lex.l \
		  rtm.c \
		  rtm.h \
		  cst.c \
		  cst.h \
		  ast.h \
		  dyndnsd.c

all: dyndnsd

dyndnsd: dyndnsd.o cst.o rtm.o y.tab.o lex.yy.o
	${CC} ${LDFLAGS} -o $@ dyndnsd.o cst.o rtm.o y.tab.o lex.yy.o

dyndnsd.o: dyndnsd.c rtm.h ast.h
	${CC} -c ${CFLAGS} \
		-DDYNDNSD_VERSION=\"${DYNDNSD_VERSION}\" \
		-DDYNDNSD_USER=\"${DYNDNSD_USER}\" \
		-DDYNDNSD_GROUP=\"${DYNDNSD_GROUP}\" \
		-DDYNDNSD_CONF_PATH=\"${DYNDNSD_CONF_PATH}\" \
		dyndnsd.c

lex.yy.o: lex.yy.c y.tab.h
	${CC} -c ${CFLAGS} lex.yy.c

lex.yy.c: lex.l
	${LEX} ${LFLAGS} lex.l

y.tab.o: y.tab.c y.tab.h
	${CC} -c ${CFLAGS} y.tab.c

y.tab.c y.tab.h: parse.y cst.h
	${YACC} ${YFLAGS} parse.y

cst.o: cst.h cst.c y.tab.h
	${CC} -c ${CFLAGS} cst.c

rtm.o: rtm.h rtm.c
	${CC} -c ${CFLAGS} rtm.c

.PHONY: install uninstall dist clean

install: dyndnsd
	install -D dyndnsd ${DESTDIR}${PREFIX}/bin/dyndnsd
	install -D -m 0644 dyndnsd.8 ${DESTDIR}${PREFIX}/man/man8/dyndnsd.8
	install -D -m 0644 dyndnsd.conf.5 ${DESTDIR}${PREFIX}/man/man5/dyndnsd.conf.5

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dyndnsd

dist:
	mkdir dyndnsd-${DYNDNSD_VERSION}
	cp ${DIST_FILES} dyndnsd-${DYNDNSD_VERSION}/
	tar czNf dyndnsd-${DYNDNSD_VERSION}.tar.gz dyndnsd-${DYNDNSD_VERSION}/
	tar cjNf dyndnsd-${DYNDNSD_VERSION}.tar.bz2 dyndnsd-${DYNDNSD_VERSION}/
	rm -rf dyndnsd-${DYNDNSD_VERSION}

clean:
	rm -f dyndnsd *.o y.tab.* lex.yy.c
