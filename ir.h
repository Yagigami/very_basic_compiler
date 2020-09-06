#ifndef IR_H
#define IR_H

#include <stdint.h>

#include "xallang.h"


enum ir_type {
	IR_LABELED, IR_INSTR,
	IR_HEX, IR_VAR,
	IRINSTR_SET, IRINSTR_RET, IRINSTR_LOCAL,
};

struct ir_program {
	struct ir_statement *stmts;
};

struct ir_statement {
	enum ir_type kind;
	union {
		struct { struct identifier lbl; struct ir_statement *inner; };
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
void ir_parse_statement(struct ir_statement *stmt);
void ir_parse_operand(struct ir_operand *op);
enum ir_type ir_parse_instr(void);
uint64_t ir_parse_integer(void);

#endif /* IR_H */
