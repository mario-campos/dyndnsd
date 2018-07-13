%{
#include <sys/socket.h>

#include <net/if.h>
#include <ifaddrs.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "cst.h"

extern char *filename;
extern bool error;
extern int yylex();
extern void yyerror(const char *, ...);

static struct ifaddrs *getif(struct ifaddrs *, const char *);

int nrun = 0;
%}

%union {
	int		 lineno;
	char            *string;
	struct cst_node *cst_node;
}

%token <lineno> INTERFACE DOMAIN RUN
%token <string> STRING QUOTE LUNQUOTE RUNQUOTE
%type <cst_node> config config_statements config_statement
%type <cst_node> interface interface_statements interface_statement
%type <cst_node> domain run exprs expr value '\n'

%%

config : config_statements {
		struct cst_node *cst;
		if (error) return 1;
		cst = cst_node(0, NULL, NULL, $1);
		ast = cst_convert(cst);
		cst_free(cst);
	}
	;

config_statements
	: config_statement
	| config_statements config_statement {
		$$ = $2; SLIST_NEXT($2, next) = $1;
	}
	;

config_statement
	: '\n' {
		$$ = cst_node('\n', NULL, NULL, NULL);
	}
	| run
	| interface
	;

interface
	: INTERFACE value '{' interface_statements '}' {
		char *ifname;
		struct ifaddrs *ifas, *ifa;

		ifa = ifas = NULL;

		if (QUOTE == $2->type)
			ifname = dequote($2->string);
		else
			ifname = $2->string;

		if (0 == getifaddrs(&ifas)) {
			if (!(ifa = getif(ifas, ifname)))
				fprintf(stderr, "%s:%d: warning: interface not found: %s\n", filename, $1, ifname);
			if (ifa && !(ifa->ifa_flags & IFF_UP))
				fprintf(stderr, "%s:%d: warning: interface is down: %s\n", filename, $1, ifname);
			if (ifa && !(ifa->ifa_addr->sa_family & AF_INET))
				fprintf(stderr, "%s:%d: warning: interface does not support IPv4: %s\n", filename, $1, ifname);
			freeifaddrs(ifas);
		}

		if (QUOTE == $2->type) free(ifname);
		$$ = cst_node(INTERFACE, NULL, $2, $4);
	}
	| INTERFACE LUNQUOTE '{' interface_statements '}' {
		yyerror("%s:%d: error: no opening quote: %s", filename, $1, $2);
		$$ = cst_node(INTERFACE, NULL, NULL, $4);
	}
	| INTERFACE RUNQUOTE '{' interface_statements '}' {
		yyerror("%s:%d: error: no closing quote: %s", filename, $1, $2);
		$$ = cst_node(INTERFACE, NULL, NULL, $4);
	}
	| error '}'
	;

interface_statements
	: interface_statement
	| interface_statements interface_statement {
		$$ = $2; SLIST_NEXT($2, next) = $1;
	}
	;

interface_statement
	: '\n' {
		$$ = cst_node('\n', NULL, NULL, NULL);
	}
	| domain
	;

domain	: DOMAIN value {
		$$ = cst_node(DOMAIN, NULL, $2, NULL);
	}
	| DOMAIN LUNQUOTE {
		yyerror("%s:%d: error: no opening quote: %s", filename, $1, $2);
		$$ = cst_node(DOMAIN, NULL, NULL, NULL);
	}
	| DOMAIN RUNQUOTE {
		yyerror("%s:%d: error: no closing quote: %s", filename, $1, $2);
		$$ = cst_node(DOMAIN, NULL, NULL, NULL);
	}
	| error '\n'
	;

run	: RUN exprs {
		if (++nrun > 1)
			yyerror("%s:%d: error: redundant statement", filename, $1);
		$$ = cst_node(RUN, NULL, $2, NULL);
	}
	;

exprs	: expr
	| exprs expr {
		for($$ = $1; SLIST_NEXT($$, next); $$ = SLIST_NEXT($$, next));
			SLIST_NEXT($$, next) = $2;
		$$ = $1;
	}
	;

value	: STRING {
      		$$ = cst_node(STRING, $1, NULL, NULL);
	}
	| QUOTE	{
		$$ = cst_node(QUOTE, $1, NULL, NULL);
	}
	;

expr	: STRING {
     		$$ = cst_node(STRING, $1, NULL, NULL);
	}
	| QUOTE	{
		$$ = cst_node(QUOTE, $1, NULL, NULL);
	}
	| LUNQUOTE {
		$$ = cst_node(LUNQUOTE, $1, NULL, NULL);
	}
	| RUNQUOTE {
		$$ = cst_node(RUNQUOTE, $1, NULL, NULL);
	}
	;

%%

static struct ifaddrs *
getif(struct ifaddrs *ifa, const char *ifname)
{
	for (; ifa->ifa_next; ifa = ifa->ifa_next) {
		if (0 == strcmp(ifa->ifa_name, ifname))
			return ifa;
	}
	return NULL;
}
