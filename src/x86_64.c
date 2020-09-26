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
	[AX64_RDI] = 0x7,
	[AX64_RSI] = 0x6,
	[AX64_RDX] = 0x2,
	[AX64_RCX] = 0x1,
	[AX64_R9 ] = 0x9,
	[AX64_R8 ] = 0x8,
	[AX64_RAX] = 0x0,
	[AX64_RBX] = 0x3,
	[AX64_RBP] = 0x5,
	[AX64_RSP] = 0x4,
	[AX64_R10] = 0xA,
	[AX64_R11] = 0xB,
	[AX64_R12] = 0xC,
	[AX64_R13] = 0xD,
	[AX64_R14] = 0xE,
	[AX64_R15] = 0xF,
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

enum ax64_cc {
	AX64_O = 0,
	AX64_NO,
	AX64_B, AX64_NAE = AX64_B,
	AX64_NB, AX64_AE = AX64_NB,
	AX64_E, AX64_Z = AX64_E,
	AX64_NE, AX64_NZ = AX64_NE,
	AX64_BE, AX64_NA = AX64_BE,
	AX64_NBE, AX64_A = AX64_NBE,
	AX64_S,
	AX64_NS,
	AX64_P, AX64_PE = AX64_P,
	AX64_NP, AX64_PO = AX64_NP,
	AX64_L, AX64_NGE = AX64_L,
	AX64_NL, AX64_GE = AX64_NL,
	AX64_LE, AX64_NG = AX64_LE,
	AX64_NLE, AX64_G = AX64_NLE,
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

static FILE *out;
static byte_t *cursor;

generic_fp *ax64_bin_program(struct ir_program *pgrm, ssize_t **lengths)
{
	generic_fp *defs = NULL;
	out = stderr;
	for (ssize_t i = 0; i < buf_len(pgrm->defs); i++) {
		buf_push(defs, ax64_bin_definition(pgrm->defs + i));
		buf_push(*lengths, cursor - (byte_t *) defs[i]);
	}
	return defs;
}

static struct reference_t {
	byte_t *pos;
	ssize_t idx;
} *references;
static byte_t **instructions;

generic_fp ax64_bin_definition(struct ir_definition *def)
{
	byte_t *start = page_aligned_memory();
	cursor = start;
	references = NULL;
	instructions = NULL;
	for (ssize_t i = 0; i < buf_len(def->stmts); i++) {
		ax64_bin_statement(def, def->stmts + i);
	}
	ax64_bin_patch(def);

	for (ssize_t i = 0; i < buf_len(instructions)-1; i++) {
		for (byte_t *instr = instructions[i]; instr < cursor && instr < instructions[i + 1]; instr++) {
			fprintf(out, "%02X ", *instr);
		}
		fprintf(out, "\n");
	}
	fprintf(out, "%02X", cursor[-1]);
	fprintf(out, "\n");
	fprintf(out, "\n");

	mprotect(start, PAGE, PROT_READ | PROT_EXEC);
	return (generic_fp) start;
}

static void emit(uint8_t byte)
{
	*cursor++ = byte;
}

static void emit4(uint32_t dword)
{
	emit(dword >>  0);
	emit(dword >>  8);
	emit(dword >> 16);
	emit(dword >> 24);
}

static void emit8(uint64_t qword)
{
	emit4(qword >>  0);
	emit4(qword >> 32);
}

void ax64_instr_ret(void)
{
	buf_push(instructions, cursor);
	emit(0xC3);
}

void ax64_instr_mov_imm(int reg, uint64_t imm)
{
	buf_push(instructions, cursor);
	emit(0x40 | (1 << 3) | (reg >> 3));
	emit(0xB8 | (reg & 0b111));
	emit8(imm);
}

void ax64_instr_mov_reg(int rd, int rs)
{
	buf_push(instructions, cursor);
	emit(0x40 | (1 << 3) | (rs >> 3 << 2) | (rd >> 3));
	emit(0x89);
	emit(0xC0 | ((rs & 0b111) << 3) | (rd & 0b111));
}

void ax64_instr_add_reg(int rd, int rs)
{
	buf_push(instructions, cursor);
	emit(0x40 | (1 << 3) | (rs >> 3 << 2) | (rd >> 3));
	emit(0x01);
	emit(0xC0 | ((rs & 0b111) << 3) | (rd & 0b111));
}

void ax64_instr_add_imm(int rd, uint32_t imm)
{
	buf_push(instructions, cursor);
	emit(0x40 | (1 << 3) | (rd >> 3));
	emit(0x81);
	emit(0xC0 | 0b000 << 3 | (rd & 0b111));
	emit4(imm);
}

void ax64_instr_jmp_imm(ssize_t ir_idx)
{
	buf_push(instructions, cursor);
	emit(0xEB);
	buf_push(references, (struct reference_t){ cursor, ir_idx });
	emit(0x00);
}

void ax64_instr_cmp_imm(int reg, uint32_t imm)
{
	buf_push(instructions, cursor);
	emit(0x40 | 0x08 | (reg >> 3));
	emit(0x81);
	emit(0xC0 | 0b111 << 3 | (reg & 0b111));
	emit4(imm);
}

void ax64_instr_cmp_reg(int rd, int rs)
{
	buf_push(instructions, cursor);
	emit(0x40 | 0x08 | (rs >> 3 << 2) | (rd >> 3));
	emit(0x3B);
	emit(0xC0 | ((rs & 0b111) << 3) | (rd & 0b111));
}

void ax64_instr_jcc_imm(enum ax64_cc cc, ssize_t ir_idx)
{
	buf_push(instructions, cursor);
	emit(0x70 | cc);
	buf_push(references, (struct reference_t){ cursor, ir_idx });
	emit(0x00);
}

static int id2reg(struct ir_definition *def, struct identifier id)
{
	ssize_t idx;
	struct identifier *match = id_find(def->params, id);
	if (match) {
		idx = match - def->params + AX64_ARG0;
	} else {
		match = id_find(def->locals, id);
		assert(match);
		idx = match - def->locals + buf_len(def->params);
	}
	return reg2bin[idx];
}

void ax64_bin_statement(struct ir_definition *def, struct ir_statement *stmt)
{
	int reg1, reg2;
	switch (stmt->instr) {
	case IRINSTR_SET:
		reg1 = id2reg(def, stmt->ops[0].oid);
		if (stmt->ops[1].kind == IR_HEX) {
			ax64_instr_mov_imm(reg1, stmt->ops[1].oint);
		} else if (stmt->ops[1].kind == IR_VAR) {
			reg2 = id2reg(def, stmt->ops[1].oid);
			ax64_instr_mov_reg(reg1, reg2);
		} else {
			assert(0);
		}
		break;
	case IRINSTR_RET:
		if (stmt->ops[0].kind == IR_HEX) {
			ax64_instr_mov_imm(reg2bin[AX64_RETVAL], stmt->ops[0].oint);
		} else if (stmt->ops[0].kind == IR_VAR) {
			ax64_instr_mov_reg(reg2bin[AX64_RETVAL], id2reg(def, stmt->ops[0].oid));
		} else {
			assert(0);
		}
		ax64_instr_ret();
		break;
	case IRINSTR_LOCAL:
		break;
	case IRINSTR_ADD:
		assert(id_cmp(stmt->ops[0].oid, stmt->ops[1].oid) != 0 &&
				id_cmp(stmt->ops[0].oid, stmt->ops[2].oid) != 0);
		reg1 = id2reg(def, stmt->ops[0].oid);
		if (stmt->ops[1].kind == IR_HEX) {
			ax64_instr_mov_imm(reg1, stmt->ops[1].oint);
		} else if (stmt->ops[1].kind == IR_VAR) {
			reg2 = id2reg(def, stmt->ops[1].oid);
			ax64_instr_mov_reg(reg1, reg2);
		} else {
			assert(0);
		}
		if (stmt->ops[2].kind == IR_HEX) {
			ax64_instr_add_imm(reg1, stmt->ops[2].oint);
		} else if (stmt->ops[2].kind == IR_VAR) {
			reg2 = id2reg(def, stmt->ops[2].oid);
			ax64_instr_add_reg(reg1, reg2);
		} else {
			assert(0);
		}
		break;
	case IRINSTR_CMP:
		reg1 = id2reg(def, stmt->ops[0].oid);
		if (stmt->ops[1].kind == IR_HEX) {
			ax64_instr_cmp_imm(reg1, stmt->ops[1].oint);
		} else if (stmt->ops[1].kind == IR_VAR) {
			reg2 = id2reg(def, stmt->ops[1].oid);
			ax64_instr_cmp_reg(reg1, reg2);
		} else {
			assert(0);
		}
		break;
	case IRINSTR_JMP:
		assert(stmt->ops[0].kind == IR_LABEL);
		ax64_instr_jmp_imm(stmt->ops[0].olbl);
		break;
	case IRINSTR_JZ:
		assert(stmt->ops[0].kind == IR_LABEL);
		ax64_instr_jcc_imm(AX64_Z, stmt->ops[0].olbl);
		break;
	case IRINSTR_JNZ:
		assert(stmt->ops[0].kind == IR_LABEL);
		ax64_instr_jcc_imm(AX64_NZ, stmt->ops[0].olbl);
		break;
	case IRINSTR_JL:
		assert(stmt->ops[0].kind == IR_LABEL);
		ax64_instr_jcc_imm(AX64_L, stmt->ops[0].olbl);
		break;
	case IRINSTR_JGE:
		assert(stmt->ops[0].kind == IR_LABEL);
		ax64_instr_jcc_imm(AX64_GE, stmt->ops[0].olbl);
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

void ax64_bin_patch(struct ir_definition *def)
{
	for (ssize_t i = 0; i < buf_len(references); i++) {
		*references[i].pos = instructions[ references[i].idx ] - references[i].pos - 1;
	}
}


