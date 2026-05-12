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
#ifndef DRAWTEXT_IMPL_H_
#define DRAWTEXT_IMPL_H_

#include "drawtext.h"

struct glyph {
	int code;
	float x, y, width, height;
	/* normalized coords [0, 1] */
	float nx, ny, nwidth, nheight;
	float orig_x, orig_y;
	float advance;
	struct glyph *next;
};

struct dtx_glyphmap {
	int ptsize;

	int xsz, ysz;
	unsigned int xsz_shift;
	unsigned char *pixels;
	unsigned int tex;
	int tex_valid;
	void *udata;

	int cstart, cend;	/* character range */
	int crange;

	float line_advance;
	float baseline;

	struct glyph *glyphs;
	struct dtx_glyphmap *next;
};

struct dtx_font {
	/* freetype FT_Face */
	void *face;

	/* list of glyphmaps */
	struct dtx_glyphmap *gmaps;

	/* last returned glyphmap (cache) */
	struct dtx_glyphmap *last_gmap;
};


struct dtx_font *dtx_font;
int dtx_font_sz;
int dtx_buf_mode;	/* DTX_NBF is 0 */
float dtx_cur_color[4];
int dtx_cur_color_int[4];
float dtx_cur_offset[2];

#define fperror(str) \
	fprintf(stderr, "%s: %s: %s\n", __func__, (str), strerror(errno))

#ifdef _MSC_VER
#define __func__	__FUNCTION__
#endif

/* returns zero if it should NOT be printed and modifies xpos/ypos */
/* implemented in font.c */
struct dtx_glyphmap *dtx_proc_char(int code, float *xpos, float *ypos);

const char *(*dtx_drawchar)(const char*, float*, float*, int*);
void (*dtx_drawflush)(void);

int dtx_gl_setopt(enum dtx_option opt, int val);
int dtx_gl_getopt(enum dtx_option opt, int *ret);
int dtx_rast_setopt(enum dtx_option opt, int val);
int dtx_rast_getopt(enum dtx_option opt, int *ret);

#endif	/* DRAWTEXT_IMPL_H_ */
