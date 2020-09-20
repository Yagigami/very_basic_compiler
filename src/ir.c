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

void ir_parse_statement(struct ir_definition *def, struct ir_statement *stmt)
{
	while (skip_whitespace(), *stream == ':') {
		stream++;
		ssize_t idx = buf_len(def->labels);
		buf_push(def->labels, idx);
	}
	ssize_t n;
	stmt->instr = ir_parse_instr(&n);
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

uint64_t unique_id = 0;

static struct identifier unique_identifier(const char *base)
{
	struct identifier id;
	enum { N = 32 };
	char *mem = xmalloc(N);
	id.name = mem;
	id.len = snprintf(mem, N, "%s_%" PRIx64, base, ++unique_id);
	assert(id.len <= N);
	return id;
}

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
	(void) f, (void) bexpr;
	assert(0 && "bool exprs not impl'ed");
}

void ir_gen_ident(FILE *f, struct identifier id)
{
	fprintf(f, "%.*s", (int) id.len, id.name);
}

void ir_dump_program(FILE *f, int indent, struct ir_program *pgrm)
{
	fprintln(f, indent + INDENT_SH, "{");
	fprintln(f, indent + INDENT_SH, "\"defs\": [");
	for (ssize_t i = 0; i < buf_len(pgrm->defs); i++) {
		if (i) fprintf(f, ",");
		ir_dump_definition(f, indent + INDENT_SH, pgrm->defs + i);
	}
	fprintln(f, indent, "]");
	fprintln(f, indent, "}");
}

void ir_dump_definition(FILE *f, int indent, struct ir_definition *def)
{
	fprintln(f, indent + INDENT_SH, "{");
	fprintf(f, "\"name\": ");
	fprintf(f, "\"%.*s\"", (int) def->name.len, def->name.name);
	fprintln(f, indent + INDENT_SH, ",");

	fprintf(f, "\"params\": [ ");
	for (ssize_t i = 0; i < buf_len(def->params); i++) {
		if (i) fprintf(f, ", ");
		fprintf(f, "\"%.*s\"", (int) def->params[i].len, def->params[i].name);
	}
	fprintln(f, indent + INDENT_SH, "],");

	fprintf(f, "\"locals\": [ ");
	for (ssize_t i = 0; i < buf_len(def->locals); i++) {
		if (i) fprintf(f, ", ");
		fprintf(f, "\"%.*s\"", (int) def->locals[i].len, def->locals[i].name);
	}
	fprintln(f, indent + INDENT_SH, "],");

	fprintf(f, "\"labels\": [ ");
	for (ssize_t i = 0; i < buf_len(def->labels); i++) {
		if (i) fprintf(f, ", ");
		fprintf(f, "%td", def->labels[i]);
	}
	fprintln(f, indent + INDENT_SH, "],");

	fprintln(f, indent + INDENT_SH, "\"stmts\": [");
	for (ssize_t i = 0; i < buf_len(def->stmts); i++) {
		if (i) fprintln(f, indent + INDENT_SH, ",");
		ir_dump_statement(f, indent + INDENT_SH, def->stmts + i);
	}
	fprintln(f, indent + INDENT_SH, "");
	fprintln(f, indent + INDENT_SH, "]");
	fprintln(f, indent, "}");
}

const char *ir_type2str[] = {
	[IR_UNK          ] = "<unknown>",
	[IR_HEX          ] = "<integer>",
	[IR_VAR          ] = "<identifier>",
	[IR_LABEL        ] = "<label>",
	[IRINSTR_SET     ] = "SET",
	[IRINSTR_RET     ] = "RET",
	[IRINSTR_LOCAL   ] = "LOCAL",
	[IRINSTR_ADD     ] = "ADD",
	[IRINSTR_CMP     ] = "CMP",
	[IRINSTR_JMP     ] = "JMP",
	[IRINSTR_JZ      ] = "JZ",
	[IRINSTR_JNZ     ] = "JNZ",
	[IRINSTR_JL      ] = "JL",
};

void ir_dump_statement(FILE *f, int indent, struct ir_statement *stmt)
{
	fprintf(f, "{ ");
	fprintf(f, "\"instr\": \"%s\", \"ops\": [", ir_type2str[stmt->instr]);
	for (ssize_t i = 0; i < buf_len(stmt->ops); i++) {
		if (i) fprintf(f, ", ");
		ir_dump_operand(f, indent, stmt->ops + i);
	}
	fprintf(f, "]");
	fprintf(f, " }");
}

void ir_dump_operand(FILE *f, int indent, struct ir_operand *op)
{
	(void) indent;
	switch (op->kind) {
	case IR_HEX:
		fprintf(f, "%" PRIu64, op->oint);
		break;
	case IR_VAR:
		fprintf(f, "\"%.*s\"", (int) op->oid.len, op->oid.name);
		break;
	case IR_LABEL:
		fprintf(f, "%zd", op->olbl);
		break;
	default:
		assert(0);
	}
}

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
	
	buf_cat(idef->params, xdef->params);
	buf_cat(idef->locals, xdef->locals);

	ir_trs_block(idef, &xdef->blk);
	
	struct ir_operand *retv = NULL;
	struct ir_operand op;
	ir_trs_intexpr(idef, &op, xdef->retval);
	buf_push(retv, op);
	buf_push(idef->stmts, (struct ir_statement){ .instr = IRINSTR_RET, .ops = retv });
}

void ir_trs_block(struct ir_definition *idef, struct xallang_block *blk)
{
	for (ssize_t i = 0; i < buf_len(blk->stmts); i++) {
		ir_trs_statement(idef, blk->stmts + i);
	}
}

void ir_trs_statement(struct ir_definition *idef, struct xallang_statement *xstmt)
{
	struct ir_operand *ops = NULL;
	struct ir_operand op;
	enum ir_type skip_cond;
	ssize_t mthen, melse, mend;
	switch (xstmt->kind) {
	case XALLANG_SET:
		buf_push(ops, (struct ir_operand){ .kind = IR_VAR, .oid = xstmt->xset.id });
		ir_trs_intexpr(idef, &op, xstmt->xset.val);
		buf_push(ops, op);
		buf_push(idef->stmts, (struct ir_statement){
				.instr = IRINSTR_SET,
				.ops = ops,
			});
		break;
	case XALLANG_IF:
		ir_trs_boolexpr(idef, &skip_cond, xstmt->xif.cond);
		buf_push(idef->stmts, (struct ir_statement){ .instr = skip_cond });
		mthen = buf_len(idef->stmts) - 1;
		ir_trs_block(idef, &xstmt->xif.thenb);
		buf_push(idef->stmts, (struct ir_statement){ .instr = IRINSTR_JMP });
		melse = buf_len(idef->stmts) - 1;
		buf_push(idef->labels, melse + 1);
		ir_trs_block(idef, &xstmt->xif.elseb);
		mend = buf_len(idef->stmts) - 1;
		buf_push(idef->labels, mend + 1);
		buf_push(ops, (struct ir_operand){
				.kind = IR_LABEL,
				.olbl = melse + 1,
			});
		idef->stmts[mthen].ops = ops;
		ops = NULL;
		buf_push(ops, (struct ir_operand){
				.kind = IR_LABEL,
				.olbl = mend + 1,
			});
		idef->stmts[melse].ops = ops;
		break;
	case XALLANG_WHILE:
		buf_push(idef->stmts, (struct ir_statement){ .instr = IRINSTR_JMP });
		mthen = buf_len(idef->stmts) - 1;
		buf_push(idef->labels, mthen + 1);
		ir_trs_block(idef, &xstmt->xwhile.body);
		mend = buf_len(idef->stmts) - 1;
		buf_push(idef->labels, mend + 1);
		buf_push(ops, (struct ir_operand){
				.kind = IR_LABEL,
				.olbl = mend + 1,
			});
		idef->stmts[mthen].ops = ops;
		ir_trs_boolexpr(idef, &skip_cond, xstmt->xwhile.cond);
		ops = NULL;
		buf_push(ops, (struct ir_operand){
				.kind = IR_LABEL,
				.olbl = mthen + 1,
			});
		buf_push(idef->stmts, (struct ir_statement){ .instr = skip_cond, .ops = ops});
		break;
	default:
		assert(0);
	}
}

void ir_trs_intexpr(struct ir_definition *idef, struct ir_operand *iop, struct xallang_intexpression *xiexpr)
{
	struct ir_operand *ops = NULL;
	struct ir_operand op;
	struct identifier id;
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
		id = unique_identifier("__local");
		buf_push(idef->locals, id);

		buf_push(ops, (struct ir_operand){ .kind = IR_VAR, .oid = id });
		buf_push(idef->stmts, (struct ir_statement){
				.instr = IRINSTR_LOCAL, .ops = ops });
		ops = NULL;
		buf_fit(ops, 3);
		buf_push(ops, (struct ir_operand){ .kind = IR_VAR, .oid = id });
		ir_trs_intexpr(idef, &op, xiexpr->lhs);
		buf_push(ops, op);
		ir_trs_intexpr(idef, &op, xiexpr->rhs);
		buf_push(ops, op);
		buf_push(idef->stmts, (struct ir_statement){
				.instr = IRINSTR_ADD, .ops = ops });

		*iop = ops[0];

		// iop->kind = IR_HEX;
		// iop->oint = -1;
		break;
	default:
		assert(0);
	}
}

void ir_trs_boolexpr(struct ir_definition *idef, enum ir_type *skip_cond, struct xallang_boolexpression *xbexpr)
{
	struct ir_operand *ops = NULL;
	struct ir_operand op;
	switch (xbexpr->kind) {
	case XALLANG_EQ:
		ir_trs_intexpr(idef, &op, xbexpr->lhs);
		buf_push(ops, op);
		ir_trs_intexpr(idef, &op, xbexpr->rhs);
		buf_push(ops, op);
		buf_push(idef->stmts, (struct ir_statement){
				.instr = IRINSTR_CMP, .ops = ops
			});
		*skip_cond = IRINSTR_JNZ;
		break;
	case XALLANG_NOT:
	case XALLANG_LT:
		ir_trs_intexpr(idef, &op, xbexpr->lhs);
		buf_push(ops, op);
		ir_trs_intexpr(idef, &op, xbexpr->rhs);
		buf_push(ops, op);
		buf_push(idef->stmts, (struct ir_statement){
				.instr = IRINSTR_CMP, .ops = ops
			});
		*skip_cond = IRINSTR_JGE;
		break;
	default:
		assert(0);
	}
}

