#include "ir.h"
#include "utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>


void ir_parse_program(struct ir_program *pgrm)
{
	while (skip_whitespace(), *stream) {
		struct ir_definition def = {0};
		ir_parse_definition(&def);
		buf_push(pgrm->defs, def);
	}
}

void ir_parse_definition(struct ir_definition *def)
{
	def->name = parse_cidentifier();
	skip_whitespace();
	if (*stream++ != '(') error("expected '(' after function name");
	skip_whitespace();
	if (*stream++ != ')') error("no matching closing ')'");
	while (skip_whitespace(), *stream != '$') {
		struct ir_statement stmt = {0};
		ir_parse_statement(def, &stmt);
		buf_push(def->stmts, stmt);
	}
	stream++;
}

void ir_parse_statement(struct ir_definition *def, struct ir_statement *stmt)
{
	if (skip_whitespace(), *stream != ':') {
		ssize_t n;
		stmt->instr = ir_parse_instr(&n);
		stmt->kind = IR_INSTR;
		for (ssize_t i = 0; skip_whitespace(), *stream != '$'; i++) {
			if (i) {
				if (*stream++ != ',') error("expected comma separating instruction operands");
			}
			struct ir_operand op = {0};
			ir_parse_operand(&op);
			buf_push(stmt->ops, op);
		}
		stream++;
		if (buf_len(stmt->ops) != n) error("expected %zd operands but got %zd", n, buf_len(stmt->ops));

		if (stmt->instr == IRINSTR_LOCAL)
			buf_push(def->locals, stmt->ops[1].oid);
	} else {
		stream++;
		stmt->kind = IR_LABELED;
		stmt->lbl = parse_cidentifier();
		skip_whitespace();
	}
}

void ir_parse_operand(struct ir_operand *op)
{
	skip_whitespace();
	if (isdigit(*stream)) {
		op->kind = IR_HEX;
		op->oint = ir_parse_integer();
	} else if (isalpha(*stream) || *stream == '|') {
		op->kind = IR_VAR;
		op->oid = parse_cidentifier();
	} else {
		assert(0);
	}
}

enum ir_type ir_parse_instr(ssize_t *n)
{
	skip_whitespace();
	if (MATCH_KW("set")) return *n = 2, IRINSTR_SET;
	if (MATCH_KW("ret")) return *n = 1, IRINSTR_RET;
	if (MATCH_KW("local")) return *n = 1, IRINSTR_LOCAL;
	return IR_UNK;
}

uint64_t ir_parse_integer(void)
{
	uint64_t i = 0;
	skip_whitespace();
	while (isxdigit(*stream)) i = i * 16 + *stream++ - '0';
	return i;
}

void ir_dump_program(FILE *f, int indent, struct ir_program *pgrm);
void ir_dump_definition(FILE *f, int indent, struct ir_definition *def);
void ir_dump_statement(FILE *f, int indent, struct ir_statement *stmt);
void ir_dump_operand(FILE *f, int indent, struct ir_operand *op);

