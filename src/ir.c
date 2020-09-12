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
		struct ir_statement stmt = {0};
		ir_parse_statement(&stmt);
		buf_push(pgrm->stmts, stmt);
		while (stmt.kind == IR_LABELED) {
			buf_push(pgrm->globals, stmt.lbl);
			stmt = *stmt.inner;
		}
			
	}
}

void ir_parse_statement(struct ir_statement *stmt)
{
	while (*stream == ':') {
		stream++;
		stmt->kind = IR_LABELED;
		stmt->lbl = parse_cidentifier();
		struct ir_statement *inner = xcalloc(1, sizeof *inner);
		stmt->inner = inner;
		stmt = inner;
	}

	ssize_t n;
	stmt->kind = IR_INSTR;
	stmt->instr = ir_parse_instr(&n);
	
	goto first;
	while (skip_whitespace(), *stream == ',') {
		stream++;
first:
		skip_whitespace();
		if (*stream != '$') break;
		stream++;
		struct ir_operand op = {0};
		ir_parse_operand(&op);
		buf_push(stmt->ops, op);
	}

	assert(n == buf_len(stmt->ops) && "operands not matching with requirements");
}

enum ir_type ir_parse_instr(ssize_t *n)
{
	skip_whitespace();
	if (MATCH_KW("set")) return *n = 2, IRINSTR_SET;
	if (MATCH_KW("ret")) return *n = 1, IRINSTR_RET;
	if (MATCH_KW("local")) return *n = 1, IRINSTR_LOCAL;
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
	while (isxdigit(*stream)) {
		i = i * 16 + hex2i(*stream++);
	}
	return i;
}

void ir_dump_program(FILE *f, int indent, struct ir_program *pgrm)
{
	fprintln(f, indent + INDENT_SH, "{");
	fprintf(f, "\"stmts\": ");
	fprintln(f, indent + INDENT_SH, "[");
	for (ssize_t i = 0; i < buf_len(pgrm->stmts); i++) {
		if (i) fprintf(f, ", ");
		ir_dump_statement(f, indent + INDENT_SH, pgrm->stmts + i);
	}
	fprintln(f, indent, "]");
	fprintln(f, indent, "}");
}

void ir_dump_statement(FILE *f, int indent, struct ir_statement *stmt)
{
	fprintln(f, indent + INDENT_SH, "{");
	fprintf(f, "\"kind\": %d", stmt->kind);
	fprintln(f, indent + INDENT_SH, ",");
	if (stmt->kind == IR_LABELED) {
		fprintf(f, "\"lbl\": \"%.*s\"", (int) stmt->lbl.len, stmt->lbl.name);
		fprintln(f, indent + INDENT_SH, ",");
		fprintf(f, "\"inner\": ");
		ir_dump_statement(f, indent + INDENT_SH, stmt->inner);
	} else {
		assert(stmt->kind == IR_INSTR);
		fprintf(f, "\"instr\": %d", stmt->instr);
		fprintln(f, indent + INDENT_SH, ",");
		fprintf(f, "\"ops\": [");
		for (ssize_t i = 0; i < buf_len(stmt->ops); i++) {
			if (i) fprintf(f, ", ");
			ir_dump_operand(f, indent + INDENT_SH, stmt->ops + i);
		}
		fprintln(f, indent + INDENT_SH, "]");
	}
	fprintln(f, indent, "}");
}

void ir_dump_operand(FILE *f, int indent, struct ir_operand *op)
{
	(void) indent;
	if (op->kind == IR_HEX) {
		fprintf(f, "\"%" PRIx64 "\"", op->oint);
	} else {
		fprintf(f, "\"%.*s\"", (int) op->oid.len, op->oid.name);
	}
}

