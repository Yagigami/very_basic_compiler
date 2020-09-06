#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/fcntl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "utils.h"


void *xmalloc(ssize_t sz)
{
	void *p = malloc(sz);
	if (p) return p;
	printf("xmalloc(%zd): allocation failure.\n", sz);
	exit(1);
}

void *xrealloc(void *ptr, ssize_t sz)
{
	void *p = realloc(ptr, sz);
	if (p) return p;
	printf("xmalloc(%zd): allocation failure.\n", sz);
	exit(1);
}

void *xcalloc(ssize_t sz, ssize_t cnt)
{
	void *p = calloc(sz, cnt);
	if (p) return p;
	printf("xcalloc(%zd, %zd): allocation failure.\n", sz, cnt);
	exit(1);
}

void xfree(void *ptr)
{
	free(ptr);
}


int load_file (struct memory_blob *blob, const char *path)
{
	int ret = -1, fd;
	struct stat stat;
	void *mem;

	if ((fd = open(path, O_RDONLY)) == -1) goto open;
	if ((ret = fstat(fd, &stat))) goto stat;
	if ((mem = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *) -1) goto mmap;

	blob->data = mem;
	blob->size = stat.st_size;
	ret = 0;

mmap: stat:
	close(fd);
open:
	return ret;
}

int write_file(struct memory_blob *blob, const char *path)
{
	int ret = -1, fd;

	if ((fd = open(path, O_WRONLY)) == -1) goto open;
	if ((ret = write(fd, blob->data, blob->size))) goto write;
	ret = 0;

write:
	close(fd);
open:
	return ret;
}

int unload_file(struct memory_blob *blob)
{
	return munmap(blob->data, blob->size);
}
