#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/stat.h>
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
	size_t      lex_size;
	size_t      lex_read;
};

struct token {
	const char *tok_text;
	size_t	    tok_size;
	enum tokentype tok_type;
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
	else if (delta >= 3 && s[0] == 'r' && s[1] == 'u' && s[2] == 'n') {
		tok->tok_size = 3;
		tok->tok_type = T_RUN;
	}
	else if (delta >= 9 && s[0] == 'i' && s[1] == 'n' && s[2] == 't' && s[3] == 'e'
	    && s[4] == 'r' && s[5] == 'f' && s[6] == 'a' && s[7] == 'c' && s[8] == 'e') {
		tok->tok_size = 9;
		tok->tok_type = T_INTERFACE;
	}
	else if (delta >= 6 && s[0] == 'd' && s[1] == 'o' &&
	    s[2] == 'm' && s[3] == 'a' && s[4] == 'i' && s[5] == 'n') {
		tok->tok_size = 6;
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
error(struct lexer *lex, const char *e)
{
	snprintf(parser_error, sizeof(parser_error),
	    "%s:%zu: %s", lex->lex_path, lex->lex_read, e);		
}

struct cst *
parse(const char *path)
{
	int fd;
	struct stat st;
	const char *text, *run_cmd = NULL;
	struct lexer lex;
	struct token tok;
	struct cst_iface *cif;
	struct cst_domain *cdo;
	struct cst *cst;

	// TODO: error handling
	cst = malloc(sizeof(*cst));
	SLIST_INIT(&cst->ifaces);

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

	close(fd);

	lex.lex_path = path;
	lex.lex_text = text;
	lex.lex_size = st.st_size;
	lex.lex_read = 0;

S_TOP:
	switch (next_token(&lex, &tok)) {
	case T_EOF: goto S_END;
	case T_SPACE:
	case T_LINEFEED: goto S_TOP;
	case T_RUN: goto S_RUN;
	case T_INTERFACE: goto S_INTERFACE;
	default:
		error(&lex, "expected 'run' or 'interface'");
		goto S_ERROR;
	}

S_INTERFACE:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_INTERFACE_SPACE;
	default:
		error(&lex, "expected whitespace after 'interface'");
		goto S_ERROR;
	}

S_INTERFACE_SPACE:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_INTERFACE_SPACE;
	case T_QUOTE:
		tok.tok_text += 1;
		tok.tok_size -= 2;
	case T_STRING: {
		// TODO: error handling
		cif = malloc(sizeof(*cif));
		SLIST_INIT(&cif->domains);
		size_t count = tok.tok_size < sizeof(cif->if_name) ?
		    tok.tok_size : sizeof(cif->if_name) - 1;
		strncpy(cif->if_name, tok.tok_text, count);
		cif->if_name[count] = '\0';
		goto S_INTERFACE_STRING;
	}
	default:
		error(&lex, "expected interface name");
		goto S_ERROR;
	}

S_INTERFACE_STRING:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_INTERFACE_STRING;
	case T_LBRACE: goto S_INTERFACE_LBRACE;
	default:
		error(&lex, "expected '{'");
		free(cif);
		goto S_ERROR;
	}

S_INTERFACE_LBRACE:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_INTERFACE_LBRACE;
	case T_DOMAIN: goto S_DOMAIN;
	default:
		error(&lex, "expected 'domain'");
		free(cif);
		goto S_ERROR;
	}

S_DOMAIN:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_DOMAIN_SPACE;
	default:
		error(&lex, "expected whitespace after 'domain'");
		while (!SLIST_EMPTY(&cif->domains)) {
			cdo = SLIST_FIRST(&cif->domains);
			SLIST_REMOVE_HEAD(&cif->domains, next);
			free(cdo);
		}
		free(cif);
		goto S_ERROR;
	}

S_DOMAIN_SPACE:
	switch (next_token(&lex, &tok)) {
	case T_QUOTE:
		tok.tok_text += 1;
		tok.tok_size -= 2;
	case T_STRING:
		// TODO: error handling
		cdo = malloc(sizeof(*cdo) + tok.tok_size + 1);
		strncpy(cdo->domain, tok.tok_text, tok.tok_size);
		cdo->domain[tok.tok_size] = '\0';
		SLIST_INSERT_HEAD(&cif->domains, cdo, next);
		goto S_DOMAIN_STRING;
	default:
		error(&lex, "expected domain name");
		while (!SLIST_EMPTY(&cif->domains)) {
			cdo = SLIST_FIRST(&cif->domains);
			SLIST_REMOVE_HEAD(&cif->domains, next);
			free(cdo);
		}
		free(cif);
		goto S_ERROR;
	}

S_DOMAIN_STRING:
	switch (next_token(&lex, &tok)) {
	case T_SPACE:
	case T_LINEFEED: goto S_DOMAIN_STRING;
	case T_DOMAIN: goto S_DOMAIN;
	case T_RBRACE:
		SLIST_INSERT_HEAD(&cst->ifaces, cif, next);
		goto S_INTERFACE_RBRACE;
	default:
		error(&lex, "expected 'domain' or '}'");
		while (!SLIST_EMPTY(&cif->domains)) {
			cdo = SLIST_FIRST(&cif->domains);
			SLIST_REMOVE_HEAD(&cif->domains, next);
			free(cdo);
		}
		free(cif);
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
		error(&lex, "expected whitespace, 'interface', or 'run'");
		goto S_ERROR;
	}

S_RUN:
	switch (next_token(&lex, &tok)) {
	case T_SPACE: goto S_RUN_SPACE;
	default:
		error(&lex, "expected whitespace after 'run'");
		goto S_ERROR;
	}

S_RUN_SPACE:
	switch (next_token(&lex, &tok)) {
	case T_INTERFACE:
	case T_DOMAIN:
	case T_RUN:
	case T_LBRACE:
	case T_RBRACE:
	case T_STRING:
		run_cmd = tok.tok_text;
		goto S_RUN_STRING;
	case T_QUOTE:
		run_cmd = strndup(&tok.tok_text[1], tok.tok_size-2);
		goto S_RUN_QUOTE;
	default:
		error(&lex, "expected command after 'run'");
	}

S_RUN_QUOTE:
	switch (next_token(&lex, &tok)) {
	case T_EOF: goto S_END;
	case T_SPACE: goto S_RUN_QUOTE;
	case T_LINEFEED: goto S_TOP;
	default:
		error(&lex, "expected '\\n'");
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
		run_cmd = strndup(run_cmd, tok.tok_text - run_cmd);
		goto S_TOP;
	}

S_ERROR:
	while (!SLIST_EMPTY(&cst->ifaces)) {
		cif = SLIST_FIRST(&cst->ifaces);
		while (!SLIST_EMPTY(&cif->domains)) {
			cdo = SLIST_FIRST(&cif->domains);
			SLIST_REMOVE_HEAD(&cif->domains, next);
			free(cdo);
		}
		SLIST_REMOVE_HEAD(&cst->ifaces, next);
		free(cif);
	}
	free(cst);
	free((char*)run_cmd);
	return NULL;

S_END:
	return cst;
}
