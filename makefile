CFLAGS += -I. -Isrc

all: dyndnsd

lex.yy.c: src/lex.l src/ast.h
	${LEX} ${LFLAGS} src/lex.l
y.tab.c y.tab.h: src/parse.y src/ast.h src/cst.h
	${YACC} ${YFLAGS} src/parse.y
lex.yy.o: lex.yy.c y.tab.h
y.tab.o: y.tab.c y.tab.h
die.o: src/die.h src/die.c
ast.o: src/ast.h src/ast.c
cst.o: src/cst.h src/cst.c y.tab.h
dyndnsd.o: src/dyndnsd.h src/dyndnsd.c

dyndnsd: dyndnsd.o ast.o cst.o die.o y.tab.o lex.yy.o
	${CC} ${LDFLAGS} -o $@ dyndnsd.o ast.o cst.o die.o y.tab.o lex.yy.o

.PHONY: clean

clean:
	rm -f dyndnsd *.o y.tab.* lex.yy.c
