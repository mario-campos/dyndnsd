bin_PROGRAMS = dyndnsd
dyndnsd_SOURCES = src/dyndnsd.c src/ast.c src/parser.y src/lexer.l
dyndnsd_CFLAGS = -pipe -Wall -Wextra -Wconversion -Wsign-conversion \
		 -Wformat -Wformat-security -Wl,-z,relro -Wl,-z,now \
		 -fstack-protector-all -fpie -pie $(CURL_CFLAGS)
dyndnsd_LDADD = $(CURL_LIBS)
BUILT_SOURCES = src/parser.h
AM_YFLAGS = -d
dist_man_MANS = man/dyndnsd.8 man/dyndnsd.conf.5
MOSTLYCLEANFILES = src/parser.c src/parser.h src/lexer.c
EXTRA_DIST = LICENSE README.md
