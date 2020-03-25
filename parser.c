#include <sys/mman.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "parser.h"

char parser_error[100];

enum tokentype {
	T_EOF = 0,
	T_RUN,
	T_INTERFACE,
	T_DOMAIN,
	T_LBRACE,
	T_RBRACE,
	T_LINEFEED,
	T_SPACE,
	T_QUOTE,
	T_STRING
};

struct lexer {
	const char *lex_path;
	const char *lex_text;
	const char *lex_lptr;
	int         lex_lnum;
	size_t      lex_size;
	size_t      lex_read;
};

struct token {
	const char     *tok_text;
	size_t	    	tok_size;
	enum tokentype 	tok_type;
};

static size_t
consume_ws(const char *s, size_t slen)
{
	size_t i;
	for (i = 0; i < slen; i++)
		if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n')
			break;
	return i;
}

static size_t
consume_comment(const char *s, size_t slen)
{
	size_t i;
	for (i = 0; i < slen; i++)
		if (s[i] == '\n') break;
	return i;
}

static size_t
consume_string(const char *s, size_t slen)
{
	size_t i;
	for (i = 0; i < slen; i++) {
		if (s[i] == '{' || s[i] == '}' || s[i] == '#' || s[i] == '"' ||
		    s[i] == ' ' || s[i] == '\t' || s[i] == '\n')
			break;
	} return i;
}

static enum tokentype
next_token(struct lexer *lex, struct token *tok)
{
	size_t delta;
	const char *s;

	s = &lex->lex_text[lex->lex_read];
	delta = lex->lex_size - lex->lex_read;

	if (lex->lex_read >= lex->lex_size) {
		tok->tok_size = 0;
		tok->tok_type = T_EOF;
	}
	else if (delta >= (sizeof("run")-1) && s[0] == 'r' && s[1] == 'u' && s[2] == 'n') {
		tok->tok_size = sizeof("run") - 1;
		tok->tok_type = T_RUN;
	}
	else if (delta >= (sizeof("interface")-1) && s[0] == 'i' && s[1] == 'n' && s[2] == 't'
	    && s[3] == 'e' && s[4] == 'r' && s[5] == 'f' && s[6] == 'a' && s[7] == 'c' && s[8] == 'e') {
		tok->tok_size = sizeof("interface") - 1;
		tok->tok_type = T_INTERFACE;
	}
	else if (delta >= (sizeof("domain")-1) && s[0] == 'd' && s[1] == 'o' &&
	    s[2] == 'm' && s[3] == 'a' && s[4] == 'i' && s[5] == 'n') {
		tok->tok_size = sizeof("domain") - 1;
		tok->tok_type = T_DOMAIN;
	}
	else if (*s == '{') {
		tok->tok_size = 1;
		tok->tok_type = T_LBRACE;
	}
	else if (*s == '}') {
		tok->tok_size = 1;
		tok->tok_type = T_RBRACE;
	}
	else if (*s == '\n') {
		tok->tok_size = 1;
		tok->tok_type = T_LINEFEED;
		lex->lex_lptr = s;
		lex->lex_lnum++;
	}
	else if (*s == ' ' || *s == '\t') {
		tok->tok_size = consume_ws(s, delta);
		tok->tok_type = T_SPACE;
	}
	else if (*s == '#') {
		lex->lex_read += consume_comment(s, delta);
		return next_token(lex, tok);
	}
	else if (*s == '"') {
		size_t i;
		for (i = 1; i <= delta; i++) {
			if (s[i] == '"') {
				tok->tok_type = T_QUOTE;
				break;
			}
			if (s[i] == '\n') {
				tok->tok_type = T_STRING;
				break;
			}
		}
		tok->tok_size = i + 1;
	}
	else {
		tok->tok_size = consume_string(s, delta);
		tok->tok_type = T_STRING;
	}

	tok->tok_text = s;
	lex->lex_read += tok->tok_size;
	return tok->tok_type;
}

static void
error(struct lexer *lex, struct token *tok, const char *e)
{
	snprintf(parser_error, sizeof(parser_error), "%s:%d:%zu: %s",
	    lex->lex_path, lex->lex_lnum, tok->tok_text - lex->lex_lptr, e);
}

struct ast *
parse(const char *path)
{
	int fd;
	struct stat st;
	const char *text;
	struct lexer lex;
	struct token tok;
	struct ast_if *aif;
	struct ast_dn *adn;
	struct ast *ast;

	if (-1 == (fd = open(path, O_RDONLY|O_CLOEXEC))) {
		snprintf(parser_error, sizeof(parser_error), "open(2): %s", strerror(errno));
		return NULL;
	}

	if (-1 == fstat(fd, &st)) {
		snprintf(parser_error, sizeof(parser_error), "fstat(2): %s", strerror(errno));
		close(fd);
		return NULL;
	}

	if (MAP_FAILED == (text = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0))) {
		snprintf(parser_error, sizeof(parser_error), "mmap(2): %s", strerror(errno));
		close(fd);
		return NULL;
	}

	if (NULL == (ast = malloc(sizeof(*ast)))) {
		snprintf(parser_error, sizeof(parser_error), "malloc(3): %s", strerror(errno));
		return NULL;
	}

	close(fd);
	SLIST_INIT(&ast->aif_head);
	madvise((void*)text, st.st_size, MADV_SEQUENTIAL);

	lex.lex_path = path;
	lex.lex_text = text;
	lex.lex_lptr = text;
	lex.lex_size = st.st_size;
	lex.lex_read = 0;
	lex.lex_lnum = 1;

S_TOP:
	switch (next_token(&lex, &tok)) {
	case T_EOF: goto S_END;
	case T_SPACE:
	case T_LINEFEED: goto S_TOP;
	case T_RUN: goto S_RUN;
	case T_INTERFACE:
		if (NULL == (aif = malloc(sizeof(*aif)))) {
			snprintf(parser_error, sizeof(parser_error), "malloc(3): %s", strerror(errno));
			goto S_ERROR;
		}
		SLIST_INIT(&aif->adn_head);
		SLIST_INSERT_HEAD(&ast->aif_head, aif, next);
		goto S_INTERFACE;
	default:
		error(&lex, &tok, "invalid syntax: expected 'run', 'interface', EOF, or whitespace");
		goto S_ERROR;
	}

S_RUN:
	switch (next_token(&lex, &tok)) {
	case T_SPACE: goto S_RUN_SPACE;
	default:
		error(&lex, &tok, "invalid syntax: expected whitespace after 'run'");
		goto S_ERROR;
	}

S_RUN_SPACE:
	switch (next_token(&lex, &tok)) {
	case T_INTERFACE:
	case T_DOMAIN:
	case T_RUN:
	case T_LBRACE:
	case T_RBRACE:
	case T_QUOTE:
		ast->run = strndup(&tok.tok_text[1], tok.tok_size-2);
		goto S_RUN_QUOTE;
	case T_STRING:
		ast->run = tok.tok_text;
		goto S_RUN_STRING;
	default:
		error(&lex, &tok, "invalid syntax: expected command after 'run'");
		goto S_ERROR;
	}

S_RUN_QUOTE:
	switch (next_token(&lex, &tok)) {
	case T_EOF: goto S_END;
	case T_SPACE: goto S_RUN_QUOTE;
	case T_LINEFEED: goto S_TOP;
	default:
		error(&lex, &tok, "invalid syntax: expected '\\n' or EOF");
		goto S_ERROR;
	}

S_RUN_STRING:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_INTERFACE:
	case T_DOMAIN:
	case T_RUN:
	case T_LBRACE:
	case T_RBRACE:
	case T_STRING:
	case T_QUOTE: goto S_RUN_STRING;
	case T_LINEFEED:
	case T_EOF:
		ast->run = strndup(ast->run, tok.tok_text - ast->run);
		goto S_TOP;
	}

S_INTERFACE:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_INTERFACE_SPACE;
	default:
		error(&lex, &tok, "invalid syntax: expected whitespace after 'interface'");
		goto S_ERROR;
	}

S_INTERFACE_SPACE:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_INTERFACE_SPACE;
	case T_QUOTE:
		tok.tok_text += 1;
		tok.tok_size -= 2;
		/* fallthrough */
	case T_STRING: {
		size_t count = tok.tok_size < sizeof(aif->name) ?
		    tok.tok_size : sizeof(aif->name) - 1;
		strncpy(aif->name, tok.tok_text, count);
		aif->name[count] = '\0';
		goto S_INTERFACE_STRING;
	}
	default:
		error(&lex, &tok, "invalid syntax: expected interface name");
		goto S_ERROR;
	}

S_INTERFACE_STRING:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_INTERFACE_STRING;
	case T_LBRACE: goto S_INTERFACE_LBRACE;
	default:
		error(&lex, &tok, "invalid syntax: expected '{' or whitespace");
		goto S_ERROR;
	}

S_INTERFACE_LBRACE:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_INTERFACE_LBRACE;
	case T_DOMAIN: goto S_DOMAIN;
	default:
		error(&lex, &tok, "invalid syntax: expected 'domain' or whitespace");
		goto S_ERROR;
	}

S_DOMAIN:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_DOMAIN_SPACE;
	default:
		error(&lex, &tok, "invalid syntax: expected whitespace after 'domain'");
		goto S_ERROR;
	}

S_DOMAIN_SPACE:
	switch (next_token(&lex, &tok)) {
	case T_QUOTE:
		tok.tok_text += 1;
		tok.tok_size -= 2;
		/* fallthrough */
	case T_STRING:
		if (NULL == (adn = malloc(sizeof(*adn) + tok.tok_size + 1))) {
			snprintf(parser_error, sizeof(parser_error), "malloc(3): %s", strerror(errno));
			goto S_ERROR;
		}
		strncpy(adn->name, tok.tok_text, tok.tok_size);
		adn->name[tok.tok_size] = '\0';
		SLIST_INSERT_HEAD(&aif->adn_head, adn, next);
		goto S_DOMAIN_STRING;
	default:
		error(&lex, &tok, "invalid syntax: expected domain name");
		goto S_ERROR;
	}

S_DOMAIN_STRING:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_DOMAIN_STRING;
	case T_DOMAIN: goto S_DOMAIN;
	case T_RBRACE: goto S_INTERFACE_RBRACE;
	default:
		error(&lex, &tok, "invalid syntax: expected 'domain', '}', or whitespace");
		goto S_ERROR;
	}

S_INTERFACE_RBRACE:
	switch (next_token(&lex, &tok)) {
	case T_RUN: goto S_RUN;
	case T_INTERFACE: goto S_INTERFACE;
	case T_SPACE:
	case T_LINEFEED: goto S_TOP;
	case T_EOF: goto S_END;
	default:
		error(&lex, &tok, "invalid syntax: expected 'interface', 'run', EOF, or whitespace");
		goto S_ERROR;
	}

S_ERROR:
	// free that which was already parsed
	while (!SLIST_EMPTY(&ast->aif_head)) {
		aif = SLIST_FIRST(&ast->aif_head);
		while (!SLIST_EMPTY(&aif->adn_head)) {
			adn = SLIST_FIRST(&aif->adn_head);
			SLIST_REMOVE_HEAD(&aif->adn_head, next);
			free(adn);
		}
		SLIST_REMOVE_HEAD(&ast->aif_head, next);
		free(aif);
	}
	free((char*)ast->run);
	free(ast);
	ast = NULL;

S_END:
	munmap((void*)lex.lex_text, lex.lex_size);
	return ast;
}
