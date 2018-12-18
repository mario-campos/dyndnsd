DYNDNSD_USER  = nobody
DYNDNSD_GROUP = nobody
CFLAGS += -g

dyndnsd: dyndnsd.o cst.o rtm.o y.tab.o lex.yy.o
	${CC} ${LDFLAGS} -o $@ dyndnsd.o cst.o rtm.o y.tab.o lex.yy.o

dyndnsd.o: dyndnsd.h dyndnsd.c
	${CC} -c -DDYNDNSD_USER=\"${DYNDNSD_USER}\" -DDYNDNSD_GROUP=\"${DYNDNSD_GROUP}\" ${CFLAGS} dyndnsd.c

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

.PHONY: clean
clean:
	rm -f dyndnsd *.o y.tab.* lex.yy.c
