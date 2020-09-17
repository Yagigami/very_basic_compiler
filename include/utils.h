#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdnoreturn.h>
#include <assert.h>
#include <string.h>


// memory
void *xmalloc(ssize_t sz);
void *xrealloc(void *ptr, ssize_t sz);
void *xcalloc(ssize_t sz, ssize_t cnt);
void xfree(void *ptr);

// file
struct memory_blob {
	void *data;
	ssize_t size;
};

int load_file  (struct memory_blob *blob, const char *path);
int write_file (struct memory_blob *blob, const char *path);
int unload_file(struct memory_blob *blob);

// buf
struct _stretchy_buf {
	ssize_t len, cap;
	char mem[];
};

#define _buf_hdr(x) ((struct _stretchy_buf *) ((char *) (x) - offsetof(struct _stretchy_buf, mem)))
#define _buf_len(x) _buf_hdr((x))->len
#define _buf_cap(x) _buf_hdr((x))->cap

#define buf_len(x) ((x) ? _buf_len((x)): 0)
#define buf_cap(x) ((x) ? _buf_cap((x)): 0)
// TODO: should be buf_cap - buf_len?
#define buf_fits(x, n) (buf_len((x)) >= (n))
#define buf_fit(x, n) (buf_fits((x), (n)) ? (x): ((x) = _buf_resize((x), (n), 2 * sizeof *(x))))
#define buf_push(x, ...) (buf_fit((x), buf_len(x)+1), (x)[_buf_len((x))++] = (__VA_ARGS__))
#define buf_fini(x) xfree(_buf_hdr((x)))
#define buf_cat(dst, src) (buf_fit((dst), buf_len((dst)) + buf_len((src))), (dst) = _buf_cat(_buf_hdr((dst)), _buf_hdr((src)), sizeof *(dst)))

static inline void *_buf_resize(void *old, size_t sz, size_t cnt)
{
	ssize_t len = buf_len(old);
	struct _stretchy_buf *buf = xrealloc(old ? _buf_hdr(old): old, sizeof *buf + cnt * sz);
	buf->len = len;
	buf->cap = cnt;
	return buf->mem;
}

static inline void *_buf_cat(struct _stretchy_buf *dst, struct _stretchy_buf *src, size_t sz)
{
	// can't legally do `!src->mem`
	if (src == (struct _stretchy_buf *) -offsetof(struct _stretchy_buf, mem))
		goto end;
	assert(dst->cap >= dst->len + src->len);
	memcpy(dst->mem + dst->len * sz, src->mem, src->len * sz);
	dst->len += src->len;
end:
	return dst->mem;
}

// stringop
#define CMP(s1, s2) strncmp((s1), (s2), sizeof (s1)-1)
#define MATCH_KW(kw) (CMP((kw), stream) == 0 && (stream += sizeof (kw)-1, 1))
void skip_whitespace(void);
void skip_whitespace_nonl(void);

// err
noreturn void error(const char *msg, ...);

#endif /* UTILS_H */

