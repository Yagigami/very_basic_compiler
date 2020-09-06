#include <string.h>
#include <ctype.h>
#include <stdnoreturn.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "utils.h"
#include "xallang.h"


#define CMP(s1, s2) strncmp((s1), (s2), sizeof (s1))

#define MATCH_KW(kw) (CMP((kw), stream) == 0 && (stream += sizeof (kw), 1))

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

void xl_parse_program(struct xallang_program *pgrm)
{
	skip_whitespace();
	while (*stream == '(') {
		stream++;
		skip_whitespace();
		if (MATCH_KW("def")) {
			struct xallang_definition def;
			xl_parse_definition(&def);
			buf_push(pgrm->defs, def);
		} else {
			error("expected 'def' keyword");
		}
	}
}

void xl_parse_definition(struct xallang_definition *def)
{
	def->name = parse_cidentifier();
	skip_whitespace();
	if (*stream++ != '(') error("expected a '(' before argument list in function definition");
	while (skip_whitespace(), *stream != ')') {
		buf_push(def->params, parse_cidentifier());
	}
	xl_parse_block(&def->blk);
	def->retval = xl_parse_intexpr();
}

void xl_parse_block(struct xallang_block *blk)
{
	while (skip_whitespace(), *stream == '(') {
		struct xallang_statement stmt;
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

