#ifndef DYNDNSD_H
#define DYNDNSD_H

#define DYNDNSD_VERSION "0.0.0"
#define DYNDNSD_DEBUG_MODE (1 << 1)
#define DYNDNSD_VALID_MODE (1 << 2)

extern FILE *yyin;
extern int   yyparse();

#endif /* DYNDNSD_H */
