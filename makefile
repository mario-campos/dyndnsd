dyndnsd: ast.o die.o dyndnsd.o y.tab.o lex.yy.o
	$(CC) $(CFLAGS) -o dyndnsd ast.o die.o dyndnsd.o y.tab.o lex.yy.o

ast.o: src/ast.c src/ast.h y.tab.h
	$(CC) $(CFLAGS) -I . -I src -o ast.o -c src/ast.c

die.o: src/die.c src/die.h
	$(CC) $(CFLAGS) -I src -o die.o -c src/die.c

dyndnsd.o: src/dyndnsd.c src/dyndnsd.h
	$(CC) $(FLAGS) -I src -o dyndnsd.o -c src/dyndnsd.c

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
