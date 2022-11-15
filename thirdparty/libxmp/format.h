#ifndef LIBXMP_FORMAT_H
#define LIBXMP_FORMAT_H

#include "common.h"
#include "hio.h"

struct format_loader {
	const char *name;
	int (*const test)(HIO_HANDLE *, char *, const int);
	int (*const loader)(struct module_data *, HIO_HANDLE *, const int);
};

const char *const *format_list(void);

#ifndef LIBXMP_CORE_PLAYER

#define NUM_FORMATS 53
#define NUM_PW_FORMATS 43

#define LIBXMP_NO_PROWIZARD

#ifndef LIBXMP_NO_PROWIZARD
int pw_test_format(HIO_HANDLE *, char *, const int, struct xmp_test_info *);
#endif
#endif

#endif

