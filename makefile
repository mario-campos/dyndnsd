CFLAGS   +!= pkg-config --cflags cmocka
LDFLAGS  +!= pkg-config --libs cmocka

all: dyndnsd

lex.yy.c: src/lexer.l
	${LEX} ${LFLAGS} src/lexer.l

lex.yy.o: lex.yy.c y.tab.h
	${CC} ${CFLAGS} lex.yy.c
	
y.tab.c y.tab.h: src/parser.y
	${YACC} ${YFLAGS} src/parser.y

y.tab.o: y.tab.c y.tab.h
	${CC} ${CFLAGS} -c y.tab.c

rtm.o: src/rtm.h src/rtm.c
	${CC} ${CFLAGS} -c src/rtm.c

process.o: src/process.h src/process.c
	${CC} ${CFLAGS} -c src/process.c

die.o: src/die.h src/die.c
	${CC} ${CFLAGS} -c src/die.c

ast.o: src/ast.h src/ast.c
	${CC} ${CFLAGS} -c src/ast.c

dyndnsd.o: src/dyndnsd.h src/dyndnsd.c
	${CC} ${CFLAGS} -c src/dyndnsd.c

dyndnsd: ast.o die.o process.o rtm.o y.tab.o lex.yy.o
	${CC} ${LDFLAGS} -o $@ dyndnsd.o ast.o die.o process.o rtm.o y.tab..o lex.yy.o

test.o: test/test.c
	${CC} ${CFLAGS} -c test/test.c

dyndnsd-test: test.o die.o
	${CC} ${LDFLAGS} -o $@ test.o die.o

check! dyndnsd-test
	./dyndnsd-test

.PHONY: clean

clean:
	rm -f *.o dyndnsd dyndnsd-test y.tab.h
