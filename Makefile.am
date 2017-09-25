bin_PROGRAMS = dyndnsd
dyndnsd_SOURCES = src/dyndnsd.c src/parser.y src/lexer.l
dist_man_MANS = man/dyndnsd.8 man/dyndnsd.conf.5
MOSTLYCLEANFILES = parser.c parser.h lexer.c
EXTRA_DIST = LICENSE README.md
BUILT_SOURCES = parser.h
AM_FLAGS = -Wall
AM_YFLAGS = -d
