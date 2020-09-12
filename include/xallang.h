#ifndef XALLANG_H
#define XALLANG_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>


struct identifier {
	const char *name;
	ptrdiff_t len: sizeof (ptrdiff_t) * CHAR_BIT - 1;
	unsigned notresolved: 1;
};

enum xallang_type {
	XALLANG_SET, XALLANG_IF, XALLANG_WHILE,
	XALLANG_SUM, XALLANG_ID, XALLANG_INT,
	XALLANG_NOT, XALLANG_LT, XALLANG_EQ,
};

struct xallang_block {
	struct xallang_statement *stmts;
};

struct xallang_statement {
	enum xallang_type kind;
	union {
		struct { struct identifier id; struct xallang_intexpression *val; } xset;
		struct { struct xallang_boolexpression *cond; struct xallang_block thenb, elseb; } xif;
		struct { struct xallang_boolexpression *cond; struct xallang_block body; } xwhile;
	};
};

struct xallang_intexpression {
	enum xallang_type kind;
	union {
		struct { struct xallang_intexpression *lhs, *rhs; };
		struct identifier xid;
		uint64_t xint;
	};
};

struct xallang_boolexpression {
	enum xallang_type kind;
	union {
		struct { struct xallang_intexpression *lhs, *rhs; };
		struct xallang_boolexpression *xbool;
	};
};

struct xallang_definition {
	struct identifier name;
	struct identifier *params;
	struct xallang_block blk;
	struct xallang_intexpression *retval;
};

struct xallang_program {
	struct xallang_definition *defs;
};


extern const char *stream;

void xl_parse_program(struct xallang_program *pgrm);
void xl_parse_definition(struct xallang_definition *def);
void xl_parse_block(struct xallang_block *blk);
void xl_parse_statement(struct xallang_statement *stmt);
struct xallang_intexpression *xl_parse_intexpr(void);
struct xallang_boolexpression *xl_parse_boolexpr(void);

void xl_dump_program(FILE *f, int indent, struct xallang_program *pgrm);
void xl_dump_definition(FILE *f, int indent, struct xallang_definition *def);
void xl_dump_block(FILE *f, int indent, struct xallang_block *blk);
void xl_dump_statement(FILE *f, int indent, struct xallang_statement *stmt);
void xl_dump_intexpr(FILE *f, int indent, struct xallang_intexpression *iexpr);
void xl_dump_boolexpr(FILE *f, int indent, struct xallang_boolexpression *bexpr);

struct identifier parse_cidentifier(void);
uint64_t parse_u64(void);

static inline void fprintln(FILE *f, int indent, const char *s) {
	static const char *fill =
		"                                                                      "
		"                                                                      "
		"                                                                      "
		"                                                                      "
	;
	fprintf(f, "%s\n%.*s", s, indent, fill);
}

#define INDENT_SH 4


#endif /* XALLANG_H */

