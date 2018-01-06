%{
#include <stdbool.h>

#include "ast.h"
#include "parser.h"

extern char *__progname;

void yyerror(__unused struct ast_root **, const char *);
%}

%option nounput
%option noyywrap
%option yylineno

newline			\n+
comment			#.*
whitespace		[ \t]+
unquoted_string		[^ \n\t]+

%%

interface		return INTERFACE;
domain			return DOMAIN;
run			return RUN;
"{"			return '{';
"}"			return '}';
{newline}		return '\n';
{comment}		/* ignore */ ;
{whitespace}		/* ignore */ ;
{unquoted_string}	{ yylval.string = strdup(yytext); return STRING; }

%%

void
yyerror(__unused struct ast_root **ast, const char *msg)
{
	fprintf(stderr, "%s: %s (line %d)\n", __progname, msg, yylineno);
}
