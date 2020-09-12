#ifndef IR_H
#define IR_H

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "xallang.h"


enum ir_type {
	IR_UNK,
	IR_LABELED, IR_INSTR,
	IR_HEX, IR_VAR,
	IRINSTR_SET, IRINSTR_RET, IRINSTR_LOCAL,
	IRINSTR_ADD, IRINSTR_JMP,
};

struct ir_program {
	struct ir_definition *defs;
};

struct ir_definition {
	struct identifier name;
	struct ir_statement *stmts;
	struct identifier *locals;
};

struct ir_statement {
	enum ir_type kind;
	union {
		struct identifier lbl;
		struct { enum ir_type instr; struct ir_operand *ops; };
	};
};

struct ir_operand {
	enum ir_type kind;
	union {
		uint64_t oint;
		struct identifier oid;
	};
};

void ir_parse_program(struct ir_program *pgrm);
void ir_parse_definition(struct ir_definition *def);
void ir_parse_statement(struct ir_definition *def, struct ir_statement *stmt);
void ir_parse_operand(struct ir_operand *op);
enum ir_type ir_parse_instr(ssize_t *n);
uint64_t ir_parse_integer(void);

void ir_gen_program(FILE *f, struct xallang_program *pgrm);
void ir_gen_definition(FILE *f, struct xallang_definition *def);
void ir_gen_statement(FILE *f, struct xallang_statement *stmt);
void ir_gen_intexpr(FILE *f, struct xallang_intexpression *iexpr);
void ir_gen_boolexpr(FILE *f, struct xallang_boolexpression *bexpr);
void ir_gen_ident(FILE *f, struct identifier id);

void ir_dump_program(FILE *f, int indent, struct ir_program *pgrm);
void ir_dump_definition(FILE *f, int indent, struct ir_definition *def);
void ir_dump_statement(FILE *f, int indent, struct ir_statement *stmt);
void ir_dump_operand(FILE *f, int indent, struct ir_operand *op);

#endif /* IR_H */
