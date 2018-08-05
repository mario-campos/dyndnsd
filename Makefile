DYNDNSD_USER  = nobody
DYNDNSD_GROUP = nobody
CPPFLAGS += -DDYNDNSD_USER=\"${DYNDNSD_USER}\" -DDYNDNSD_GROUP=\"${DYNDNSD_GROUP}\"
CFLAGS   += -I. -Isrc -g

all: dyndnsd

lex.yy.c: src/lex.l
	${LEX} ${LFLAGS} src/lex.l
y.tab.c y.tab.h: src/parse.y src/cst.h
	${YACC} ${YFLAGS} src/parse.y
lex.yy.o: lex.yy.c y.tab.h
y.tab.o: y.tab.c y.tab.h
cst.o: src/cst.h src/cst.c y.tab.h
dyndnsd.o: src/dyndnsd.h src/dyndnsd.c

dyndnsd: dyndnsd.o cst.o y.tab.o lex.yy.o
	${CC} ${LDFLAGS} -o $@ dyndnsd.o cst.o y.tab.o lex.yy.o

.PHONY: clean

clean:
	rm -f dyndnsd *.o y.tab.* lex.yy.c
