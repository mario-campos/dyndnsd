#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "parser.h"

char parser_error[100];
struct lexer lexer;

enum tokentype {
	TOKEN_RUN = 0,
	TOKEN_INTERFACE,
	TOKEN_DOMAIN,
	TOKEN_STRING,
	TOKEN_QUOTE,
	TOKEN_WHITESPACE,
	TOKEN_COMMENT, 
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_ERROR,
	TOKEN_EOF
};

struct lexer {
	const char *lex_text;
	size_t      lex_size;
	size_t      lex_read;
};

struct token {
	const char     *tok_text;
	size_t		tok_size;
	enum tokentype 	tok_type;
};

static bool
consume(const char *s, size_t slen, const char *t, size_t tlen)
{
	return 0 == strncmp(s, t, tlen <= slen ? tlen : slen);
}

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

static void
next_token(struct token *t)
{
	size_t delta;
	const char *s;

	if (lexer.lex_read >= lexer.lex_size) {
		t->tok_type = TOKEN_EOF;
		return;
	}

	s = &lexer.lex_text[lexer.lex_read];
	delta = lexer.lex_size - lexer.lex_read;

	if (consume(s, delta, "run", strlen("run"))) {
		t->tok_size = strlen("run");
		t->tok_type = TOKEN_RUN;
	}
	else if (consume(s, delta, "interface", strlen("interface"))) {
		t->tok_size = strlen("interface");
		t->tok_type = TOKEN_INTERFACE;
	}
	else if (consume(s, delta, "domain", strlen("domain"))) {
		t->tok_size = strlen("domain");
		t->tok_type = TOKEN_DOMAIN;
	}
	else if (*s == '{') {
		t->tok_size = 1;
		t->tok_type = TOKEN_LBRACE;
	}
	else if (*s == '}') {
		t->tok_size = 1;
		t->tok_type = TOKEN_RBRACE;
	}
	else if (*s == ' ' || *s == '\t' || *s == '\n') {
		t->tok_size = consume_ws(s, delta);
		t->tok_type = TOKEN_WHITESPACE;
	}
	else if (*s == '#') {
		t->tok_size = consume_comment(s, delta);
		t->tok_type = TOKEN_COMMENT;
	}
	else if (*s == '"') {
		size_t i;
		for (i = 1; i <= delta; i++) {
			if (s[i] == '"') {
				t->tok_type = TOKEN_QUOTE;
				break;
			}
			if (s[i] == '\n') {
				t->tok_type = TOKEN_ERROR;
				break;
			}
		}
		t->tok_size = i + 1;
	}
	else {
		t->tok_size = consume_string(s, delta);
		t->tok_type = TOKEN_STRING;
	}

	t->tok_text = s;
	lexer.lex_read += t->tok_size;
}

static enum tokentype
peek(void)
{
	size_t offset;
	struct token token;

	offset = lexer.lex_read;
	next_token(&token);
	lexer.lex_read = offset;
	return token.tok_type;
}

static bool
accept(enum tokentype t)
{
	if (peek() == t) {
		struct token token;
		next_token(&token);
		return true;
	}
	return false;
}

static void
parse_err(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(parser_error, sizeof(parser_error), fmt, ap);
	va_end(ap);
}

struct ast *
parse(const char *path)
{
	int fd;
	FILE *fstream;
	struct stat stat;
	struct token token;
	const char *text;

	if (-1 == (fd = open(path, O_RDONLY|O_CLOEXEC))) {
		parse_err("%s: %s", "open(2)", strerror(errno));
		goto end;
	}

	if (NULL == (fstream = fdopen(fd, "r"))) {
		parse_err("%s: %s", "fdopen(3)", strerror(errno));
		goto end;
	}

	if (-1 == fstat(fd, &stat)) {
		parse_err("%s: %s", "fstat(2)", strerror(errno));
		goto end;
	}

	if (MAP_FAILED == (text = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0))) {
		parse_err("%s: %s", "mmap(2)", strerror(errno));
		goto end;
	}

	lexer.lex_text = text;
	lexer.lex_size = stat.st_size;
	lexer.lex_read = 0;

	do {
		next_token(&token);
		if (TOKEN_RUN == token.tok_type) {
			next_token(&token);
			if (TOKEN_WHITESPACE != token.tok_type) {
				parse_err("%s: %s", path, "expected TOKEN_WHITESPACE");
				goto end;
			}

			next_token(&token);
			if (TOKEN_STRING != token.tok_type &&
			    TOKEN_QUOTE != token.tok_type) {
				parse_err("%s: %s", path, "expected STRING or QUOTE");
				goto end;
			}
		}
		else if (TOKEN_INTERFACE == token.tok_type) {
			next_token(&token);
			if (TOKEN_WHITESPACE != token.tok_type) {
				parse_err("%s: %s", path, "expected TOKEN_WHITESPACE");
				goto end;
			}

			next_token(&token);
			if (TOKEN_QUOTE != token.tok_type
			&& TOKEN_STRING != token.tok_type) {
				parse_err("%s: %s", path, "expected STRING or QUOTE");
				goto end;
			}

			accept(TOKEN_WHITESPACE);

			next_token(&token);
			if (TOKEN_LBRACE != token.tok_type) {
				parse_err("%s: %s", path, "expected TOKEN_LBRACE");
				goto end;
			}

			accept(TOKEN_WHITESPACE);

			do {
				accept(TOKEN_WHITESPACE);

				next_token(&token);
				if (TOKEN_DOMAIN != token.tok_type) {
					parse_err("%s: %s", path, "expected TOKEN_DOMAIN");
					goto end;
				}

				next_token(&token);
				if (TOKEN_WHITESPACE != token.tok_type) {
					parse_err("%s: %s", path, "expected TOKEN_WHITESPACE");
					goto end;
				}

				next_token(&token);
				if (TOKEN_QUOTE != token.tok_type
				&& TOKEN_STRING != token.tok_type) {
					parse_err("%s: %s", path, "expected STRING or QUOTE");
					goto end;
				}

				accept(TOKEN_WHITESPACE);
			} while (TOKEN_RBRACE != peek());

			next_token(&token);
			if (TOKEN_RBRACE != token.tok_type) {
				parse_err("%s: %s", path, "expected TOKEN_RBRACE");
				goto end;
			}
		}
		else if (TOKEN_WHITESPACE == token.tok_type) {
			continue;
		}
		else if (TOKEN_EOF == token.tok_type) {
			break;
		}
		else {
			parse_err("%s: %s", path, "expected TOKEN_INTERFACE or TOKEN_RUN");
			goto end;
		}
	} while (true);

end:
	if (EOF == fclose(fstream))
		parse_err("%s: %s", "fclose(3)", strerror(errno));

	return NULL;
}
