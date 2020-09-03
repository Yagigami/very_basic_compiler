#include <string.h>
#include <ctype.h>
#include <stdnoreturn.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "xallang.h"


#define CMP(s1, s2) strncmp((s1), (s2), sizeof (s1))

noreturn void error(const char *msg, ...)
{
	fprintf(stderr, "error (%.32s): ", stream);
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	fputc('\n', stderr);
	va_end(args);
	exit(1);
}

struct identifier parse_cidentifier(void)
{
	skip_whitespace();
	struct identifier id = { .name = stream, };
	while (isalpha(*stream) || *stream == '_') stream++;
	id.len = stream - id.name;
	skip_whitespace();
	return id;
}

uint64_t parse_u64(void)
{
	uint64_t n = 0;
	skip_whitespace();
	while (isdigit(*stream)) {
		n = n * 10 + *stream++ - '0';
		if (*stream == '_' || *stream == '\'') stream++;
	}
	skip_whitespace();
	return n;
}

void skip_whitespace(void)
{
	while (isspace(*stream)) stream++;
}

int xl_parse_program(struct xallang_program *pgrm)
{
	skip_whitespace();
	while (*stream == '(') {
		stream++;
		if (CMP("def", stream) == 0) {
			struct xallang_definition def;
			xl_parse_definition(&def);
		} else {
			error("expected 'def' keyword");
		}
	}
}

int xl_parse_definition(struct xallang_definition *def)
{
}

int xl_parse_block(struct xallang_block *blk)
{
}

int xl_parse_statement(struct xallang_statement *stmt)
{
}

int xl_parse_intexpr(struct xallang_intexpression *expr)
{
}

int xl_parse_boolexpr(struct xallang_boolexpression *expr)
{
}

