#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "utils.h"
#include "xallang.h"
#include "ir.h"
#include "x86_64.h"


const char *stream;


void buf_test(void)
{
	struct { float *buf; } s = { NULL };
	buf_push(s.buf, 1.4f);
	buf_push(s.buf, -.000014f);
	buf_fini(s.buf);
}

int main(int argc, char **argv)
{
	int ret = -1;
	struct memory_blob blob;
	struct xallang_program pgrm = {0};
	const char *inf;

	buf_test();

	if (argc != 2) {
		printf("usage: %s somefile\n", *argv);
		goto usage;
	}
	inf = argv[1];

	if ((ret = load_file(&blob, inf))) goto load;
	stream = blob.data;
	// xl_parse_program(&pgrm);
	// xl_dump_program(stdout, 0, &pgrm);
	struct ir_program ir_pgrm = {0};
	ir_parse_program(&ir_pgrm);
	// ir_dump_program(stdout, 0, &ir_pgrm);
	ax64_gen_program(stdout, &ir_pgrm);
	if ((ret = unload_file(&blob))) goto ul;
	ret = 0;

ul: load: usage:
	return ret;
}

