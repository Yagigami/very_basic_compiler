#include <string.h>
#include <ctype.h>
#include <stdnoreturn.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "utils.h"
#include "xallang.h"


#define INDENT_SH 4

void fprintln(FILE *f, int indent, const char *s) {
	static const char *fill =
		"                                                                      "
		"                                                                      "
		"                                                                      "
		"                                                                      "
	;
	fprintf(f, "%s\n%.*s", s, indent, fill);
}

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
	while (*stream == '#' || isspace(*stream)) {
		if (*stream == '#') do stream++; while (*stream != '\n');
		stream++;
	}
}

void xl_parse_program(struct xallang_program *pgrm)
{
	while (skip_whitespace(), *stream == '(') {
		stream++;
		if (MATCH_KW("def")) {
			struct xallang_definition def = {0};
			xl_parse_definition(&def);
			buf_push(pgrm->defs, def);
		} else {
			error("expected \"def\" keyword");
		}
		if (*stream++ != ')') error("expected a closing ')' at the end of definition.");
	}
}

void xl_parse_definition(struct xallang_definition *def)
{
	def->name = parse_cidentifier();
	skip_whitespace();
	if (*stream++ != '(') error("expected a '(' before argument list in function definition");
	for (ssize_t i = 0; skip_whitespace(), *stream != ')'; i++) {
		if (i) {
			if (*stream != ',') error("expected a comma to separate definition parameters");
			stream++;
		}
		buf_push(def->params, parse_cidentifier());

	}
	while (skip_whitespace(), *stream != ')') {
	}
	stream++;
	xl_parse_block(&def->blk);
	def->retval = xl_parse_intexpr();
}

void xl_parse_block(struct xallang_block *blk)
{
	while (skip_whitespace(), *stream == '(') {
		struct xallang_statement stmt = {0};
		xl_parse_statement(&stmt);
		buf_push(blk->stmts, stmt);
	}
}

void xl_parse_statement(struct xallang_statement *stmt)
{
	if (*stream++ != '(') error("expected a '(' at the first token in a statement");
	skip_whitespace();
	if (MATCH_KW("set")) {
		stmt->xset.id = parse_cidentifier();
		stmt->xset.val = xl_parse_intexpr();
	} else if (MATCH_KW("if")) {
		stmt->xif.cond = xl_parse_boolexpr();
		skip_whitespace();
		if (*stream++ != '(') error("expected a '(' token before the start of if's then block");
		xl_parse_block(&stmt->xif.thenb);
		skip_whitespace();
		if (*stream++ != ')') error("expected a ')' token before the end of if's then block");
		skip_whitespace();
		if (*stream++ != '(') error("expected a '(' token before the start of if's else block");
		xl_parse_block(&stmt->xif.elseb);
		skip_whitespace();
		if (*stream++ != ')') error("expected a ')' token before the end of if's else block");
	} else if (MATCH_KW("while")) {
		stmt->xwhile.cond = xl_parse_boolexpr();
		xl_parse_block(&stmt->xwhile.body);
	} else {
		error("unknown statement kind");
	}
	skip_whitespace();
	if (*stream++ != ')') error("expected closing ')' at the end of statement");
}

struct xallang_intexpression *xl_parse_intexpr(void)
{
	struct xallang_intexpression *iexpr = xmalloc(sizeof *iexpr);
	skip_whitespace();
	if (*stream == '(') {
		stream++;
		skip_whitespace();
		if (*stream == '+') {
			stream++;
			iexpr->kind = XALLANG_SUM;
			iexpr->lhs = xl_parse_intexpr();
			iexpr->rhs = xl_parse_intexpr();
		} else {
			error("unknown operator in intexpression");
		}
		skip_whitespace();
		if (*stream++ != ')') error("expected a closing ')' at the end of expression");
	} else if (isdigit(*stream)) {
		iexpr->kind = XALLANG_INT;
		iexpr->xint = parse_u64();
	} else if (isalpha(*stream) || *stream == '_') {
		iexpr->kind = XALLANG_ID;
		iexpr->xid = parse_cidentifier();
	}

	return iexpr;
}

struct xallang_boolexpression *xl_parse_boolexpr(void)
{
	struct xallang_boolexpression *bexpr = xmalloc(sizeof *bexpr);
	skip_whitespace();
	if (*stream == '(') {
		stream++;
		skip_whitespace();
		if (*stream == '=') {
			bexpr->kind = XALLANG_EQ;
			bexpr->lhs = xl_parse_intexpr();
			bexpr->rhs = xl_parse_intexpr();
		} else if (*stream == '<') {
			bexpr->kind = XALLANG_LT;
			bexpr->lhs = xl_parse_intexpr();
			bexpr->rhs = xl_parse_intexpr();
		} else if (MATCH_KW("not")) {
			bexpr->kind = XALLANG_NOT;
			bexpr->xbool = xl_parse_boolexpr();
		} else {
			error("unknown boolexpression");
		}
		skip_whitespace();
		if (*stream++ != ')') error("expected a closing ')' at the end of boolexpression");
	} else {
		error("boolexpressions start with a '('");
	}

	return bexpr;
}

void xl_dump_program(FILE *f, int indent, struct xallang_program *pgrm)
{
	fprintln(f, indent + INDENT_SH, "{");
	fprintf(f, "\"defs\": [");
	for (ssize_t i = 0; i < buf_len(pgrm->defs); i++) {
		if (i) fprintln(f, indent + INDENT_SH, ",");
		xl_dump_definition(f, indent + INDENT_SH, pgrm->defs + i);
	}
	fprintf(f, "\n] }\n");
}

void xl_dump_definition(FILE *f, int indent, struct xallang_definition *def)
{
	fprintln(f, indent + INDENT_SH, "{");
	fprintf(f, "\"name\": \"%.*s\"", (int) def->name.len, def->name.name);
	fprintln(f, indent, ",");
	fprintf(f, " \"params\": [ ");
	for (ssize_t i = 0; i < buf_len(def->params); i++) {
		if (i) fprintf(f, ", ");
		fprintf(f, "\"%.*s\"", (int) def->params[i].len, def->params[i].name);
	}
	fprintf(f, "]");
	fprintln(f, indent + INDENT_SH, ",");
	fprintf(f, " \"blk\": ");
	xl_dump_block(f, indent + INDENT_SH, &def->blk);
	fprintln(f, indent + INDENT_SH, ",");
	fprintf(f, "\"retval\": ");
	xl_dump_intexpr(f, indent + INDENT_SH, def->retval);
	fprintln(f, indent + INDENT_SH, "}");
}

void xl_dump_block(FILE *f, int indent, struct xallang_block *blk)
{
	fprintf(f, "[ ");
	for (ssize_t i = 0; i < buf_len(blk->stmts); i++) {
		if (i) fprintln(f, indent + INDENT_SH ,",");
		xl_dump_statement(f, indent + INDENT_SH, blk->stmts + i);
	}
	fprintf(f, "]");
}

void xl_dump_statement(FILE *f, int indent, struct xallang_statement *stmt)
{
	fprintln(f, indent + INDENT_SH, "{");
	fprintf(f, "\"kind\": %d", stmt->kind);
	fprintln(f, indent + INDENT_SH, ",");
	switch (stmt->kind) {
	case XALLANG_SET:
		fprintf(f, "\"id\": \"%.*s\", ", (int) stmt->xset.id.len, stmt->xset.id.name);
		fprintf(f, "\"val\": ");
		xl_dump_intexpr(f, indent, stmt->xset.val);
		break;
	case XALLANG_IF:
		fprintf(f, "\"cond\": ");
		xl_dump_boolexpr(f, indent, stmt->xif.cond);
		fprintln(f, indent + INDENT_SH, ",");
		fprintf(f, "\"thenb\": ");
		xl_dump_block(f, indent + INDENT_SH, &stmt->xif.thenb);
		fprintln(f, indent + INDENT_SH, ",");
		fprintf(f, "\"elseb\": ");
		xl_dump_block(f, indent + INDENT_SH, &stmt->xif.elseb);
		break;
	case XALLANG_WHILE:
		fprintf(f, "\"cond\": ");
		xl_dump_boolexpr(f, indent + INDENT_SH, stmt->xwhile.cond);
		fprintln(f, indent + INDENT_SH, ",");
		fprintf(f, "\"body\": ");
		xl_dump_block(f, indent + INDENT_SH, &stmt->xwhile.body);
		break;
	default:
		assert(0);
		break;
	}
	fprintln(f, indent, "}");
}

void xl_dump_intexpr(FILE *f, int indent, struct xallang_intexpression *iexpr)
{
	fprintln(f, indent + INDENT_SH, "{");
	fprintf(f, "\"kind\": %d", iexpr->kind);
	fprintln(f, indent + INDENT_SH, ",");
	switch (iexpr->kind) {
	case XALLANG_SUM:
		fprintf(f, "\"lhs\": ");
		xl_dump_intexpr(f, indent + INDENT_SH, iexpr->lhs);
		fprintln(f, indent + INDENT_SH, ",");
		fprintf(f, "\"rhs\": ");
		xl_dump_intexpr(f, indent + INDENT_SH, iexpr->rhs);
		break;
	case XALLANG_INT:
		fprintf(f, "\"xint\": %llu", (unsigned long long) iexpr->xint);
		break;
	case XALLANG_ID:
		fprintf(f, "\"xid\": \"%.*s\"", (int) iexpr->xid.len, iexpr->xid.name);
		break;
	default:
		assert(0);
		break;
	}
	fprintln(f, indent, "}");
}

void xl_dump_boolexpr(FILE *f, int indent, struct xallang_boolexpression *bexpr)
{
	fprintln(f, indent + INDENT_SH, "{");
	fprintf(f, "\"kind\": %d", bexpr->kind);
	fprintln(f, indent + INDENT_SH, ",");
	switch (bexpr->kind) {
	case XALLANG_NOT:
		fprintf(f, "\"xbool\": ");
		xl_dump_boolexpr(f, indent + INDENT_SH, bexpr->xbool);
		break;
	case XALLANG_LT:
		fprintf(f, "\"lhs\": ");
		xl_dump_intexpr(f, indent + INDENT_SH, bexpr->lhs);
		fprintln(f, indent + INDENT_SH, ",");
		fprintf(f, "\"rhs\": ");
		xl_dump_intexpr(f, indent + INDENT_SH, bexpr->rhs);
		break;
	case XALLANG_EQ:
		fprintf(f, "\"lhs\": ");
		xl_dump_intexpr(f, indent + INDENT_SH, bexpr->lhs);
		fprintln(f, indent + INDENT_SH, ",");
		fprintf(f, "\"rhs\": ");
		xl_dump_intexpr(f, indent + INDENT_SH, bexpr->rhs);
		break;
	default:
		assert(0);
		break;
	}
}

