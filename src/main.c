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
	float *buf = NULL;
	buf_push(s.buf, 1.4f);
	buf_push(s.buf, -.000014f);
	buf_cat(buf, s.buf);
	assert(buf[0] == 1.4f);
	assert(buf[1] == -.000014f);
	buf_cat(buf, s.buf);
	assert(buf_len(buf) == 4);
	assert(buf[2] == 1.4f);
	assert(buf[3] == -.000014f);
	buf_fini(s.buf);
}

int main(int argc, char **argv)
{
	int ret = -1;
	struct memory_blob blob, ir_blob;
	struct xallang_program pgrm = {0};
	struct ir_program ir_pgrm = {0};
	const char *inf, *irf_name;
	FILE *ir_file;

	buf_test();

	if (argc != 2) {
		printf("usage: %s somefile\n", *argv);
		goto usage;
	}
	inf = argv[1];

	if ((ret = load_file(&blob, inf))) goto load;
	stream = blob.data;
	xl_parse_program(&pgrm);

	ir_trs_program(&ir_pgrm, &pgrm);

	/*
	irf_name = ".deleteme.ir";
	ir_file = fopen(irf_name, "w");
	ir_gen_program(ir_file, &pgrm);
	fclose(ir_file);

	if ((ret = load_file(&ir_blob, irf_name))) goto ir;
	stream = ir_blob.data;
	ir_parse_program(&ir_pgrm);
	*/

	// ir_dump_program(stdout, 0, &ir_pgrm);

	ax64_gen_program(stdout, &ir_pgrm);

	ret = 0;
ir:
	unload_file(&blob);

load: usage:
	return ret;
}

