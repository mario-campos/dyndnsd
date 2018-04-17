CFLAGS   += -I. -Isrc
CFLAGS  +!= pkg-config --cflags cmocka
LDFLAGS +!= pkg-config --libs cmocka

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
test.o: test/test.c
dyndnsd.o: src/dyndnsd.h src/dyndnsd.c

dyndnsd: dyndnsd.o ast.o cst.o die.o process.o rtm.o y.tab.o lex.yy.o
	${CC} ${LDFLAGS} -o $@ dyndnsd.o ast.o cst.o die.o process.o rtm.o y.tab.o lex.yy.o
dyndnsd-test: test.o die.o cst.o y.tab.o lex.yy.o
	${CC} ${LDFLAGS} -o $@ test.o die.o cst.o y.tab.o lex.yy.o

check! dyndnsd-test
	./dyndnsd-test

.PHONY: clean

clean:
	rm -f dyndnsd dyndnsd-test *.o y.tab.* lex.yy.c
