DYNDNSD_USER      = nobody
DYNDNSD_GROUP     = nobody
DYNDNSD_CONF_PATH = /etc/dyndnsd.conf
CPPFLAGS += -DDYNDNSD_USER=\"${DYNDNSD_USER}\" -DDYNDNSD_GROUP=\"${DYNDNSD_GROUP}\" -DDYNDNSD_CONF_PATH=\"${DYNDNSD_CONF_PATH}\"
CFLAGS   += -I. -Isrc
CMOCKA_CFLAGS  +!= pkg-config --cflags cmocka
CMOCKA_LDFLAGS +!= pkg-config --libs cmocka


all: dyndnsd

lex.yy.c: src/lex.l src/ast.h
	${LEX} ${LFLAGS} src/lex.l
y.tab.c y.tab.h: src/parse.y src/ast.h src/cst.h
	${YACC} ${YFLAGS} src/parse.y
lex.yy.o: lex.yy.c y.tab.h
y.tab.o: y.tab.c y.tab.h
rtm.o: src/rtm.h src/rtm.c
process.o: src/process.h src/process.c
die.o: src/die.h src/die.c
ast.o: src/ast.h src/ast.c
cst.o: src/cst.h src/cst.c y.tab.h
dyndnsd.o: src/dyndnsd.h src/dyndnsd.c

dyndnsd: dyndnsd.o ast.o cst.o die.o process.o rtm.o y.tab.o lex.yy.o
	${CC} ${LDFLAGS} -o $@ dyndnsd.o ast.o cst.o die.o process.o rtm.o y.tab.o lex.yy.o

test.o: test/test.c
	${CC} ${CFLAGS} ${CMOCKA_CFLAGS} -c test/test.c
dyndnsd-test: test.o die.o cst.o y.tab.o lex.yy.o
	${CC} ${LDFLAGS} ${CMOCKA_LDFLAGS} -o $@ test.o die.o cst.o y.tab.o lex.yy.o

check! dyndnsd-test
	./dyndnsd-test

.PHONY: clean

clean:
	rm -f dyndnsd dyndnsd-test *.o y.tab.* lex.yy.c
