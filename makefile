dyndnsd: src/ast.o src/die.o src/dyndnsd.o y.tab.o lex.yy.o
	$(CC) $(CFLAGS) -o dyndnsd ast.o die.o dyndnsd.o y.tab.o lex.yy.o

y.tab.h: src/parser.y
	$(YACC) -d src/parser.y

y.tab.o: src/parser.y src/ast.h
	$(YACC) -d src/parser.y
	$(CC) $(CFLAGS) -I src -o y.tab.o -c y.tab.c
	rm -f y.tab.c

lex.yy.o: src/lexer.l src/ast.h
	$(LEX) src/lexer.l
	$(CC) $(CFLAGS) -I src -o lex.yy.o -c lex.yy.c
	rm -f lex.yy.c

.PHONY: clean

clean:
	rm -f dyndnsd *.o y.tab.h
