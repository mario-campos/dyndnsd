%{
#include <string.h>
#include "parser.h"
void yyerror(struct ast **, const char *);
%}

%option nounput
%option noyywrap
%option yylineno

comment			#.*
whitespace		[ \t\n]+
quoted_string		"\""([^"\\]|\\.)*"\""
unquoted_string		[^ \n\t]+
unterminated_string	("\""{unquoted_string}|{unquoted_string}"\"")

%%

interface		return INTERFACE;
domain			return DOMAIN;
update	 		return UPDATE;
"{"			return '{';
"}"			return '}';
{comment}		/* ignore */ ;
{whitespace}		/* ignore */ ;
{quoted_string}		{ yylval.string = strndup(yytext+1, yyleng-2); return STRING; }
{unterminated_string}	yyerror(NULL, "unterminated string");
{unquoted_string}	{ yylval.string = strndup(yytext, yyleng); return STRING; }

%%

void yyerror(struct ast **ast, const char *msg) {
    fprintf(stderr, "%d: syntax error: %s (%s)\n", yylineno, msg, yytext);
}
