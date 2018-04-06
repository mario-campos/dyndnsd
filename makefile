SOURCES  = dyndnsd.c ast.c die.c parser.y lexer.l
OBJECTS := $(SOURCES:.c=.o:.y=.o:.l=.o)

dyndnsd: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

.PHONY: clean

clean:
	rm -f dyndnsd *.o y.tab.h
