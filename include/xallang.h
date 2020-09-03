#ifndef XALLANG_H
#define XALLANG_H

#include <stdint.h>


struct identifier {
	char *name;
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

#endif /* XALLANG_H */

