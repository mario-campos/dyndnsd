%{
#include "parse.h"
void unquote(const char *s);
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
{quoted_string}		{ unquote(yytext); return STRING; }
{unquoted_string}	return STRING;

%%

void unquote(const char *s) {
    yytext[--yyleng] = '\0';
    yytext++;
    yyleng--;
}

void yyerror(const char *s) {
    fprintf(stderr, "%s\n", s);
}
