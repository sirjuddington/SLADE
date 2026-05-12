/*
libdrawtext - a simple library for fast text rendering in OpenGL
Copyright (C) 2011-2016  John Tsiombikas <nuclear@member.fsf.org>

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drawtext.h"
#include "drawtext_impl.h"

static const char *drawchar(const char *str, float *xpos, float *ypos, int *should_flush);
static void flush(void);
static void draw_glyph(struct glyph *g, float x, float y);

static unsigned char *fb_pixels;
static int fb_width, fb_height;
static struct dtx_glyphmap *gmap;
static int threshold = -1;
static int use_alpha;

void dtx_target_raster(unsigned char *pixels, int width, int height)
{
	fb_pixels = pixels;
	fb_width = width;
	fb_height = height;
	dtx_drawchar = drawchar;
	dtx_drawflush = flush;
}

int dtx_rast_setopt(enum dtx_option opt, int val)
{
	switch(opt) {
	case DTX_RASTER_THRESHOLD:
		threshold = val;
		break;
	case DTX_RASTER_BLEND:
		use_alpha = val;
		break;
	default:
		return -1;
	}
	return 0;
}

int dtx_rast_getopt(enum dtx_option opt, int *res)
{
	switch(opt) {
	case DTX_RASTER_THRESHOLD:
		*res = threshold;
		break;
	case DTX_RASTER_BLEND:
		*res = use_alpha ? 1 : 0;
		break;
	default:
		return -1;
	}
	return 0;
}

static const char *drawchar(const char *str, float *xpos, float *ypos, int *should_flush)
{
	float px, py;
	int code = dtx_utf8_char_code(str);
	str = dtx_utf8_next_char((char*)str);

	*should_flush = 0;	/* the raster renderer never buffers output */

	px = *xpos;
	py = *ypos;

	if((gmap = dtx_proc_char(code, xpos, ypos))) {
		int idx = code - gmap->cstart;
		draw_glyph(gmap->glyphs + idx, px, py);
	}
	return str;
}

static void flush(void)
{
}

static void blit_opaque(unsigned char *dest, unsigned char *src, int xsz, int ysz)
{
	int i, j;
	int *col = dtx_cur_color_int;

	for(i=0; i<ysz; i++) {
		for(j=0; j<xsz; j++) {
			int val = src[j];
			*dest++ = val * col[0] / 255;
			*dest++ = val * col[1] / 255;
			*dest++ = val * col[2] / 255;
			*dest++ = val;
		}
		dest += (fb_width - xsz) * 4;
		src += gmap->xsz;
	}
}

static void blit_thres(unsigned char *dest, unsigned char *src, int xsz, int ysz)
{
	int i, j;
	int *col = dtx_cur_color_int;

	for(i=0; i<ysz; i++) {
		for(j=0; j<xsz; j++) {
			int val = src[j];
			if(val > threshold) {
				*dest++ = col[0];
				*dest++ = col[1];
				*dest++ = col[2];
				*dest++ = col[3];
			} else {
				dest += 4;
			}
		}
		dest += (fb_width - xsz) * 4;
		src += gmap->xsz;
	}
}

static void blit_blend(unsigned char *dest, unsigned char *src, int xsz, int ysz)
{
	int i, j, k;
	int *col = dtx_cur_color_int;

	for(i=0; i<ysz; i++) {
		for(j=0; j<xsz; j++) {
			int alpha = src[j];
			int inv_alpha = 255 - alpha;

			for(k=0; k<4; k++) {
				dest[k] = (col[k] * alpha + dest[k] * inv_alpha) / 255;
			}
			dest += 4;
		}
		dest += (fb_width - xsz) * 4;
		src += gmap->xsz;
	}
}

static void draw_glyph(struct glyph *g, float x, float y)
{
	unsigned char *dest, *src;
	int gx = (int)g->x;
	int gy = (int)g->y;
	int gwidth = (int)g->width;
	int gheight = (int)g->height;
	int ix = (int)(x - g->orig_x);
	int iy = (int)(y - gheight + g->orig_y);

	if(ix >= fb_width || iy >= fb_height)
		return;

	if(ix < 0) {
		gwidth += ix;
		gx -= ix;
		ix = 0;
	}
	if(iy < 0) {
		gheight += iy;
		gy -= iy;
		iy = 0;
	}
	if(ix + gwidth >= fb_width) {
		gwidth = fb_width - ix;
	}
	if(iy + gheight >= fb_height) {
		gheight = fb_height - iy;
	}

	if(gwidth <= 0 || gheight <= 0)
		return;

	dest = fb_pixels + (iy * fb_width + ix) * 4;
	src = gmap->pixels + gy * gmap->xsz + gx;

	if(use_alpha) {
		blit_blend(dest, src, gwidth, gheight);
	} else if(threshold > 0) {
		blit_thres(dest, src, gwidth, gheight);
	} else {
		blit_opaque(dest, src, gwidth, gheight);
	}
}

