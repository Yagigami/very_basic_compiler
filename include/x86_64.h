#ifndef ARCH_X86_64_H
#define ARCH_X86_64_H

#include <stdio.h>

#include "ir.h"


void ax64_gen_program(FILE *f, struct ir_program *pgrm);
void ax64_gen_statement(FILE *f, struct ir_statement *stmt);
void ax64_gen_operand(FILE *f, struct ir_operand *op);

#endif /* ARCH_X86_64_H */

