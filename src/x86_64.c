#include <assert.h>
#include <unistd.h>
#include <stdio.h>

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

void ax64_gen_program(FILE *f, struct ir_program *pgrm)
{
	assert(buf_len(pgrm->defs));

	for (ssize_t i = 0; i < buf_len(pgrm->defs); i++) {
		struct identifier *id = pgrm->defs + i;
		fprintf(f, ".globl %.*s\n", (int) id->len, id->name);
	}
	fputs("\n\n", f);
}

void ax64_gen_statement(FILE *f, struct ir_statement *stmt)
{
}

void ax64_gen_operand(FILE *f, struct ir_operand *op)
{
}

