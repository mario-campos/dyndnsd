CFLAGS   +!= pkg-config --cflags cmocka
LDFLAGS  +!= pkg-config --libs cmocka

all: dyndnsd

parser.o: src/parser.y
	${YACC} -d -o parser.c src/parser.y
	${CC} ${CFLAGS} -c -o $@ parser.c
	rm -f parser.c

lexer.o: src/lexer.l src/parser.y
	${LEX} -o lexer.c src/lexer.l
	${CC} ${CFLAGS} -c -o $@ lexer.c
	rm -f lexer.c

rtm.o: src/rtm.h src/rtm.c
	${CC} ${CFLAGS} -c -o $@ src/rtm.c

process.o: src/process.h src/process.c
	${CC} ${CFLAGS} -c -o $@ src/process.c

die.o: src/die.h src/die.c
	${CC} ${CFLAGS} -c -o $@ src/die.c

ast.o: src/ast.h src/ast.c
	${CC} ${CFLAGS} -c -o $@ src/ast.c

dyndnsd.o: src/dyndnsd.h src/dyndnsd.c
	${CC} ${CFLAGS} -c -o $@ src/dyndnsd.c

dyndnsd: ast.o die.o process.o rtm.o parser.o lexer.o
	${CC} ${LDFLAGS} -o $@ dyndnsd.o ast.o die.o process.o rtm.o parser.o lexer.o

test.o: test/test.c
	${CC} ${CFLAGS} -c -o $@ test/test.c

dyndnsd-test: test.o die.o
	${CC} ${LDFLAGS} -o $@ test.o die.o

check! dyndnsd-test
	./dyndnsd-test

.PHONY: clean

clean:
	rm -f *.o dyndnsd dyndnsd-test y.tab.h
