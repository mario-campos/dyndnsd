%{
extern int yylex();
extern int yyerror();
%}

%token INTERFACE
%token DOMAIN
%token HTTP_GET
%token STRING

%%

statements0
	: parameter
	| interface
	| statements0 parameter
	| statements0 interface
	;

interface
	: INTERFACE STRING '{' statements1 '}'
	;

statements1
	: parameter
	| domain
	| statements1 parameter
	| statements1 domain
	;

domain	: DOMAIN STRING
	| DOMAIN STRING '{' parameter '}'
	;

parameter 
	: HTTP_GET STRING
	;
