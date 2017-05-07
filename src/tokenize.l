%{
#include "parse.h"
%}

%option noyywrap

comment			"#"[^\n]*
whitespace		[ \t\n]+
quoted_string		"\""([^"\\]|\\.)*"\""
unquoted_string		[^ \n\t]+

%%

"interface"		return INTERFACE;
"domain"		return DOMAIN;
"http-url" 		return HTTP_URL;
"http-auth"		return HTTP_AUTH;
"http-header"		return HTTP_HEADER;
"{"			return '{';
"}"			return '}';
{comment}		;
{whitespace}		;
{quoted_string}		{ yytext[yyleng-1] = '\0'; yytext++; yyleng -= 2; return STRING; }
{unquoted_string}	return STRING;

%%

void yyerror(const char *s) {
    fprintf(stderr, "%s\n", s);
}
