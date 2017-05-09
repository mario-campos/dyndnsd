bin_PROGRAMS = dyndnsd
dyndnsd_SOURCES = src/dyndnsd.c src/parse.y src/tokenize.l
dist_man_MANS = man/dyndnsd.8 man/dyndnsd.conf.5
MOSTLYCLEANFILES = src/parse.c src/parse.h src/tokenize.c
EXTRA_DIST = LICENSE README.md
BUILT_SOURCES = src/parse.h
AM_FLAGS = -Wall
AM_YFLAGS = -d
