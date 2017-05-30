%{
#include <string.h>
#include "parser.h"
%}

%option nounput
%option noyywrap

comment			"#"[^\n]*
whitespace		[ \t\n]+
quoted_string		"\""([^"\\]|\\.)*"\""
unquoted_string		[^ \n\t]+

%%

"interface"		return INTERFACE;
"domain"		return DOMAIN;
"http-get" 		return HTTP_GET;
"{"			return '{';
"}"			return '}';
{comment}		;
{whitespace}		;
{quoted_string}		{ char *s = strdup(yytext); s[yyleng-1] = '\0'; s++; yylval.str = s; return STRING; }
{unquoted_string}	{ yylval.str = strdup(yytext); return STRING; }

%%

void yyerror(struct ast **ast, const char *s) {
    fprintf(stderr, "%s\n", s);
}

