%{
extern int yylex();
extern int yyerror();
%}

%token INTERFACE
%token DOMAIN
%token HTTP_AUTH
%token HTTP_URL
%token HTTP_HEADER
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
	| DOMAIN STRING '{' parameters '}'
	;

parameters
	: parameter
	| parameters parameter
	;

parameter 
	: HTTP_URL STRING
	| HTTP_AUTH STRING STRING
	| HTTP_HEADER STRING STRING
	;
