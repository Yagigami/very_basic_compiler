#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/fcntl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <ctype.h>

#include "utils.h"


extern const char *stream;

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

void error(const char *msg, ...)
{
	fprintf(stderr, "error (%.32s): ", stream);
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	fputc('\n', stderr);
	va_end(args);
	exit(1);
}

void skip_whitespace(void)
{
	while (*stream == '#' || isspace(*stream)) {
		if (*stream == '#') do stream++; while (*stream != '\n');
		stream++;
	}
}

void skip_whitespace_nonl(void)
{
	while (*stream == '#' || (isspace(*stream) && *stream != '\n')) {
		if (*stream++ == '#') do stream++; while (*stream != '\n');
	}
}

struct identifier *id_find(struct identifier *buf, struct identifier id)
{
	for (ssize_t i = 0; i < buf_len(buf); i++) {
		if (id.len == buf[i].len &&
		strncmp(id.name, buf[i].name, id.len) == 0)
			return buf + i;
	}
	return NULL;
}

int id_cmp(struct identifier id1, struct identifier id2)
{
	ssize_t d = id1.len - id2.len;
	if (d) return d;
	return strncmp(id1.name, id2.name, id1.len);
}
