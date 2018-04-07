MAIN_SRCS  = dyndnsd.c ast.c die.c process.c rtm.c parser.y lexer.l
TEST_SRCS  = test.c die.c
MAIN_OBJS := ${MAIN_SRCS:.c=.o:.y=.o:.l=.o}
TEST_OBJS := ${TEST_SRCS:.c=.o}
CFLAGS   +!= pkg-config --cflags cmocka
LDFLAGS  +!= pkg-config --libs cmocka

dyndnsd: ${MAIN_OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${MAIN_OBJS}

check: ${TEST_OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${TEST_SRCS}
	./$@

.PHONY: clean

clean:
	rm -f dyndnsd check *.o y.tab.h
