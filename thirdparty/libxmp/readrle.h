/* nomarch 1.0 - extract old `.arc' archives.
 * Copyright (C) 2001 Russell Marks. See main.c for license details.
 *
 * readrle.h
 */

#ifndef LIBXMP_READRLE_H
#define LIBXMP_READRLE_H

struct rledata {
	int lastchr,repeating;
};

struct data_in_out {
	unsigned char *data_in_point,*data_in_max;
	unsigned char *data_out_point,*data_out_max;
};

void	libxmp_outputrle(int chr,void (*outputfunc)(int, struct data_in_out *),
			struct rledata *, struct data_in_out *);

unsigned char	*libxmp_convert_rle(unsigned char *data_in,
                                  unsigned long in_len,
                                  unsigned long orig_len);

#endif
