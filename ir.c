#include "ir.h"
#include "utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


void ir_parse_program(struct ir_program *pgrm)
{
	while (skip_whitespace(), *stream) {
		struct ir_statement stmt = {0};
		ir_parse_statement(&stmt);
		buf_push(pgrm->stmts, stmt);
	}
}

void ir_parse_statement(struct ir_statement *stmt)
{
	if (*stream == ':') {
		stream++;
		stmt->kind = IR_LABELED;
		stmt->lbl = parse_cidentifier();
		struct ir_statement *inner = xcalloc(1, sizeof *inner);
		ir_parse_statement(inner);
		stmt->inner = inner;
		stmt = inner;
	}

	stmt->kind = IR_INSTR;
	stmt->instr = ir_parse_instr();
	
	while (skip_whitespace(), *stream == '$') {
		stream++;
		struct ir_operand op = {0};
		ir_parse_operand(&op);
		buf_push(stmt->ops, op);
	}
}

enum ir_type ir_parse_instr(void)
{
	skip_whitespace();
	if (MATCH_KW("set")) return IRINSTR_SET;
	if (MATCH_KW("ret")) return IRINSTR_RET;
	if (MATCH_KW("local")) return IRINSTR_LOCAL;
	assert(0);
}

void ir_parse_operand(struct ir_operand *op)
{
	skip_whitespace();
	if (isdigit(*stream)) {
		op->kind = IR_HEX;
		op->oint = ir_parse_integer();
	} else if (isalpha(*stream)) {
		op->kind = IR_VAR;
		op->oid = parse_cidentifier();
	} else {
		assert(0);
	}
}

int hex2i(char c)
{
	if (isdigit(c)) return c - '0';
	assert(isxdigit(c));
	return tolower(c) - 'a';
}

uint64_t ir_parse_integer(void)
{
	skip_whitespace();
	uint64_t i = 0;
	while (isdigit(*stream)) {
		i = i * 16 + hex2i(*stream++);
	}
	return i;
}
