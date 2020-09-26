#ifndef ARCH_X86_64_H
#define ARCH_X86_64_H

#include <stdio.h>

#include "ir.h"
#include "utils.h"


void ax64_gen_program(FILE *f, struct ir_program *pgrm);
void ax64_gen_statement(FILE *f, struct ir_definition *def, struct ir_statement *stmt);
void ax64_gen_operand(FILE *f, struct ir_definition *def, struct ir_operand *op);

typedef void (*generic_fp) (void);
typedef unsigned char byte_t;

generic_fp *ax64_bin_program(struct ir_program *pgrm, ssize_t **lengths);
generic_fp ax64_bin_definition(struct ir_definition *def);
void ax64_bin_statement(struct ir_definition *def, struct ir_statement *stmt);
void ax64_bin_operand(struct ir_definition *def, struct ir_operand *op);
void ax64_bin_patch(struct ir_definition *def);

#endif /* ARCH_X86_64_H */

