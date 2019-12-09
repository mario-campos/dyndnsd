#ifndef PARSER_H
#define PARSER_H

struct ast_iface {
	char *if_name;
	size_t domain_len;
	char *domain[];
};

struct ast {
	char *cmd;
	size_t iface_len;
	struct ast_iface *iface[];
};

struct ast *parse(const char *);
const char *parse_err(void);

#endif /* PARSER_H */
