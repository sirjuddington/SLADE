#ifndef LIBXMP_MEMIO_H
#define LIBXMP_MEMIO_H

#include <stddef.h>

typedef struct {
	const unsigned char *start;
	ptrdiff_t pos;
	ptrdiff_t size;
} MFILE;

#ifdef __cplusplus
extern "C" {
#endif

MFILE  *mopen(const void *, long);
int     mgetc(MFILE *stream);
size_t  mread(void *, size_t, size_t, MFILE *);
int     mseek(MFILE *, long, int);
long    mtell(MFILE *);
int     mclose(MFILE *);
int	meof(MFILE *);

#ifdef __cplusplus
}
#endif

#endif
