%{
#include <stdbool.h>
#include <stdlib.h>

#include "ast.h"
#include "y.tab.h"

void yyerror(const char *);
%}

%option nounput
%option noyywrap
%option yylineno

newline			\n+
comment			#.*
whitespace		[ \t]+
quoted_string		\"([^"\\]|\\.)*\"
unquoted_string		[^ \n\t]+
lineescape		\\.*\n

%%

interface		return INTERFACE;
domain			return DOMAIN;
run			return RUN;
"{"			return '{';
"}"			return '}';
{newline}		return '\n';
{comment}		/* ignore */ ;
{whitespace}		/* ignore */ ;
{lineescape}		/* ignore */ ;
{quoted_string}		{ yylval.string = strdup(yytext); return QUOTE; }
{unquoted_string}	{ yylval.string = strdup(yytext); return STRING; }

%%

void
yyerror(const char *msg)
{
	fprintf(stderr, "%s: %s (line %d)\n", getprogname(), msg, yylineno);
}
