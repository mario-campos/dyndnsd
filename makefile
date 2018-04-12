CFLAGS   +!= pkg-config --cflags cmocka
LDFLAGS  +!= pkg-config --libs cmocka

build/parser.o: src/parser.y
	${YACC} -d src/parser.y

build/lexer.o: src/lexer.l src/parser.y
	${LEX} -o $@ src/lexer.l

build/rtm.o: src/rtm.h src/rtm.c
	${CC} ${CFLAGS} -o $@ src/rtm.c

build/process.o: src/process.h src/process.c
	${CC} ${CFLAGS} -o $@ src/process.c

build/die.o: src/die.h src/die.c
	${CC} ${CFLAGS} -o $@ src/die.c

build/ast.o: src/ast.h src/ast.c
	${CC} ${CFLAGS} -o $@ src/ast.c

build/dyndnsd.o: src/dyndnsd.h src/dyndnsd.c src/ast.h src/ast.c src/die.h src/die.c src/process.h src/process.c src/rtm.h src/rtm.c src/parser.y src/lexer.l
	${CC} ${CFLAGS} -o $@ src/dyndnsd.c src/ast.c src/die.c src/process.c src/rtm.c src/parser.c src/lexer.c

build/dyndnsd: build/dyndnsd.o build/ast.o build/die.o build/process.o build/rtm.o build/parser.o build/lexer.o
	${CC} ${LDFLAGS} -o $@ build/dyndnsd.o build/ast.o build/die.o build/process.o build/rtm.o build/parser.o build/lexer.o

build/dyndnsd-test: build/test.o build/die.o
	${CC} ${LDFLAGS} -o $@ build/test.o build/die.o

check! build/dyndnsd-test
	build/dyndnsd-test

.PHONY: clean

clean:
	rm -rf build y.tab.h
