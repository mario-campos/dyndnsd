%{
#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "y.tab.h"

extern char *filename;
bool error = false;
%}

%option noinput
%option nounput
%option noyywrap
%option yylineno

newline			\n+
comment			#.*
whitespace		[ \t]+
quoted_string		\"([^"\n\\]|\\.)*\"
lunquoted_string	[^" \n\t]+\"
runquoted_string	\"[^" \n\t]+
unquoted_string		[^ \n\t]+
lineescape		\\.*\n

%%
interface		{ yylval.lineno = yylineno; return INTERFACE; }
domain			{ yylval.lineno = yylineno; return DOMAIN; }
run			{ yylval.lineno = yylineno; return RUN; }
"{"			return '{';
"}"			return '}';
{newline}		return '\n';
{comment}		/* ignore */ ;
{whitespace}		/* ignore */ ;
{lineescape}		/* ignore */ ;
{quoted_string}		{ yylval.string = strdup(yytext); return QUOTE; }
{lunquoted_string}	{ yylval.string = strdup(yytext); return LUNQUOTE; }
{runquoted_string}	{ yylval.string = strdup(yytext); return RUNQUOTE; }
{unquoted_string}	{ yylval.string = strdup(yytext); return STRING; }

%%

void
yyerror(const char *fmt, ...)
{
	va_list ap;
	char *msg;

	va_start(ap, fmt);
	if (-1 == vasprintf(&msg, fmt, ap))
		err(1, "vasprintf");
	va_end(ap);

	error = true;

	if (0 == strcmp(msg, "syntax error"))
		/*
		   If yyerror() is invoked by yacc, the lexer's line
		   number is likely the most accurate indicator of the
		   the error.
		*/
		fprintf(stderr, "%s:%d: error: syntax error\n", filename, yylineno);
	else
		/*
		   However, if yyerror() is self-invoked, then the lexer's
		   line number is likely too far ahead; the accurate line
		   number will be provided in 'msg'.
		*/
		fprintf(stderr, "%s\n", msg);

	free(msg);
}
