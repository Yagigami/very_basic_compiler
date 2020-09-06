#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "xallang.h"


const char *stream;


int main(int argc, char **argv)
{
	int ret = -1;
	struct memory_blob blob;
	const char *inf;
	if (argc != 2) {
		printf("usage: %s somefile\n", *argv);
		goto usage;
	}
	inf = argv[1];

	if ((ret = load_file(&blob, inf))) goto load;
	puts(blob.data);
	if ((ret = unload_file(&blob))) goto ul;
	ret = 0;

ul: load: usage:
	return ret;
}

