#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "x86_64.h"
#include "utils.h"


// increasing "cost"
enum ax64_reg {
	AX64_RAX,
	AX64_RDI,
	AX64_RSI,
	AX64_RDX,
	AX64_RCX,
	AX64_R9,
	AX64_R8,
	// saved regs
	AX64_RBX,
	AX64_RBP,
	AX64_RSP,
	AX64_R10,
	AX64_R11,
	AX64_R12,
	AX64_R13,
	AX64_R14,
	AX64_R15,
	AX64_R16,
	// aliases
	AX64_RETVAL = AX64_RAX,
	AX64_ARG0 = AX64_RDI,
	AX64_ARG1 = AX64_RSI,
	AX64_ARG2 = AX64_RDX,
	AX64_ARG3 = AX64_RCX,
	AX64_ARG4 = AX64_R9,
	AX64_ARG5 = AX64_R8,
	AX64_REG_ARG_END = AX64_ARG5,
};

static const char *reg2str[] = {
	[AX64_RAX] = "%rax",
	[AX64_RDI] = "%rdi",
	[AX64_RSI] = "%rsi",
	[AX64_RDX] = "%rdx",
	[AX64_RCX] = "%rcx",
	[AX64_RBX] = "%rbx",
	[AX64_RBP] = "%rbp",
	[AX64_RSP] = "%rsp",
	[AX64_R10] = "%r10",
	[AX64_R11] = "%r11",
	[AX64_R12] = "%r12",
	[AX64_R13] = "%r13",
	[AX64_R14] = "%r14",
	[AX64_R15] = "%r15",
	[AX64_R16] = "%r16",
};

static const struct identifier ax64_retreg = { .len = 4, .name = "%rax", };

void ax64_gen_program(FILE *f, struct ir_program *pgrm)
{
	assert(buf_len(pgrm->defs));

	for (ssize_t i = 0; i < buf_len(pgrm->defs); i++) {
		struct identifier *id = &pgrm->defs[i].name;
		fprintf(f, ".globl %.*s\n", (int) id->len, id->name);
	}
	fputs("\n\n", f);

	for (ssize_t i = 0; i < buf_len(pgrm->defs); i++) {
		struct ir_definition *def = pgrm->defs + i;
		fprintf(f, "%.*s:\n", (int) def->name.len, def->name.name);
		for (ssize_t s = 0; s < buf_len(def->stmts); s++) {
			ax64_gen_statement(f, def, def->stmts + s);
		}
		fprintf(f, "\n\n");
	}
}

void ax64_gen_statement_instr(FILE *f, struct ir_definition *def, struct ir_statement *stmt)
{
	switch (stmt->instr) {
	case IRINSTR_SET:
		fprintf(f, "\tmov ");
		ax64_gen_operand(f, def, stmt->ops + 1); // src
		fprintf(f, ", ");
		ax64_gen_operand(f, def, stmt->ops + 0); // dst
		break;
	case IRINSTR_RET:
		fprintf(f, "\tmov ");
		ax64_gen_operand(f, def, stmt->ops + 0);
		fprintf(f, ", ");
		fprintf(f, "%.*s", (int) ax64_retreg.len, ax64_retreg.name);
		fprintf(f, "\n");
		fprintf(f, "\tret");
		break;
	case IRINSTR_LOCAL:
		return;
	case IRINSTR_ADD:
		if (stmt->ops[0].oid.len == stmt->ops[1].oid.len && strncmp(stmt->ops[0].oid.name, stmt->ops[1].oid.name, stmt->ops[0].oid.len) == 0) {
			fprintf(f, "\tadd ");
			ax64_gen_operand(f, def, stmt->ops + 2);
			fprintf(f, ", ");
			ax64_gen_operand(f, def, stmt->ops + 0);
		} else {
			fprintf(f, "\tlea ");
			fprintf(f, "(");
			ax64_gen_operand(f, def, stmt->ops + 2);
			fprintf(f, ", ");
			ax64_gen_operand(f, def, stmt->ops + 1);
			fprintf(f, "), ");
			ax64_gen_operand(f, def, stmt->ops + 0);
		}
		break;
	case IRINSTR_JMP:
		fprintf(f, "\tjmp ");
		fprintf(f, "%.*s", (int) stmt->ops[0].oid.len, stmt->ops[0].oid.name);
		break;
	default:
		assert(0);
	}
	fprintf(f, "\n");
}

void ax64_gen_statement(FILE *f, struct ir_definition *def, struct ir_statement *stmt)
{
	switch (stmt->kind) {
	case IR_LABELED:
		fprintf(f, "%.*s:\n", (int) stmt->lbl.len, stmt->lbl.name);
		break;
	case IR_INSTR:
		ax64_gen_statement_instr(f, def, stmt);
		break;
	default:
		assert(0);
	}
}

static const char *local2str(struct ir_definition *def, struct identifier *id)
{
	for (struct identifier *start = def->locals, *end = start + buf_len(def->locals), *cur = start;
			cur != end; cur++) {
		if (cur->len == id->len && strncmp(cur->name, id->name, cur->len) == 0)
			return reg2str[cur - start];
	}
	assert(0);
}

void ax64_gen_operand(FILE *f, struct ir_definition *def, struct ir_operand *op)
{
	switch (op->kind) {
	case IR_HEX:
		fprintf(f, "$0x%" PRIx64, op->oint);
		break;
	case IR_VAR:
		fprintf(f, "%s", local2str(def, &op->oid));
		break;
	default:
		assert(0);
	}
}

