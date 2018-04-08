MAIN_SRCS  = dyndnsd.c ast.c die.c process.c rtm.c parser.y lexer.l
TEST_SRCS  = test.c die.c
MAIN_OBJS := ${MAIN_SRCS:.c=.o:.y=.o:.l=.o}
TEST_OBJS := ${TEST_SRCS:.c=.o}
CFLAGS   +!= pkg-config --cflags cmocka
LDFLAGS  +!= pkg-config --libs cmocka

dyndnsd: ${MAIN_OBJS}
	${CC} ${LDFLAGS} -o $@ ${MAIN_OBJS}

dyndnsd-test: ${TEST_OBJS}
	${CC} ${LDFLAGS} -o $@ ${TEST_OBJS}

check! dyndnsd-test
	./dyndnsd-test

.PHONY: clean

clean:
	rm -f dyndnsd dyndnsd-test *.o y.tab.h
