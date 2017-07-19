%{
#include <string.h>
#include "parser.h"
void yyerror(struct ast **, const char *);
%}

%option nounput
%option noyywrap
%option yylineno

comment			"#"[^\n]*
whitespace		[ \t\n]+
quoted_string		"\""([^"\\]|\\.)*"\""
unquoted_string		[^ \n\t]+

%%

"interface"		return INTERFACE;
"domain"		return DOMAIN;
"update" 		return UPDATE;
"{"			return '{';
"}"			return '}';
{comment}		;
{whitespace}		;
{quoted_string}		{ yylval.str = strndup(yytext+1, yyleng-2); return STRING; }
{unquoted_string}	{ yylval.str = strndup(yytext, yyleng); return STRING; }

%%

void yyerror(struct ast **ast, const char *s) {
    fprintf(stderr, "%s (line %d)\n", s, yylineno);
}
