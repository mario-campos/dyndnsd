%{
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

#include "ast.h"
#include "parser.h"

#define YYMAXLENG 15

extern bool ast_error;
bool ast_error = false;

void yyerror(struct ast_root **, const char *);
%}

%option nounput
%option noyywrap
%option yylineno

comment			#.*
whitespace		[ \t\n]+
quoted_string		\"([^"\\]|\\.)*\"
unquoted_string		[^ \n\t]+

%%

user			return USER;
group			return GROUP;
interface		return INTERFACE;
domain			return DOMAIN;
update	 		return UPDATE;
"{"			return '{';
"}"			return '}';
{comment}		/* ignore */ ;
{whitespace}		/* ignore */ ;
{quoted_string}		{ yylval.string = strndup(yytext+1, yyleng-2); return STRING; }
\"{unquoted_string}	{
				yyerror(NULL, "this string lacks an closing quotation character");
				yylval.string = strdup(yytext+1);
				ast_error = true;
				return STRING;
			}
{unquoted_string}\"	{
				yyerror(NULL, "this string lacks an opening quotation character");
				yylval.string = strndup(yytext, yyleng-1);
				ast_error = true;
				return STRING;
			}
{unquoted_string}	{ yylval.string = strndup(yytext, yyleng); return STRING; }

%%

void
yyerror(struct ast_root **ast, const char *msg)
{
	const char *fmtstr;
	if (yyleng > YYMAXLENG)
		fmtstr = "error: %s (%.*s...)";
	else
		fmtstr = "error: %s (%.*s)";
	syslog(LOG_ERR, fmtstr, msg, YYMAXLENG, yytext);
}
