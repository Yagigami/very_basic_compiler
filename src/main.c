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
	float *buf = NULL;
	buf_push(buf, 1.4f);
	buf_push(buf, -.000014f);
	buf_push(buf, 6.789f);
	buf_push(buf, 6.789f);
	buf_push(buf, 6.789f);
	buf_push(buf, 6.789f);
	buf_push(buf, 6.789f);
	buf_push(buf, 6.789f);
	buf_push(buf, 6.789f);
	buf_fini(buf);
	struct s { uint64_t a[7]; } *b = NULL;
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_push(b, (struct s){ { 1, 2, 3, 4, 5, 6, 7} });
	buf_fini(b);
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

	// xl_dump_program(stdout, 0, &pgrm);

	/*
	irf_name = ".deleteme.ir";
	ir_file = fopen(irf_name, "w");
	ir_gen_program(ir_file, &pgrm);
	fclose(ir_file);

	if ((ret = load_file(&ir_blob, irf_name))) goto ir;
	stream = ir_blob.data;
	ir_parse_program(&ir_pgrm);
	*/

	ir_trs_program(&ir_pgrm, &pgrm);

	// ir_dump_program(stdout, 0, &ir_pgrm);

	ax64_gen_program(stdout, &ir_pgrm);

	ret = 0;
ir:
	unload_file(&blob);

load: usage:
	return ret;
}

