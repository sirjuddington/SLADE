/* nomarch 1.4 - extract old `.arc' archives.
 * Copyright (C) 2001-2006 Russell Marks. See unarc.c for license details.
 *
 * Modified for xmp by Claudio Matsuoka, Aug 2007
 * - add quirks for Digital Symphony LZW packing
 * - add wrapper to read data from stream
 *
 * Modified for xmp by Claudio Matsuoka, Feb 2012
 * - remove global data
 *
 * readlzw.c - read (RLE+)LZW-compressed files.
 *
 * This is based on zgv's GIF reader. The LZW stuff is much the same, but
 * figuring out the details of the rather bizarre encoding involved much
 * wall therapy. %-(
 */

#include "readrle.h"

#include "common.h"
#include "readlzw.h"


struct local_data {
  /* now this is for the string table.
   * the data->st_ptr array stores which pos to back reference to,
   *  each string is [...]+ end char, [...] is traced back through
   *  the 'pointer' (index really), then back through the next, etc.
   *  a 'null pointer' is = to LZW_UNUSED.
   * the data->st_chr array gives the end char for each.
   *  an unoccupied slot is = to LZW_UNUSED.
   */
  #define LZW_UNUSED (-1)
  #define REALMAXSTR 65536
  int st_ptr[REALMAXSTR],st_chr[REALMAXSTR],st_last;
  int st_ptr1st[REALMAXSTR];

  /* this is for the byte -> bits mangler:
   *  dc_bitbox holds the bits, dc_bitsleft is number of bits left in dc_bitbox,
   */
  int dc_bitbox,dc_bitsleft;

  int codeofs;
  int global_use_rle,oldver;
  struct rledata rd;
  struct data_in_out io;
  uint32 quirk;

  int maxstr;
  int outputstring_buf[REALMAXSTR];

  int st_oldverhashlinks[4096];	/* only used for 12-bit types */

  int nomarch_input_size;	/* hack for xmp, will fix later */
};

/* prototypes */
static void code_resync(int old, struct local_data *);
static void inittable(int orgcsize, struct local_data *);
static int addstring(int oldcode,int chr, struct local_data *);
static int readcode(int *newcode,int numbits, struct local_data *);
static void outputstring(int code, struct local_data *);
static void outputchr(int chr, struct local_data *);
static int findfirstchr(int code, struct local_data *);


static unsigned char *convert_lzw_dynamic(unsigned char *data_in,
					  int max_bits,int use_rle,
					  unsigned long in_len,
					  unsigned long orig_len, int q,
					  struct local_data *data)
{
	unsigned char *data_out;
	int csize, orgcsize;
	int newcode, oldcode, k = 0;
	int first = 1, noadd;

	//printf("in_len = %d, orig_len = %d\n", in_len, orig_len);
	data->quirk = q;
	data->global_use_rle = use_rle;
	data->maxstr = (1<<max_bits);

	if (data->maxstr > REALMAXSTR) {
	    return NULL;
	}

	if ((data_out = calloc(1, orig_len)) == NULL) {
	/*  fprintf(stderr,"nomarch: out of memory!\n");*/
	    return NULL;
	}

	data->io.data_in_point = data_in;
	data->io.data_in_max = data_in + in_len;
	data->io.data_out_point = data_out;
	data->io.data_out_max = data_out + orig_len;
	data->dc_bitbox = data->dc_bitsleft = 0;
	data->codeofs = 0;
	libxmp_outputrle(-1, NULL, &data->rd, &data->io);	/* init RLE */

	data->oldver = 0;
	csize = 9;		/* initial code size */
	if (max_bits == 0) {	/* special case for static 12-bit */
	    data->oldver = 1,csize = 12,data->maxstr = 4096;
	}
	orgcsize = csize;
	inittable(orgcsize, data);

	oldcode = newcode = 0;
	if (data->quirk & NOMARCH_QUIRK_SKIPMAX) {
	    data->io.data_in_point++;	/* skip type 8 max. code size, always 12 */
	}

	if (max_bits == 16) {
	    data->maxstr = (1 << *data->io.data_in_point++);	 /* but compress-type *may* change it (!) */
	}

	/* XXX */
	if (data->maxstr > (1 << max_bits)) {
	    free(data_out);
	    return NULL;
	}

	data->nomarch_input_size = 0;

	while (1) {
	    //printf("input_size = %d        csize = %d\n", data->nomarch_input_size, csize);
	    if (!readcode(&newcode,csize,data)) {
		//printf("readcode failed!\n");
		break;
	    }
	    //printf("newcode = %x\n", newcode);

	    if (data->quirk & NOMARCH_QUIRK_END101) {
		if (newcode == 0x101 /* data_out_point>=data_out_max */) {
		    //printf("end\n");
		    break;
		}
	    }

	    noadd = 0;
	    if (first) {
		k = newcode, first = 0;
		if (data->oldver) noadd = 1;
	    }

	    if (newcode == 256 && !data->oldver) {
		/* this *doesn't* reset the table (!), merely reduces code size again.
		 * (It makes new strings by treading on the old entries.)
		 * This took fscking forever to work out... :-(
		 */
		data->st_last = 255;

		if (data->quirk & NOMARCH_QUIRK_START101) { /* CM: Digital symphony data->quirk */
		    data->st_last++;
		}

		/* XXX do we need a resync if there's a reset when *already* csize==9?
		 * (er, assuming that can ever happen?)
		 */
		code_resync(csize, data);
		csize = orgcsize;
		if (!readcode(&newcode,csize,data)) {
		    break;
		}
	    }

	    if ((!data->oldver && newcode <= data->st_last) ||
		(data->oldver && data->st_chr[newcode] != LZW_UNUSED)) {
		outputstring(newcode, data);
		k = findfirstchr(newcode, data);
	    }
	    else {
		/* this is a bit of an assumption, but these ones don't seem to happen in
		 * non-broken files, so...
		 */
		#if 0
		/* actually, don't bother, just let the CRC tell the story. */
		if (newcode > data->st_last + 1) {
		    fprintf(stderr,"warning: bad LZW code\n");
		}
		#endif
		/*k = findfirstchr(oldcode);*/	/* don't think I actually need this */
		outputstring(oldcode, data);
		outputchr(k, data);
	    }

	    if (data->st_last != data->maxstr - 1) {
		if (!noadd) {
		    if (!addstring(oldcode,k,data)) {
			/* XXX I think this is meant to be non-fatal?
			 * well, nothing for now, anyway...
			 */
		    }
		    if (data->st_last != data->maxstr - 1 && data->st_last == ((1<<csize) - 1)) {
			csize++;
			code_resync(csize-1, data);
		    }
		}
	    }

	    oldcode = newcode;
	}

	if (~data->quirk & NOMARCH_QUIRK_NOCHK) {
	    /* junk it on error */
	    if (data->io.data_in_point != data->io.data_in_max) {
		free(data_out);
		return NULL;
	    }
	}

	return data_out;
}

unsigned char *libxmp_convert_lzw_dynamic(unsigned char *data_in,
					  int max_bits,int use_rle,
					  unsigned long in_len,
					  unsigned long orig_len, int q)
{
	struct local_data *data;
	unsigned char *d;

	if ((data = malloc(sizeof (struct local_data))) == NULL) {
		goto err;
	}

	d = convert_lzw_dynamic(data_in, max_bits, use_rle, in_len,
						orig_len, q, data);

	/* Sanity check */
	if (d == NULL) {
		goto err2;
	}
	if (d + orig_len != data->io.data_out_point) {
		free(d);
		goto err2;
	}

	free(data);

	return d;

err2:	free(data);
err:	return NULL;
}

unsigned char *libxmp_read_lzw_dynamic(FILE *f, uint8 *buf, int max_bits,int use_rle,
			unsigned long in_len, unsigned long orig_len, int q)
{
	uint8 *buf2, *b;
	int pos;
	int size;
	struct local_data *data;
	size_t read_len;

	if ((data = malloc(sizeof (struct local_data))) == NULL) {
		goto err;
	}

	if ((buf2 = malloc(in_len)) == NULL) {
		//perror("read_lzw_dynamic");
		goto err2;
	}

	pos = ftell(f);
	if ((read_len = fread(buf2, 1, in_len, f)) != in_len) {
		if (~q & XMP_LZW_QUIRK_DSYM) {
			goto err3;
		}
		in_len = read_len;
	}
	b = convert_lzw_dynamic(buf2, max_bits, use_rle, in_len, orig_len, q, data);
	memcpy(buf, b, orig_len);
	size = q & NOMARCH_QUIRK_ALIGN4 ? ALIGN4(data->nomarch_input_size) :
						data->nomarch_input_size;
	if (fseek(f, pos + size, SEEK_SET) < 0) {
		goto err4;
	}
	free(b);
	free(buf2);
	free(data);

	return buf;

err4:	free(b);
err3:	free(buf2);
err2:	free(data);
err:	return NULL;
}

/* uggghhhh, this is agonisingly painful. It turns out that
 * the original program bunched up codes into groups of 8, so we have
 * to waste on average about 5 or 6 bytes when we increase code size.
 * (and ghod, was this ever a complete bastard to track down.)
 * mmm, nice, tell me again why this format is dead?
 */
static void code_resync(int old, struct local_data *data)
{
	int tmp;

	if (data->quirk & NOMARCH_QUIRK_NOSYNC) {
	    return;
	}

	while (data->codeofs) {
	    if (!readcode(&tmp,old,data))
		break;
	}
}


static void inittable(int orgcsize, struct local_data *data)
{
	int f;
	int numcols = (1 << (orgcsize - 1));

	for (f = 0; f < REALMAXSTR; f++) {
	    data->st_chr[f]=LZW_UNUSED;
	    data->st_ptr[f]=LZW_UNUSED;
	    data->st_ptr1st[f]=LZW_UNUSED;
	}

	for (f = 0; f < 4096; f++) {
	    data->st_oldverhashlinks[f] = LZW_UNUSED;
	}

	if (data->oldver) {
	    data->st_last = -1; /* since it's a counter, when static */
	    for (f = 0; f < 256; f++) {
		addstring(0xffff,f,data);
	    }
	}
	else {
	    for (f = 0; f < numcols; f++) {
		data->st_chr[f] = f;
	    }
	    data->st_last = numcols - 1; /* last occupied slot */

	    if (data->quirk & NOMARCH_QUIRK_START101) { /* CM: Digital symphony quirk */
		data->st_last++;
	    }
	}
}


/* required for finding true table index in ver 1.x files */
static int oldver_getidx(int oldcode,int chr, struct local_data *data)
{
	int lasthash, hashval;
	int a, f;

	/* in type 5/6 crunched files, we hash the code into the array. This
	 * means we don't have a real data->st_last, but for compatibility with
	 * the rest of the code we pretend it still means that. (data->st_last
	 * has already been incremented by the time we get called.) In our
	 * case it's only useful as a measure of how full the table is.
	 *
	 * the hash is a mid-square thing.
	 */
	a = (((oldcode + chr) | 0x800) & 0xffff);
	hashval = (((a * a) >> 6) & 0xfff);

	/* first, check link chain from there */
	while (data->st_chr[hashval] != LZW_UNUSED && data->st_oldverhashlinks[hashval] != LZW_UNUSED) {
	    hashval = data->st_oldverhashlinks[hashval];
	}

	/* make sure we return early if possible to avoid adding link */
	if (data->st_chr[hashval] == LZW_UNUSED)
	{
	    return hashval;
	}

	lasthash = hashval;

	/* slightly odd approach if it's not in that - first try skipping
	 * 101 entries, then try them one-by-one. It should be impossible
	 * for this to loop indefinitely, if the table isn't full. (And we
	 * shouldn't have been called if it was full...)
	 */
	hashval += 101;
	hashval &= 0xfff;

	if (data->st_chr[hashval] != LZW_UNUSED) {
	    for (f = 0; f < data->maxstr; f++, hashval++, hashval &= 0xfff) {
		if (data->st_chr[hashval] == LZW_UNUSED) {
		    break;
		}
	    }
	    if (hashval == data->maxstr) {
		return -1; /* table full, can't happen */
	    }
	}

	/* add link to here from the end of the chain */
	data->st_oldverhashlinks[lasthash] = hashval;

	return hashval;
}


/* add a string specified by oldstring + chr to string table */
int addstring(int oldcode,int chr, struct local_data *data)
{
	int idx;

	//printf("oldcode = %02x\n", oldcode);
	data->st_last++;
	if (data->st_last & data->maxstr) {
	    data->st_last=data->maxstr - 1;
	    return 1; /* not too clear if it should die or not... */
	}

	idx = data->st_last;
	//printf("addstring idx=%x, oldcode=%02x, chr=%02x\n", idx, oldcode, chr);

	if (data->oldver) {
	    /* old version finds index in a rather odd way. */
	    if ((idx = oldver_getidx(oldcode,chr,data)) == -1) {
		return 0;
	    }
	}

	data->st_chr[idx] = chr;

	/* XXX should I re-enable this? think it would be ok... */
#if 0
	if (data->st_last == oldcode)
	    return 0;		/* corrupt */
#endif
	if (oldcode >= data->maxstr) return 1;
	data->st_ptr[idx] = oldcode;

	if (data->st_ptr[oldcode] == LZW_UNUSED) { /* if we're pointing to a root... */
	    data->st_ptr1st[idx] = oldcode;        /* then that holds the first char */
	}
	else {                                     /* otherwise... */
	    data->st_ptr1st[idx]=data->st_ptr1st[oldcode]; /* use their pointer to first */
	}

	return 1;
}


/* read a code of bitlength numbits */
static int readcode(int *newcode, int numbits, struct local_data *data)
{
	int bitsfilled, got;

	bitsfilled = got = 0;
	*newcode = 0;

	while (bitsfilled<numbits) {
	    if (data->dc_bitsleft == 0) {       /* have we run out of bits? */
		if (data->io.data_in_point >= data->io.data_in_max) {
		    //printf("data_in_point=%p >= data_in_max=%p\n", data_in_point, data_in_max);
		    return 0;
		}
		data->dc_bitbox = *data->io.data_in_point++;
		data->dc_bitsleft = 8;
		data->nomarch_input_size++;	/* hack for xmp/dsym */
	    }
	    if (data->dc_bitsleft < numbits-bitsfilled) {
		got = data->dc_bitsleft;
	    }
	    else {
		got = numbits-bitsfilled;
	    }

	    if (data->oldver) {
		data->dc_bitbox &= 0xff;
		data->dc_bitbox <<= got;
		bitsfilled += got;

		/* Sanity check */
		if (bitsfilled > numbits) {
		    return 0;
		}

		*newcode |= ((data->dc_bitbox >> 8) << (numbits - bitsfilled));
		data->dc_bitsleft -= got;
	    }
	    else {
		*newcode |= ((data->dc_bitbox & ((1<<got) - 1)) << bitsfilled);
		data->dc_bitbox >>= got;
		data->dc_bitsleft -= got;
		bitsfilled += got;
	    }
	}

	if (*newcode < 0 || *newcode > data->maxstr - 1) {
	    //printf("*newcode (= %d) < 0 || *newcode (= %d) > data->maxstr (= %d) - 1\n", *newcode, *newcode, data->maxstr);
	    return 0;
	}

	/* yuck... see code_resync() for explanation */
	data->codeofs++;
	data->codeofs &= 7;

	return 1;
}


static void outputstring(int code, struct local_data *data)
{
	int *ptr = data->outputstring_buf;

	while (data->st_ptr[code] != LZW_UNUSED && ptr < data->outputstring_buf + data->maxstr) {
	    *ptr++ = data->st_chr[code];
	    code   = data->st_ptr[code];
	}

	outputchr(data->st_chr[code], data);
	while (ptr > data->outputstring_buf) {
	    outputchr(*--ptr, data);
	}
}


static void rawoutput(int byte, struct data_in_out *io)
{
	//static int i = 0;
	if (io->data_out_point < io->data_out_max) {
	    *io->data_out_point++ = byte;
	}
	//printf(" output = %02x <================ %06x\n", byte, i++);
}


static void outputchr(int chr, struct local_data *data)
{
	if (data->global_use_rle) {
	    libxmp_outputrle(chr,rawoutput,&data->rd,&data->io);
	}
	else {
	    rawoutput(chr,&data->io);
	}
}


static int findfirstchr(int code, struct local_data *data)
{
	if (data->st_ptr[code] != LZW_UNUSED) { /* not first? then use brand new st_ptr1st! */
	    code = data->st_ptr1st[code];    /* now with no artificial colouring */
	}
	return data->st_chr[code];
}
