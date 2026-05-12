/*
libdrawtext - a simple library for fast text rendering in OpenGL
Copyright (C) 2011-2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef NO_FREETYPE
#define USE_FREETYPE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>
#include <errno.h>
#ifdef USE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif
#include "drawtext.h"
#include "drawtext_impl.h"
#include "tpool.h"

struct io {
	void *data;
	int size;
	int (*readchar)(struct io*);
	void *(*readline)(void *buf, int bsz, struct io*);
};

#define FTSZ_TO_PIXELS(x)	((x) / 64)
#define MAX_IMG_SIZE		8192

static int opt_padding = 8;
static int opt_save_ppm;

static struct dtx_glyphmap *load_glyphmap(struct io *io);

#ifdef USE_FREETYPE
static int init_freetype(void);
static void cleanup(void);

static int calc_best_size(int total_width, int max_gwidth, int max_gheight,
		int padding, int pow2, int *imgw, int *imgh);
static int next_pow2(int x);

static FT_Library ft;


static int init_done;

static int init_freetype(void)
{
	if(!init_done) {
		if(FT_Init_FreeType(&ft) != 0) {
			return -1;
		}
		atexit(cleanup);
		init_done = 1;
	}
	return 0;
}

static void cleanup(void)
{
	if(init_done) {
		FT_Done_FreeType(ft);
	}
}
#endif	/* USE_FREETYPE */

static int find_pow2(int x);

struct dtx_font *dtx_open_font(const char *fname, int sz)
{
	struct dtx_font *fnt = 0;

#ifdef USE_FREETYPE
	init_freetype();

	if(!(fnt = calloc(1, sizeof *fnt))) {
		fperror("failed to allocate font structure");
		return 0;
	}

	if(FT_New_Face(ft, fname, 0, (FT_Face*)&fnt->face) != 0) {
		fprintf(stderr, "failed to open font file: %s\n", fname);
		free(fnt);
		return 0;
	}

	/* pre-create the extended ASCII range glyphmap */
	if(sz) {
		dtx_prepare_range(fnt, sz, 0, 256);

		if(!dtx_font) {
			dtx_use_font(fnt, sz);
		}
	}
#else
	fprintf(stderr, "ignoring call to dtx_open_font: not compiled with freetype support!\n");
#endif

	return fnt;
}

struct dtx_font *dtx_open_font_mem(void *ptr, int memsz, int fontsz)
{
	struct dtx_font *fnt = 0;

#ifdef USE_FREETYPE
	FT_Open_Args args;

	init_freetype();

	if(!(fnt = calloc(1, sizeof *fnt))) {
		fperror("failed to allocate font structure");
		return 0;
	}

	memset(&args, 0, sizeof args);
	args.flags = FT_OPEN_MEMORY;
	args.memory_base = ptr;
	args.memory_size = memsz;

	if(FT_Open_Face(ft, &args, 0, (FT_Face*)&fnt->face) != 0) {
		fprintf(stderr, "failed to open font from memory\n");
		free(fnt);
		return 0;
	}

	/* pre-create the extended ASCII range glyphmap */
	if(fontsz) {
		dtx_prepare_range(fnt, fontsz, 0, 256);

		if(!dtx_font) {
			dtx_use_font(fnt, fontsz);
		}
	}
#else
	fprintf(stderr, "ignoring call to dtx_open_font_mem: not compiled with freetype support!\n");
#endif

	return fnt;
}

struct dtx_font *dtx_open_font_glyphmap(const char *fname)
{
	struct dtx_font *fnt;
	struct dtx_glyphmap *gmap;

	if(!(fnt = calloc(1, sizeof *fnt))) {
		fperror("failed to allocate font structure");
		return 0;
	}

	if(fname) {
		if(!(gmap = dtx_load_glyphmap(fname))) {
			free(fnt);
			return 0;
		}

		dtx_add_glyphmap(fnt, gmap);

		if(!dtx_font) {
			dtx_use_font(fnt, gmap->ptsize);
		}
	}
	return fnt;
}

struct dtx_font *dtx_open_font_glyphmap_mem(void *ptr, int memsz)
{
	struct dtx_font *fnt;
	struct dtx_glyphmap *gmap;

	if(!(fnt = calloc(1, sizeof *fnt))) {
		fperror("failed to allocate font structure");
		return 0;
	}

	if(!(gmap = dtx_load_glyphmap_mem(ptr, memsz))) {
		free(fnt);
		return 0;
	}

	dtx_add_glyphmap(fnt, gmap);

	if(!dtx_font) {
		dtx_use_font(fnt, gmap->ptsize);
	}
	return fnt;
}

void dtx_close_font(struct dtx_font *fnt)
{
	if(!fnt) return;

#ifdef USE_FREETYPE
	FT_Done_Face(fnt->face);
#endif

	/* destroy the glyphmaps */
	while(fnt->gmaps) {
		void *tmp = fnt->gmaps;
		fnt->gmaps = fnt->gmaps->next;
		dtx_free_glyphmap(tmp);
	}

	free(fnt);
}

void dtx_prepare(struct dtx_font *fnt, int sz)
{
	if(!dtx_get_font_glyphmap_range(fnt, sz, 0, 256)) {
		fprintf(stderr, "%s: failed (sz: %d, range: 0-255 [ascii])\n", __func__, sz);
	}
}

void dtx_prepare_range(struct dtx_font *fnt, int sz, int cstart, int cend)
{
	if(!dtx_get_font_glyphmap_range(fnt, sz, cstart, cend)) {
		fprintf(stderr, "%s: failed (sz: %d, range: %d-%d)\n", __func__, sz, cstart, cend);
	}
}

int dtx_calc_font_distfield(struct dtx_font *fnt, int scale_numer, int scale_denom)
{
	struct dtx_glyphmap *gm = fnt->gmaps;
	while(gm) {
		if(dtx_calc_glyphmap_distfield(gm) == -1) {
			fprintf(stderr, "%s failed to create distfield glyphmap\n", __func__);
			return -1;
		}

		if(dtx_resize_glyphmap(gm, scale_numer, scale_denom, DTX_LINEAR) == -1) {
			fprintf(stderr, "%s: failed to resize glyhphmap during distfield conversion\n", __func__);
		}
		gm->tex_valid = 0;
		gm = gm->next;
	}
	return 0;
}

struct dtx_glyphmap *dtx_get_font_glyphmap(struct dtx_font *fnt, int sz, int code)
{
	struct dtx_glyphmap *gm;

	/* check to see if the last we've given out fits the bill */
	if(fnt->last_gmap && code >= fnt->last_gmap->cstart && code < fnt->last_gmap->cend && fnt->last_gmap->ptsize == sz) {
		return fnt->last_gmap;
	}

	/* otherwise search for the appropriate glyphmap */
	gm = fnt->gmaps;
	while(gm) {
		if(code >= gm->cstart && code < gm->cend && sz == gm->ptsize) {
			fnt->last_gmap = gm;
			return gm;
		}
		gm = gm->next;
	}
	return 0;
}

struct dtx_glyphmap *dtx_get_font_glyphmap_range(struct dtx_font *fnt, int sz, int cstart, int cend)
{
	struct dtx_glyphmap *gm;

	/* search the available glyphmaps to see if we've got one that includes
	 * the requested range
	 */
	gm = fnt->gmaps;
	while(gm) {
		if(gm->cstart <= cstart && gm->cend >= cend && gm->ptsize == sz) {
			return gm;
		}
		gm = gm->next;
	}

	/* not found, create one and add it to the list */
	if(!(gm = dtx_create_glyphmap_range(fnt, sz, cstart, cend))) {
		return 0;
	}
	return gm;
}

int dtx_get_num_glyphmaps(struct dtx_font *fnt)
{
	int count = 0;
	struct dtx_glyphmap *gm = fnt->gmaps;
	while(gm) {
		++count;
		gm = gm->next;
	}
	return count;
}

struct dtx_glyphmap *dtx_get_glyphmap(struct dtx_font *fnt, int idx)
{
	struct dtx_glyphmap *gm = fnt->gmaps;
	while(gm && idx--) {
		gm = gm->next;
	}
	return gm;
}

struct dtx_glyphmap *dtx_create_glyphmap_range(struct dtx_font *fnt, int sz, int cstart, int cend)
{
	struct dtx_glyphmap *gmap = 0;

#ifdef USE_FREETYPE
	FT_Face face = fnt->face;
	int i, j;
	int gx, gy;
	int total_width, max_width, max_height;
	int half_pad = opt_padding / 2;

	FT_Set_Char_Size(fnt->face, 0, sz * 64, 72, 72);

	if(!(gmap = calloc(1, sizeof *gmap))) {
		return 0;
	}

	gmap->ptsize = sz;
	gmap->cstart = cstart;
	gmap->cend = cend;
	gmap->crange = cend - cstart;
	gmap->line_advance = FTSZ_TO_PIXELS((float)face->size->metrics.height);
	gmap->baseline = -FTSZ_TO_PIXELS((float)face->size->metrics.descender);

	if(!(gmap->glyphs = malloc(gmap->crange * sizeof *gmap->glyphs))) {
		free(gmap);
		return 0;
	}

	total_width = opt_padding;
	max_width = max_height = 0;

	for(i=0; i<gmap->crange; i++) {
		int w, h;

		FT_Load_Char(face, i + cstart, 0);
		w = FTSZ_TO_PIXELS(face->glyph->metrics.width);
		h = FTSZ_TO_PIXELS(face->glyph->metrics.height);

		if(w > max_width) max_width = w;
		if(h > max_height) max_height = h;

		total_width += w + opt_padding;
	}

	if(calc_best_size(total_width, max_width, max_height, opt_padding, 1, &gmap->xsz, &gmap->ysz) == -1) {
		free(gmap->glyphs);
		free(gmap);
		return 0;
	}
	gmap->xsz_shift = find_pow2(gmap->xsz);

	if(!(gmap->pixels = malloc(gmap->xsz * gmap->ysz))) {
		free(gmap->glyphs);
		free(gmap);
		return 0;
	}
	memset(gmap->pixels, 0, gmap->xsz * gmap->ysz);

	gx = opt_padding;
	gy = opt_padding;

	for(i=0; i<gmap->crange; i++) {
		float gwidth, gheight;
		unsigned char *src, *dst;
		FT_GlyphSlot glyph;

		FT_Load_Char(face, i + cstart, FT_LOAD_RENDER);
		glyph = face->glyph;
		gwidth = FTSZ_TO_PIXELS((float)glyph->metrics.width);
		gheight = FTSZ_TO_PIXELS((float)glyph->metrics.height);

		if(gx > gmap->xsz - gwidth - opt_padding) {
			gx = opt_padding;
			gy += max_height + opt_padding;
		}

		src = glyph->bitmap.buffer;
		dst = gmap->pixels + (gy << gmap->xsz_shift) + gx;

		for(j=0; j<(int)glyph->bitmap.rows; j++) {
			memcpy(dst, src, glyph->bitmap.width);
			dst += gmap->xsz;
			src += glyph->bitmap.pitch;
		}

		gmap->glyphs[i].code = i;
		gmap->glyphs[i].x = gx - half_pad;
		gmap->glyphs[i].y = gy - half_pad;
		gmap->glyphs[i].width = gwidth + half_pad * 2;
		gmap->glyphs[i].height = gheight + half_pad * 2;
		gmap->glyphs[i].orig_x = -FTSZ_TO_PIXELS((float)glyph->metrics.horiBearingX) + 1;
		gmap->glyphs[i].orig_y = FTSZ_TO_PIXELS((float)glyph->metrics.height - glyph->metrics.horiBearingY) + 1;
		gmap->glyphs[i].advance = FTSZ_TO_PIXELS((float)glyph->metrics.horiAdvance);
		/* also precalc normalized */
		gmap->glyphs[i].nx = (float)gmap->glyphs[i].x / (float)gmap->xsz;
		gmap->glyphs[i].ny = (float)gmap->glyphs[i].y / (float)gmap->ysz;
		gmap->glyphs[i].nwidth = (float)gmap->glyphs[i].width / (float)gmap->xsz;
		gmap->glyphs[i].nheight = (float)gmap->glyphs[i].height / (float)gmap->ysz;

		gx += gwidth + opt_padding;
	}

	/* add it to the glyphmaps list of the font */
	dtx_add_glyphmap(fnt, gmap);
#endif	/* USE_FREETYPE */

	return gmap;
}

void dtx_free_glyphmap(struct dtx_glyphmap *gmap)
{
	if(gmap) {
		free(gmap->pixels);
		free(gmap->glyphs);
		free(gmap);
	}
}

#define CHECK_BOUNDS(gm, x, y) ((x) >= 0 && (x) < (gm)->xsz && (y) >= 0 && (y) < (gm)->ysz)
#define GET_PIXEL(gm, x, y) ((gm)->pixels[((y) << (gm)->xsz_shift) + (x)])

static int calc_distance(struct dtx_glyphmap *gmap, int x, int y, int max_dist)
{
	int i, j, startx, starty, endx, endy, px, py;
	int bwidth, bheight;
	int min_distsq = INT_MAX;
	unsigned char cpix = GET_PIXEL(gmap, x, y);
	int dist;

	if(max_dist > 128) max_dist = 128;

	startx = x >= max_dist ? x - max_dist : 0;
	starty = y >= max_dist ? y - max_dist : 0;
	endx = x + max_dist < gmap->xsz ? x + max_dist : gmap->xsz - 1;
	endy = y + max_dist < gmap->ysz ? y + max_dist : gmap->ysz - 1;

	/* try the cardinal directions first to find the search bounding box */
	for(i=0; i<4; i++) {
		int max_dist = x - startx;
		for(j=0; j<max_dist; j++) {
			if(GET_PIXEL(gmap, x - j, y) != cpix) {
				startx = x - j;
				break;
			}
		}
		max_dist = endx + 1 - x;
		for(j=0; j<max_dist; j++) {
			if(GET_PIXEL(gmap, x + j, y) != cpix) {
				endx = x + j;
				break;
			}
		}
		max_dist = y - starty;
		for(j=0; j<max_dist; j++) {
			if(GET_PIXEL(gmap, x, y - j) != cpix) {
				starty = y - j;
				break;
			}
		}
		max_dist = endy + 1 - y;
		for(j=0; j<max_dist; j++) {
			if(GET_PIXEL(gmap, x, y + j) != cpix) {
				endy = y + j;
				break;
			}
		}
	}

	/* find the minimum squared distance inside the bounding box */
	bwidth = endx + 1 - startx;
	bheight = endy + 1 - starty;

	py = starty;
	for(i=0; i<bheight; i++) {
		px = startx;
		for(j=0; j<bwidth; j++) {
			if(GET_PIXEL(gmap, px, py) != cpix) {
				int dx = px - x;
				int dy = py - y;
				int distsq = dx * dx + dy * dy;

				if(distsq < min_distsq) {
					min_distsq = distsq;
				}
			}
			++px;
		}
		++py;
	}

	dist = (int)sqrt(min_distsq);
	if(dist > 127) dist = 127;

	return cpix ? dist + 128 : 127 - dist;
}

struct distcalc_data {
	struct dtx_glyphmap *gmap;
	int scanline;
	unsigned char *pixels;
};

static void distcalc_func(void *cls)
{
	int i;
	struct distcalc_data *data = cls;
	struct dtx_glyphmap *gmap = data->gmap;

	printf("scanline %d of %d\n", data->scanline + 1, gmap->ysz);
	for(i=0; i<gmap->xsz; i++) {
		*data->pixels++ = calc_distance(gmap, i, data->scanline, 64);
	}
}

int dtx_calc_glyphmap_distfield(struct dtx_glyphmap *gmap)
{
	int i, num_pixels = gmap->xsz * gmap->ysz;
	unsigned char *new_pixels;
	unsigned char *dptr;
#ifdef USE_THREADS
	struct dtx_thread_pool *tpool = 0;
	struct distcalc_data *data = 0;
#endif

	/* first quantize the glyphmap to 1bit */
	dptr = gmap->pixels;
	for(i=0; i<num_pixels; i++) {
		unsigned char c = *dptr;
		*dptr++ = c < 128 ? 0 : 255;
	}

	if(!(new_pixels = malloc(num_pixels))) {
		fprintf(stderr, "%s: failed to allocate %dx%d pixel buffer\n", __func__, gmap->xsz, gmap->ysz);
		return -1;
	}
	dptr = new_pixels;

#ifdef USE_THREADS
	tpool = dtx_tpool_create(0);
	data = malloc(sizeof *data * gmap->ysz);

	if(tpool) {
		for(i=0; i<gmap->ysz; i++) {
			data[i].gmap = gmap;
			data[i].scanline = i;
			data[i].pixels = new_pixels + (i << gmap->xsz_shift);
			dtx_tpool_enqueue(tpool, data + i, distcalc_func, 0);
		}
		dtx_tpool_wait(tpool);
		dtx_tpool_destroy(tpool);
		free(data);
	} else
#endif	/* USE_THREADS */
	{
		for(i=0; i<gmap->ysz; i++) {
			struct distcalc_data d;
			d.gmap = gmap;
			d.scanline = i;
			d.pixels = new_pixels + (i << gmap->xsz_shift);
			distcalc_func(&d);
		}
	}

	free(gmap->pixels);
	gmap->pixels = new_pixels;
	return 0;
}

static unsigned char sample_area(struct dtx_glyphmap *gm, float x, float y, float area)
{
	int i, j;
	int ksz = (int)(area + 0.5);
	int half_ksz = ksz / 2;

	int sum = 0, nsamples = 0;

	for(i=0; i<ksz; i++) {
		for(j=0; j<ksz; j++) {
			int sx = x + j - half_ksz;
			int sy = y + i - half_ksz;

			if(sx < 0 || sx >= gm->xsz || sy < 0 || sy >= gm->ysz) {
				continue;
			}

			sum += gm->pixels[(sy << gm->xsz_shift) + sx];
			++nsamples;
		}
	}

	if(nsamples != 0) {
		sum /= nsamples;
	}
	return sum > 255 ? 255 : sum;
}

static unsigned char sample_pixel(struct dtx_glyphmap *gm, int x, int y)
{
	if(CHECK_BOUNDS(gm, x, y)) {
		return gm->pixels[(y << gm->xsz_shift) + x];
	}
	return 0;
}

static int count_bits(int x)
{
	int i, n = 0;
	for(i=0; i<sizeof x * CHAR_BIT; i++) {
		n += x & 1;
		x >>= 1;
	}
	return n;
}

int dtx_resize_glyphmap(struct dtx_glyphmap *gmap, int snum, int sdenom, int filter)
{
	int i, j, nxsz, nysz;
	unsigned char *dptr, *new_pixels;
	float scale, inv_scale, area;

	if(snum == sdenom) return 0;

	if((count_bits(snum) | count_bits(sdenom)) != 1) {
		fprintf(stderr, "%s: invalid scale fraction %d/%d (not power of 2)\n", __func__, snum, sdenom);
		return -1;
	}

	/* normalize the fraction */
	if(snum > sdenom) {
		snum /= sdenom;
		sdenom /= sdenom;
	} else {
		snum /= snum;
		sdenom /= snum;
	}

	if(snum != 1 && sdenom != 1) {
		fprintf(stderr, "%s: invalid scale fraction %d/%d (neither is 1)\n", __func__, snum, sdenom);
		return -1;
	}

	nxsz = snum * gmap->xsz / sdenom;
	nysz = snum * gmap->ysz / sdenom;

	if(nxsz < 1 || nysz < 1) {
		return -1;
	}

	new_pixels = malloc(nxsz * nysz);
	if(!new_pixels) {
		fprintf(stderr, "%s: failed to allocate %dx%d pixel buffer\n", __func__, nxsz, nysz);
		return -1;
	}

	dptr = new_pixels;

	scale = (float)snum / (float)sdenom;
	inv_scale = 1.0 / scale;
	area = scale <= 1.0 ? inv_scale : 2.0;

	if(filter == DTX_NEAREST) {
		/* no filtering, nearest neighbor */
		for(i=0; i<nysz; i++) {
			for(j=0; j<nxsz; j++) {
				*dptr++ = sample_pixel(gmap, j * inv_scale, i * inv_scale);
			}
		}
	} else {
		/* bilinear filtering */
		for(i=0; i<nysz; i++) {
			for(j=0; j<nxsz; j++) {
				*dptr++ = sample_area(gmap, j * inv_scale, i * inv_scale, area);
			}
		}
	}

	free(gmap->pixels);
	gmap->pixels = new_pixels;
	gmap->xsz = nxsz;
	gmap->ysz = nysz;
	gmap->xsz_shift = find_pow2(nxsz);

	/* also scale all the metrics accordingly */
	for(i=0; i<gmap->crange; i++) {
		struct glyph *g = gmap->glyphs + i;
		g->x *= scale;
		g->y *= scale;
		g->width *= scale;
		g->height *= scale;
		g->orig_x *= scale;
		g->orig_y *= scale;
		g->advance *= scale;
	}
	gmap->ptsize = snum * gmap->ptsize / sdenom;
	gmap->line_advance *= scale;
	return 0;
}

unsigned char *dtx_get_glyphmap_pixels(struct dtx_glyphmap *gmap)
{
	return gmap->pixels;
}

int dtx_get_glyphmap_width(struct dtx_glyphmap *gmap)
{
	return gmap->xsz;
}

int dtx_get_glyphmap_height(struct dtx_glyphmap *gmap)
{
	return gmap->ysz;
}

int dtx_get_glyphmap_ptsize(struct dtx_glyphmap *gmap)
{
	return gmap->ptsize;
}

struct dtx_glyphmap *dtx_load_glyphmap(const char *fname)
{
	FILE *fp;
	struct dtx_glyphmap *gmap;

	if(!(fp = fopen(fname, "rb"))) {
		return 0;
	}
	gmap = dtx_load_glyphmap_stream(fp);
	fclose(fp);
	return gmap;
}


static int file_readchar(struct io *io)
{
	return fgetc(io->data);
}

static void *file_readline(void *buf, int bsz, struct io *io)
{
	return fgets(buf, bsz, io->data);
}

static int mem_readchar(struct io *io)
{
	char *p = io->data;

	if(io->size-- <= 0) {
		return -1;
	}
	io->data = p + 1;
	return *p;
}

static void *mem_readline(void *buf, int bsz, struct io *io)
{
	int c;
	char *ptr = buf;

	while(--bsz > 0 && (c = mem_readchar(io)) != -1) {
		*ptr++ = c;
		if(c == '\n') break;
	}
	*ptr = 0;

	return buf;
}

struct dtx_glyphmap *dtx_load_glyphmap_stream(FILE *fp)
{
	struct io io;
	io.data = fp;
	io.readchar = file_readchar;
	io.readline = file_readline;
	return load_glyphmap(&io);
}

struct dtx_glyphmap *dtx_load_glyphmap_mem(void *ptr, int memsz)
{
	struct io io;
	io.data = ptr;
	io.size = memsz;
	io.readchar = mem_readchar;
	io.readline = mem_readline;
	return load_glyphmap(&io);
}

static struct dtx_glyphmap *load_glyphmap(struct io *io)
{
	char buf[512];
	int hdr_lines = 0;
	struct dtx_glyphmap *gmap;
	struct glyph *glyphs = 0;
	struct glyph *g;
	int min_code = INT_MAX;
	int max_code = INT_MIN;
	int i, max_pixval = 255, num_pixels;
	int greyscale = 0;

	if(!(gmap = calloc(1, sizeof *gmap))) {
		fperror("failed to allocate glyphmap");
		return 0;
	}
	gmap->ptsize = -1;
	gmap->line_advance = FLT_MIN;

	while(hdr_lines < 3) {
		char *line = buf;
		if(!io->readline(buf, sizeof buf, io)) {
			fperror("unexpected end of file");
			goto err;
		}

		while(isspace(*line)) {
			line++;
		}

		if(line[0] == '#') {
			int c, res;
			float x, y, xsz, ysz, orig_x, orig_y, adv, line_adv, baseline;
			int ptsize;

			if((res = sscanf(line + 1, " size: %d\n", &ptsize)) == 1) {
				gmap->ptsize = ptsize;

			} else if((res = sscanf(line + 1, " advance: %f\n", &line_adv)) == 1) {
				gmap->line_advance = line_adv;

			} else if((res = sscanf(line + 1, " baseline: %f\n", &baseline)) == 1) {
				gmap->baseline = baseline;

			} else if((res = sscanf(line + 1, " %d: %fx%f+%f+%f o:%f,%f adv:%f\n",
							&c, &xsz, &ysz, &x, &y, &orig_x, &orig_y, &adv)) == 8) {
				if(!(g = malloc(sizeof *g))) {
					fperror("failed to allocate glyph");
					goto err;
				}
				g->code = c;
				g->x = x;
				g->y = y;
				g->width = xsz;
				g->height = ysz;
				g->orig_x = orig_x;
				g->orig_y = orig_y;
				g->advance = adv;
				/* normalized coordinates will be precalculated after everything is loaded */

				g->next = glyphs;
				glyphs = g;

				if(c < min_code) {
					min_code = c;
				}
				if(c > max_code) {
					max_code = c;
				}

			} else {
				fprintf(stderr, "%s: invalid glyph info line\n", __func__);
				goto err;
			}

		} else {
			switch(hdr_lines) {
			case 0:
				if(line[0] != 'P' || !(line[1] == '6' || line[1] == '5')) {
					fprintf(stderr, "%s: invalid file format (magic)\n", __func__);
					goto err;
				}
				greyscale = line[1] == '5';
				break;

			case 1:
				if(sscanf(line, "%d %d", &gmap->xsz, &gmap->ysz) != 2) {
					fprintf(stderr, "%s: invalid file format (dim)\n", __func__);
					goto err;
				}
				break;

			case 2:
				{
					char *endp;
					max_pixval = strtol(line, &endp, 10);
					if(endp == line) {
						fprintf(stderr, "%s: invalid file format (maxval)\n", __func__);
						goto err;
					}
				}
				break;

			default:
				break;	/* can't happen */
			}
			hdr_lines++;
		}
	}

	if(gmap->ptsize == -1 || gmap->line_advance == FLT_MIN) {
		fprintf(stderr, "%s: invalid glyphmap, insufficient information in ppm comments\n", __func__);
		goto err;
	}

	/* precalculate normalized glyph coordinates */
	g = glyphs;
	while(g) {
		g->nx = g->x / gmap->xsz;
		g->ny = g->y / gmap->ysz;
		g->nwidth = g->width / gmap->xsz;
		g->nheight = g->height / gmap->ysz;
		g = g->next;
	}

	num_pixels = gmap->xsz * gmap->ysz;
	if(!(gmap->pixels = malloc(num_pixels))) {
		fperror("failed to allocate pixels");
		goto err;
	}

	for(i=0; i<num_pixels; i++) {
		long c = io->readchar(io);
		if(c == -1) {
			fprintf(stderr, "unexpected end of file while reading pixels\n");
			goto err;
		}
		gmap->pixels[i] = 255 * c / max_pixval;
		if(!greyscale) {
			io->readchar(io);
			io->readchar(io);
		}
	}

	gmap->xsz_shift = find_pow2(gmap->xsz);
	gmap->cstart = min_code;
	gmap->cend = max_code + 1;
	gmap->crange = gmap->cend - gmap->cstart;

	if(!(gmap->glyphs = calloc(gmap->crange, sizeof *gmap->glyphs))) {
		fperror("failed to allocate glyph info");
		goto err;
	}

	while(glyphs) {
		struct glyph *g = glyphs;
		glyphs = glyphs->next;

		gmap->glyphs[g->code - gmap->cstart] = *g;
		free(g);
	}
	return gmap;

err:
	dtx_free_glyphmap(gmap);
	while(glyphs) {
		void *tmp = glyphs;
		glyphs = glyphs->next;
		free(tmp);
	}
	return 0;
}

int dtx_save_glyphmap(const char *fname, const struct dtx_glyphmap *gmap)
{
	FILE *fp;
	int res;

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "%s: failed to open file: %s: %s\n", __func__, fname, strerror(errno));
		return -1;
	}
	res = dtx_save_glyphmap_stream(fp, gmap);
	fclose(fp);
	return res;
}

int dtx_save_glyphmap_stream(FILE *fp, const struct dtx_glyphmap *gmap)
{
	int i, num_pixels;
	struct glyph *g = gmap->glyphs;

	fprintf(fp, "P%d\n%d %d\n", opt_save_ppm ? 6 : 5, gmap->xsz, gmap->ysz);
	fprintf(fp, "# size: %d\n", gmap->ptsize);
	fprintf(fp, "# advance: %g\n", gmap->line_advance);
	fprintf(fp, "# baseline: %g\n", gmap->baseline);
	for(i=0; i<gmap->crange; i++) {
		fprintf(fp, "# %d: %gx%g+%g+%g o:%g,%g adv:%g\n", g->code + gmap->cstart,
				g->width, g->height, g->x, g->y, g->orig_x, g->orig_y, g->advance);
		g++;
	}
	fprintf(fp, "255\n");

	num_pixels = gmap->xsz * gmap->ysz;
	for(i=0; i<num_pixels; i++) {
		int c = gmap->pixels[i];
		fputc(c, fp);
		if(opt_save_ppm) {
			fputc(c, fp);
			fputc(c, fp);
		}
	}
	return 0;
}

void dtx_add_glyphmap(struct dtx_font *fnt, struct dtx_glyphmap *gmap)
{
	gmap->next = fnt->gmaps;
	fnt->gmaps = gmap;
}


void dtx_set(enum dtx_option opt, int val)
{
	switch(opt) {
	case DTX_PADDING:
		opt_padding = val;
		break;

	case DTX_SAVE_PPM:
		opt_save_ppm = val;
		break;

	default:
		dtx_gl_setopt(opt, val);
		dtx_rast_setopt(opt, val);
	}
}

int dtx_get(enum dtx_option opt)
{
	int val;

	switch(opt) {
	case DTX_PADDING:
		return opt_padding;

	case DTX_SAVE_PPM:
		return opt_save_ppm;

	default:
		break;
	}

	if(dtx_gl_getopt(opt, &val) != -1) {
		return val;
	}
	if(dtx_rast_getopt(opt, &val) != -1) {
		return val;
	}
	return -1;
}

void dtx_use_font(struct dtx_font *fnt, int sz)
{
	if(!dtx_drawchar) {
		dtx_target_opengl();
	}

	dtx_font = fnt;
	dtx_font_sz = sz;
}

float dtx_line_height(void)
{
	struct dtx_glyphmap *gmap = dtx_get_glyphmap(dtx_font, 0);

	return gmap->line_advance;
}

float dtx_baseline(void)
{
	struct dtx_glyphmap *gmap = dtx_get_glyphmap(dtx_font, 0);

	return gmap->baseline;
}

void dtx_glyph_box(int code, struct dtx_box *box)
{
	int cidx;
	struct dtx_glyphmap *gmap;
	gmap = dtx_get_font_glyphmap(dtx_font, dtx_font_sz, code);

	cidx = code - gmap->cstart;

	box->x = gmap->glyphs[cidx].orig_x;
	box->y = gmap->glyphs[cidx].orig_y;
	box->width = gmap->glyphs[cidx].width;
	box->height = gmap->glyphs[cidx].height;
}

float dtx_glyph_width(int code)
{
	struct dtx_box box;
	dtx_glyph_box(code, &box);
	return box.width;
}

float dtx_glyph_height(int code)
{
	struct dtx_box box;
	dtx_glyph_box(code, &box);
	return box.height;
}

void dtx_string_box(const char *str, struct dtx_box *box)
{
	dtx_substring_box(str, 0, INT_MAX, box);
}

void dtx_substring_box(const char *str, int start, int end, struct dtx_box *box)
{
	int code;
	float pos_x = 0.0f, pos_y = 0.0f;
	struct glyph *g = 0;
	float x0, y0, x1, y1;

	x0 = y0 = FLT_MAX;
	x1 = y1 = -FLT_MAX;

	/* skip start characters */
	while(*str && start > 0) {
		str = dtx_utf8_next_char((char*)str);
		--start;
		--end;
	}

	while(*str && --end >= 0) {
		float px, py;
		struct dtx_glyphmap *gmap;

		code = dtx_utf8_char_code(str);
		str = dtx_utf8_next_char((char*)str);

		px = pos_x;
		py = pos_y;

		if((gmap = dtx_proc_char(code, &pos_x, &pos_y))) {
			g = gmap->glyphs + code - gmap->cstart;

			if(px + g->orig_x < x0) {
				x0 = px + g->orig_x;
			}
			if(py - g->orig_y < y0) {
				y0 = py - g->orig_y;
			}
			if(px + g->orig_x + g->width > x1) {
				x1 = px + g->orig_x + g->width;
			}
			if(py - g->orig_y + g->height > y1) {
				y1 = py - g->orig_y + g->height;
			}
		}
	}

	box->x = x0;
	box->y = y0;
	box->width = x1 - x0;
	box->height = y1 - y0;
}

float dtx_string_width(const char *str)
{
	struct dtx_box box;

	dtx_string_box(str, &box);
	return box.width;
}

float dtx_string_height(const char *str)
{
	struct dtx_box box;

	dtx_string_box(str, &box);
	return box.height;
}

float dtx_char_pos(const char *str, int n)
{
	int i, code;
	float pos = 0.0;
	struct dtx_glyphmap *gmap;

	for(i=0; i<n; i++) {
		if(!*str) break;

		code = dtx_utf8_char_code(str);
		str = dtx_utf8_next_char((char*)str);

		if((gmap = dtx_get_font_glyphmap(dtx_font, dtx_font_sz, code))) {
			pos += gmap->glyphs[code - gmap->cstart].advance;
		}
	}
	return pos;
}

int dtx_char_at_pt(const char *str, float pt)
{
	int i;
	float prev_pos = 0.0f, pos = 0.0f;
	struct dtx_glyphmap *gmap;

	for(i=0; *str; i++) {
		int code = dtx_utf8_char_code(str);
		str = dtx_utf8_next_char((char*)str);

		if((gmap = dtx_get_font_glyphmap(dtx_font, dtx_font_sz, code))) {
			pos += gmap->glyphs[code - gmap->cstart].advance;

			if(fabs(pt - prev_pos) < fabs(pt - pos)) {
				break;
			}
		}
		prev_pos = pos;
	}
	return i;
}

struct dtx_glyphmap *dtx_proc_char(int code, float *xpos, float *ypos)
{
	struct dtx_glyphmap *gmap;
	gmap = dtx_get_font_glyphmap(dtx_font, dtx_font_sz, code);

	switch(code) {
	case '\n':
		*xpos = 0.0;
		if(gmap) {
			*ypos -= gmap->line_advance;
		}
		return 0;

	case '\t':
		if(gmap) {
			*xpos = (fmod(*xpos, 4.0) + 4.0) * gmap->glyphs[0].advance;
		}
		return 0;

	case '\r':
		*xpos = 0.0;
		return 0;

	default:
		break;
	}

	if(gmap) {
		*xpos += gmap->glyphs[code - gmap->cstart].advance;
	}
	return gmap;
}

#ifdef USE_FREETYPE
static int calc_best_size(int total_width, int max_gwidth, int max_gheight, int padding, int pow2, int *imgw, int *imgh)
{
	int xsz, ysz, num_rows;
	float aspect;

	/* the widest glyph won't fit in the maximum image size */
	if(max_gwidth > MAX_IMG_SIZE) {
		return -1;
	}

	for(xsz=2; xsz<=MAX_IMG_SIZE; xsz *= 2) {
		num_rows = total_width / xsz + 1;

		/* assume worst case, all last glyphs will float to the next line
		 * so let's add extra rows for that. */
		num_rows += (padding + (max_gwidth + padding) * num_rows + xsz - 1) / xsz;

		ysz = num_rows * (max_gheight + padding) + padding;
		if(ysz <= 0 || ysz > MAX_IMG_SIZE) continue;

		if(pow2) {
			ysz = next_pow2(ysz);
		}
		aspect = (float)xsz / (float)ysz;

		if(aspect >= 1.0) {
			break;
		}
	}

	if(xsz > MAX_IMG_SIZE || ysz > MAX_IMG_SIZE || ysz <= 0) {
		return -1;
	}

	*imgw = xsz;
	*imgh = ysz;
	return 0;
}


static int next_pow2(int x)
{
	x--;
	x = (x >> 1) | x;
	x = (x >> 2) | x;
	x = (x >> 4) | x;
	x = (x >> 8) | x;
	x = (x >> 16) | x;
	return x + 1;
}
#endif

static int find_pow2(int x)
{
	int i;
	for(i=0; i<sizeof x * CHAR_BIT; i++) {
		if((1 << i) == x) {
			return i;
		}
	}
	return 0;
}
