#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "x86_64.h"
#include "utils.h"


// increasing "cost"
enum ax64_reg {
	AX64_RDI,
	AX64_RSI,
	AX64_RDX,
	AX64_RCX,
	AX64_R9,
	AX64_R8,
	// retval
	AX64_RAX,
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

static const byte_t reg2bin[] = {
	[AX64_RDI] = 0b111,
	[AX64_RSI] = 0b110,
	[AX64_RDX] = 0b010,
	[AX64_RCX] = 0b001,
	[AX64_R9 ] = 0b001,
	[AX64_R8 ] = 0b000,
	[AX64_RAX] = 0b000,
	[AX64_RBX] = 0b011,
	[AX64_RBP] = 0b101,
	[AX64_RSP] = 0b100,
	[AX64_R10] = 0b010,
	[AX64_R11] = 0b011,
	[AX64_R12] = 0b100,
	[AX64_R13] = 0b101,
	[AX64_R14] = 0b110,
	[AX64_R15] = 0b111,
};

static const char *reg2str[] = {
	[AX64_RDI] = "%rdi",
	[AX64_RSI] = "%rsi",
	[AX64_RDX] = "%rdx",
	[AX64_RCX] = "%rcx",
	[AX64_R9 ] = "%r9 ",
	[AX64_R8 ] = "%r8 ",
	[AX64_RAX] = "%rax",
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

const char *label_str(ssize_t id)
{
	static char str[32] = ".L";
	sprintf(str + 2, "%zd", id);
	return str;
}

void ax64_gen_statement(FILE *f, struct ir_definition *def, struct ir_statement *stmt)
{
	for (ssize_t i = 0; i < buf_len(def->labels); i++) {
		struct ir_statement *s = def->stmts + def->labels[i];
		if (s == stmt) {
			fprintf(f, "%s:\n", label_str(def->labels[i]));
		}
	}
	if (stmt->instr == IRINSTR_LOCAL) return;
	fprintf(f, "\t");
	switch (stmt->instr) {
	case IRINSTR_SET:
		fprintf(f, "mov ");
		ax64_gen_operand(f, def, stmt->ops + 1); // src
		fprintf(f, ", ");
		ax64_gen_operand(f, def, stmt->ops + 0); // dst
		break;
	case IRINSTR_RET:
		fprintf(f, "mov ");
		ax64_gen_operand(f, def, stmt->ops + 0);
		fprintf(f, ", ");
		fprintf(f, "%.*s", (int) ax64_retreg.len, ax64_retreg.name);
		fprintf(f, "\n\t");
		fprintf(f, "ret");
		break;
	case IRINSTR_LOCAL:
		assert(0);
		return;
	case IRINSTR_ADD:
		if (id_cmp(stmt->ops[0].oid, stmt->ops[1].oid) != 0) {
			fprintf(f, "mov ");
			ax64_gen_operand(f, def, stmt->ops + 1);
			fprintf(f, ", ");
			ax64_gen_operand(f, def, stmt->ops + 0);
			fprintf(f, "\n\t");
		}
		fprintf(f, "add ");
		ax64_gen_operand(f, def, stmt->ops + 2);
		fprintf(f, ", ");
		ax64_gen_operand(f, def, stmt->ops + 0);
		break;
	case IRINSTR_JMP:
		fprintf(f, "jmp ");
		ax64_gen_operand(f, def, stmt->ops + 0);
		break;
	case IRINSTR_CMP:
		fprintf(f, "cmp ");
		if (stmt->ops[0].kind == IR_HEX) {
			ax64_gen_operand(f, def, stmt->ops + 0);
			fprintf(f, ", ");
			ax64_gen_operand(f, def, stmt->ops + 1);
		} else {
			ax64_gen_operand(f, def, stmt->ops + 1);
			fprintf(f, ", ");
			ax64_gen_operand(f, def, stmt->ops + 0);
		}
		break;
	case IRINSTR_JZ:
		fprintf(f, "je ");
		ax64_gen_operand(f, def, stmt->ops + 0);
		break;
	case IRINSTR_JNZ:
		fprintf(f, "jne ");
		ax64_gen_operand(f, def, stmt->ops + 0);
		break;
	case IRINSTR_JL:
		fprintf(f, "jl ");
		ax64_gen_operand(f, def, stmt->ops + 0);
		break;
	case IRINSTR_JGE:
		fprintf(f, "jge ");
		ax64_gen_operand(f, def, stmt->ops + 0);
		break;
	default:
		assert(0);
	}
	fprintf(f, "\n");
}

static const char *local2str(struct ir_definition *def, struct identifier *id)
{
	struct identifier *match = id_find(def->params, *id);
	if (match)
		return reg2str[match - def->params + AX64_ARG0];
	match = id_find(def->locals, *id);
	if (match)
		return reg2str[match - def->locals + buf_len(def->params)];
	fprintf(stderr, "tried finding id \"%.*s\"\n", (int) id->len, id->name);
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
	case IR_LABEL:
		fputs(label_str(op->olbl), f);
		break;
	default:
		assert(0);
	}
}

enum { PAGE = 4096 };
__attribute__((malloc, aligned(PAGE), returns_nonnull))
void *page_aligned_memory(void)
{
	void *p = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE, dev_zero, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	return p;
}

void unmap_memory(void *m)
{
	munmap(m, PAGE);
}

generic_fp *ax64_bin_program(struct ir_program *pgrm)
{
	generic_fp *defs = NULL;
	for (ssize_t i = 0; i < buf_len(pgrm->defs); i++) {
		buf_push(defs, ax64_bin_definition(pgrm->defs + i));
	}
	return defs;
}

static byte_t *cursor;

generic_fp ax64_bin_definition(struct ir_definition *def)
{
	byte_t *start = page_aligned_memory();
	cursor = start;
	for (ssize_t i = 0; i < buf_len(def->stmts); i++) {
		ax64_bin_statement(def, def->stmts + i);
	}
	mprotect(start, PAGE, PROT_READ | PROT_EXEC);
	return (generic_fp) start;
}

void ax64_instr_ret(void)
{
	*cursor++ = 0xC3;
}

void ax64_instr_mov_imm(int reg, uint64_t imm)
{
	if (imm >> 32)
		*cursor++ = 0x48;
	*cursor++ = 0xB8 | reg;

	*cursor++ = imm >>  0 & 0xFF;
	*cursor++ = imm >>  8 & 0xFF;
	*cursor++ = imm >> 16 & 0xFF;
	*cursor++ = imm >> 32 & 0xFF;
	if ((imm >>= 32) == 0) return;
	*cursor++ = imm >>  0 & 0xFF;
	*cursor++ = imm >>  8 & 0xFF;
	*cursor++ = imm >> 16 & 0xFF;
	*cursor++ = imm >> 32 & 0xFF;
}

void ax64_instr_mov_reg(int rd, int rs)
{

}

void ax64_bin_statement(struct ir_definition *def, struct ir_statement *stmt)
{
	ssize_t idx;
	struct identifier *match;
	switch (stmt->instr) {
	case IRINSTR_SET:
		match = id_find(def->params, stmt->ops[0].oid);
		if (match) idx = match - def->params + AX64_ARG0;
		else match = id_find(def->locals, stmt->ops[0].oid);
		assert(match);
		idx = match - def->locals + buf_len(def->params);
		if (stmt->ops[1].kind == IR_HEX) {
			ax64_instr_mov_imm(reg2bin[idx], stmt->ops[1].oint);
		} else if (stmt->ops[1].kind == IR_VAR) {
			ssize_t idx2;
			match = id_find(def->params, stmt->ops[1].oid);
			if (match) idx2 = match - def->params + AX64_ARG0;
			else match = id_find(def->locals, stmt->ops[1].oid);
			assert(match);
			idx2 = match - def->locals + buf_len(def->params);
			ax64_instr_mov_reg(reg2bin[idx], reg2bin[idx2]);
		} else {
			assert(0);
		}
		break;
	case IRINSTR_RET:
		if (stmt->ops[0].kind == IR_HEX) {
			ax64_instr_mov_imm(reg2bin[AX64_RETVAL], stmt->ops[0].oint);
		}
		ax64_instr_ret();
		break;
	case IRINSTR_LOCAL:
		break;
	case IRINSTR_ADD:
		break;
	case IRINSTR_CMP:
		break;
	case IRINSTR_JMP:
		break;
	case IRINSTR_JZ:
		break;
	case IRINSTR_JNZ:
		break;
	case IRINSTR_JL:
		break;
	case IRINSTR_JGE:
		break;
	default:
		assert(0);
	}
}

void ax64_bin_operand(struct ir_definition *def, struct ir_operand *op)
{
	(void) cursor;
	(void) op;
}

