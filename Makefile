DYNDNSD_USER  = nobody
DYNDNSD_GROUP = nobody
CPPFLAGS += -DDYNDNSD_USER=\"${DYNDNSD_USER}\" -DDYNDNSD_GROUP=\"${DYNDNSD_GROUP}\"
CFLAGS   += -I. -Isrc -g

dyndnsd: dyndnsd.o cst.o rtm.o y.tab.o lex.yy.o
	${CC} ${LDFLAGS} -o $@ dyndnsd.o cst.o rtm.o y.tab.o lex.yy.o

dyndnsd.o: src/dyndnsd.h src/dyndnsd.c
	${CC} -c ${CFLAGS} ${CPPFLAGS} src/dyndnsd.c

lex.yy.o: lex.yy.c y.tab.h
	${CC} -c ${CFLAGS} ${CPPFLAGS} lex.yy.c

lex.yy.c: src/lex.l
	${LEX} ${LFLAGS} src/lex.l

y.tab.o: y.tab.c y.tab.h
	${CC} -c ${CFLAGS} ${CPPFLAGS} y.tab.c

y.tab.c y.tab.h: src/parse.y src/cst.h
	${YACC} ${YFLAGS} src/parse.y

cst.o: src/cst.h src/cst.c y.tab.h
	${CC} -c ${CFLAGS} ${CPPFLAGS} src/cst.c

rtm.o: src/rtm.h src/rtm.c
	${CC} -c ${CFLAGS} ${CPPFLAGS} src/rtm.c

.PHONY: clean
clean:
	rm -f dyndnsd *.o y.tab.* lex.yy.c
