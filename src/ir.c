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

struct identifier *idlist_find(struct identifier *ids, struct identifier id) {
	for (ssize_t i = 0; i < buf_len(ids); i++) {
		if (ids[i].len == id.len && strncmp(ids[i].name, id.name, id.len) == 0)
			return ids + i;
	}
	return NULL;
}

void ir_parse_statement(struct ir_definition *def, struct ir_statement *stmt)
{
	if (skip_whitespace(), *stream != ':') {
		ssize_t n;
		stmt->instr = ir_parse_instr(&n);
		stmt->kind = IR_INSTR;
		for (ssize_t i = 0; skip_whitespace_nonl(), *stream != '\n'; i++) {
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
			buf_push(def->locals, stmt->ops[0].oid);
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
	} else if (isalpha(*stream) || *stream == '_') {
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
	if (MATCH_KW("add")) return *n = 3, IRINSTR_ADD;
	if (MATCH_KW("jmp")) return *n = 1, IRINSTR_JMP;
	return IR_UNK;
}

int hex2int(int c)
{
	if (isdigit(c)) return c - '0';
	return tolower(c) - 'a' + 0xa;
}

uint64_t ir_parse_integer(void)
{
	uint64_t i = 0;
	skip_whitespace();
	while (isxdigit(*stream)) i = i * 16 + hex2int(*stream++);
	return i;
}

void ir_gen_program(FILE *f, struct xallang_program *pgrm)
{
	fprintf(f, "#!/home/yagi/dev/c/comp/main\n\n\n");

	for (ssize_t i = 0; i < buf_len(pgrm->defs); i++) {
		ir_gen_definition(f, pgrm->defs + i);
		fprintf(f, "\n\n");
	}
}

void ir_gen_definition(FILE *f, struct xallang_definition *def)
{
	fprintf(f, "%.*s(", (int) def->name.len, def->name.name);
	fprintf(f, ")\n");
	for (ssize_t i = 0; i < buf_len(def->params); i++) {
		fprintf(f, "\tlocal ");
		ir_gen_ident(f, def->params[i]);
		fprintf(f, "\n");
	}
	for (ssize_t i = 0; i < buf_len(def->locals); i++) {
		fprintf(f, "\tlocal ");
		ir_gen_ident(f, def->locals[i]);
		fprintf(f, "\n");
	}
	for (ssize_t i = 0; i < buf_len(def->blk.stmts); i++) {
		ir_gen_statement(f, def->blk.stmts + i);
	}
	fprintf(f, "\tret ");
	ir_gen_intexpr(f, def->retval);

	fprintf(f, "\n$");
}

void ir_gen_statement(FILE *f, struct xallang_statement *stmt)
{
	switch (stmt->kind) {
		case XALLANG_SET:
			fprintf(f, "\tset ");
			ir_gen_ident(f, stmt->xset.id);
			fprintf(f, ", ");
			ir_gen_intexpr(f, stmt->xset.val);
			break;
		case XALLANG_IF:
			assert(0 && "if");
			break;
		case XALLANG_WHILE:
			assert(0 && "while");
			break;
		default:
			assert(0);

	}
	fprintf(f, "\n");
}

static uint64_t unique_id = 0;

void ir_gen_intexpr(FILE *f, struct xallang_intexpression *iexpr)
{
	switch (iexpr->kind) {
	case XALLANG_SUM:
		assert(0 && "expressions not implemented");
		break;
	case XALLANG_ID:
		ir_gen_ident(f, iexpr->xid);
		break;
	case XALLANG_INT:
		fprintf(f, "%" PRIx64, iexpr->xint);
		break;
	default:
		assert(0);
	}
}

void ir_gen_boolexpr(FILE *f, struct xallang_boolexpression *bexpr)
{
	assert(0 && "bool exprs not impl'ed");
}

void ir_gen_ident(FILE *f, struct identifier id)
{
	fprintf(f, "%.*s", (int) id.len, id.name);
}

/*
void ir_dump_program(FILE *f, int indent, struct ir_program *pgrm);
void ir_dump_definition(FILE *f, int indent, struct ir_definition *def);
void ir_dump_statement(FILE *f, int indent, struct ir_statement *stmt);
void ir_dump_operand(FILE *f, int indent, struct ir_operand *op);
*/

void ir_trs_program(struct ir_program *ipgrm, struct xallang_program *xpgrm)
{
	for (ssize_t i = 0; i < buf_len(xpgrm->defs); i++) {
		struct ir_definition idef = {0};
		ir_trs_definition(&idef, xpgrm->defs + i);
		buf_push(ipgrm->defs, idef);
	}
}

void ir_trs_definition(struct ir_definition *idef, struct xallang_definition *xdef)
{
	idef->name = xdef->name;
	
	buf_cat(idef->locals, xdef->params);
	buf_cat(idef->locals, xdef->locals);

	for (ssize_t i = 0; i < buf_len(xdef->blk.stmts); i++) {
		ir_trs_statement(idef->stmts, xdef->blk.stmts + i);
	}
	
	struct ir_operand *retv = NULL;
	buf_fit(retv, 1);
	ir_trs_intexpr(retv, xdef->retval);
	buf_push(idef->stmts, (struct ir_statement){ .kind = IR_INSTR, .instr = IRINSTR_RET, .ops = retv });
}

void ir_trs_statement(struct ir_statement *istmts, struct xallang_statement *xstmt)
{
	switch (xstmt->kind) {
	case IRINSTR_SET:
		break;
	case XALLANG_IF:
	case XALLANG_WHILE:
	default:
		assert(0);
	}
}

void ir_trs_intexpr(struct ir_operand *iop, struct xallang_intexpression *xiexpr)
{
	switch (xiexpr->kind) {
	case XALLANG_INT:
		iop->kind = IR_HEX;
		iop->oint = xiexpr->xint;
		break;
	case XALLANG_ID:
		iop->kind = IR_VAR;
		iop->oid = xiexpr->xid;
		break;
	case XALLANG_SUM:
	default:
		assert(0);
	}
}

void ir_trs_boolexpr(struct ir_operand *iop, struct xallang_boolexpression *xbexpr)
{
	switch (xbexpr->kind) {
	case XALLANG_NOT:
	case XALLANG_LT:
	case XALLANG_EQ:
	default:
		assert(0);
	}
}

