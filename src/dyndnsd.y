%token INTERFACE
%token DOMAIN
%token HTTP_AUTH
%token HTTP_URL
%token HTTP_HEADER
%token STRING

%%

configuration
	: interfaces
	| parameters interfaces
	;

interfaces
	: interface
	| interfaces interface
	;

interface
	: INTERFACE STRING '{' domains '}'
	| INTERFACE STRING '{' parameters domains '}'
	;

domains	: domain
	| domains domain
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
