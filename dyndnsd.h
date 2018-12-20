#ifndef DYNDNSD_H
#define DYNDNSD_H

#define DYNDNSD_DEBUG_MODE 0x0001
#define DYNDNSD_VALID_MODE 0x0002

extern FILE *yyin;
extern int   yyparse();

#endif /* DYNDNSD_H */
