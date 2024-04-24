/*
libdrawtext - a simple library for fast text rendering in OpenGL
Copyright (C) 2011-2019  John Tsiombikas <nuclear@member.fsf.org>

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
#ifndef LIBDRAWTEXT_H_
#define LIBDRAWTEXT_H_

#include <stdio.h>
#include <stdlib.h>

struct dtx_font;
struct dtx_glyphmap;

/* draw buffering modes */
enum {
	DTX_NBF = 0,/* unbuffered */
	DTX_LBF,	/* line buffered */
	DTX_FBF		/* fully buffered */
};

/* glyphmap resize filtering */
enum {
	DTX_NEAREST,
	DTX_LINEAR
};

struct dtx_box {
	float x, y;
	float width, height;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Open a truetype/opentype/whatever font.
 *
 * If sz is non-zero, the default ASCII glyphmap at the requested point size is
 * automatically created as well, and ready to use.
 *
 * To use other unicode ranges and different font sizes you must first call
 * dtx_prepare or dtx_prepare_range before issuing any drawing calls, otherwise
 * nothing will be rendered.
 */
struct dtx_font *dtx_open_font(const char *fname, int sz);
/* same as dtx_open_font, but open from a memory buffer instead of a file */
struct dtx_font *dtx_open_font_mem(void *ptr, int memsz, int fontsz);
/* open a font by loading a precompiled glyphmap (see: dtx_save_glyphmap)
 * this works even when compiled without freetype support
 */
struct dtx_font *dtx_open_font_glyphmap(const char *fname);
/* same as dtx_open_font_glyphmap, but open from a memory buffer instead of a file */
struct dtx_font *dtx_open_font_glyphmap_mem(void *ptr, int memsz);
/* close a font opened by either of the above */
void dtx_close_font(struct dtx_font *fnt);

/* prepare an ASCII glyphmap for the specified font size */
void dtx_prepare(struct dtx_font *fnt, int sz);
/* prepare an arbitrary unicode range glyphmap for the specified font size */
void dtx_prepare_range(struct dtx_font *fnt, int sz, int cstart, int cend);

/* convert all glyphmaps to distance fields for use with the distance field
 * font rendering algorithm. This is a convenience function which calls
 * dtx_calc_glyphmap_distfield and
 * dtx_resize_glyphmap(..., scale_numer, scale_denom, DTX_LINEAR) for each
 * glyphmap in this font.
 */
int dtx_calc_font_distfield(struct dtx_font *fnt, int scale_numer, int scale_denom);

/* Finds the glyphmap that contains the specified character code and matches the requested size
 * Returns null if it hasn't been created (you should call dtx_prepare/dtx_prepare_range).
 */
struct dtx_glyphmap *dtx_get_font_glyphmap(struct dtx_font *fnt, int sz, int code);

/* Finds the glyphmap that contains the specified unicode range and matches the requested font size
 * Will automatically generate one if it can't find it.
 */
struct dtx_glyphmap *dtx_get_font_glyphmap_range(struct dtx_font *fnt, int sz, int cstart, int cend);

/* returns the number of glyphmaps in this font */
int dtx_get_num_glyphmaps(struct dtx_font *fnt);
/* returns the Nth glyphmap of this font */
struct dtx_glyphmap *dtx_get_glyphmap(struct dtx_font *fnt, int idx);

/* Creates and returns a glyphmap for a particular unicode range and font size.
 * The generated glyphmap is added to the font's list of glyphmaps.
 */
struct dtx_glyphmap *dtx_create_glyphmap_range(struct dtx_font *fnt, int sz, int cstart, int cend);
/* free a glyphmap */
void dtx_free_glyphmap(struct dtx_glyphmap *gmap);

/* converts a glyphmap to a distance field glyphmap, for use with the distance
 * field font rendering algorithm.
 *
 * It is recommended to use a fairly large font size glyphmap for this, and
 * then shrink the resulting distance field glyphmap as needed, with
 * dtx_resize_glyphmap
 */
int dtx_calc_glyphmap_distfield(struct dtx_glyphmap *gmap);

/* resize a glyphmap by the provided scale factor fraction snum/sdenom
 * in order to maintain the power of 2 invariant, scaling fractions are only
 * allowed to be of the form 1/x or x/1, where x is a power of 2
 */
int dtx_resize_glyphmap(struct dtx_glyphmap *gmap, int snum, int sdenom, int filter);

/* returns a pointer to the raster image of a glyphmap (1 byte per pixel grayscale) */
unsigned char *dtx_get_glyphmap_image(struct dtx_glyphmap *gmap);
/* returns the width of the glyphmap image in pixels */
int dtx_get_glyphmap_width(struct dtx_glyphmap *gmap);
/* returns the height of the glyphmap image in pixels */
int dtx_get_glyphmap_height(struct dtx_glyphmap *gmap);
/* returns the point size represented by this glyphmap */
int dtx_get_glyphmap_ptsize(struct dtx_glyphmap *gmap);

/* The following functions can be used even when the library is compiled without
 * freetype support.
 */
struct dtx_glyphmap *dtx_load_glyphmap(const char *fname);
struct dtx_glyphmap *dtx_load_glyphmap_stream(FILE *fp);
struct dtx_glyphmap *dtx_load_glyphmap_mem(void *ptr, int memsz);
int dtx_save_glyphmap(const char *fname, const struct dtx_glyphmap *gmap);
int dtx_save_glyphmap_stream(FILE *fp, const struct dtx_glyphmap *gmap);

/* adds a glyphmap to a font */
void dtx_add_glyphmap(struct dtx_font *fnt, struct dtx_glyphmap *gmap);


/* ---- options and flags ---- */
enum dtx_option {
	/* options for the OpenGL renderer */
	DTX_GL_ATTR_VERTEX,   /* vertex attribute location     (default: -1 for standard gl_Vertex) */
	DTX_GL_ATTR_TEXCOORD, /* texture uv attribute location (default: -1 for gl_MultiTexCoord0) */
	DTX_GL_ATTR_COLOR,	  /* color attribute location      (default: -1 for gl_Color) */
	/* options for the raster renderer */
	DTX_RASTER_THRESHOLD, /* opaque/transparent threshold  (default: -1. fully opaque glyphs) */
	DTX_RASTER_BLEND,     /* glyph alpha blending (0 or 1) (default: 0 (off)) */

	/* generic options */
	DTX_PADDING = 128,    /* padding between glyphs in pixels (default: 8) */
	DTX_SAVE_PPM,         /* let dtx_save_glyphmap* save PPM instead of PGM (0 or 1) (default: 0 (PGM)) */

	DTX_FORCE_32BIT_ENUM = 0x7fffffff	/* this is not a valid option */
};

void dtx_set(enum dtx_option opt, int val);
int dtx_get(enum dtx_option opt);


/* ---- rendering ---- */

/* the dtx_target_ functions select which rendering mode to use.
 * default: opengl
 */
void dtx_target_opengl(void);
/* pixels are expected to be RGBA ordered bytes, 4 per pixel
 * text is rendered with pre-multiplied alpha
 */
void dtx_target_raster(unsigned char *pixels, int width, int height);


/* data structures passed to user-supplied draw callback */
struct dtx_vertex { float x, y, s, t; };
struct dtx_pixmap {
	unsigned char *pixels;	/* pixel buffer pointer (8 bits per pixel) */
	int width, height;		/* dimensions of the pixel buffer */
	void *udata;	/* user-supplied pointer to data associated with this
					 * pixmap. On the first callback invocation this pointer
					 * will be null. The user may set it to associate any extra
					 * data to this pixmap (such as texture structures or
					 * identifiers). Libdrawtext will never modify this pointer.
					 */
};

/* user-defined glyph drawing callback type (set with dtx_target_user)
 * It's called when the output buffer is flushed, with a pointer to the vertex
 * buffer that needs to be drawn (every 3 vertices forming a triangle), the
 * number of vertices in the buffer, and a pointer to the current glyphmap
 * atlas pixmap (see struct dtx_pixmap above).
 */
typedef void (*dtx_user_draw_func)(struct dtx_vertex *v, int vcount,
		struct dtx_pixmap *pixmap, void *cls);

/* set user-supplied draw callback and optional closure pointer, which will
 * be passed unchanged as the last argument on every invocation of the draw
 * callback.
 */
void dtx_target_user(dtx_user_draw_func drawfunc, void *cls);


/* position of the origin of the first character to be printed */
void dtx_position(float x, float y);
/* TODO currently only used by the raster renderer, implement in gl too */
void dtx_color(float r, float g, float b, float a);

/* before drawing anything this function must set the font to use */
void dtx_use_font(struct dtx_font *fnt, int sz);

/* sets the buffering mode
 * - DTX_NBUF: every call to dtx_string gets rendered immediately.
 * - DTX_LBUF: renders when the buffer is full or the string contains a newline.
 * - DTX_FBUF: renders only when the buffer is full (you must call dtx_flush explicitly).
 */
void dtx_draw_buffering(int mode);

/* Sets the vertex attribute indices to use for passing vertex and texture coordinate
 * data. By default both are -1 which means you don't have to use a shader, and if you
 * do they are accessible through gl_Vertex and gl_MultiTexCoord0, as usual.
 *
 * NOTE: If you are using OpenGL ES 2.x or OpenGL >= 3.1 core (non-compatibility)
 * context you must specify valid attribute indices.
 *
 * NOTE2: equivalent to:
 *    dtx_set(DTX_GL_ATTR_VERTEX, vert_attr);
 *    dtx_set(DTX_GL_ATTR_TEXCOORD, tex_attr);
 */
void dtx_vertex_attribs(int vert_attr, int tex_attr);

/* draws a single glyph at the origin */
void dtx_glyph(int code);
/* draws a utf-8 string starting at the origin. \n \r and \t are handled appropriately. */
void dtx_string(const char *str);
void dtx_substring(const char *str, int start, int end);

void dtx_printf(const char *fmt, ...);

/* render any pending glyphs (see dtx_draw_buffering) */
void dtx_flush(void);


/* ---- encodings ---- */

/* returns a pointer to the next character in a utf-8 stream */
char *dtx_utf8_next_char(char *str);

/* returns a pointer to the previous character in a utf-8 stream */
char *dtx_utf8_prev_char(char *ptr, char *first);

/* returns the unicode character codepoint of the utf-8 character starting at str */
int dtx_utf8_char_code(const char *str);

/* returns the number of bytes of the utf-8 character starting at str */
int dtx_utf8_nbytes(const char *str);

/* returns the number of utf-8 characters in a zero-terminated utf-8 string */
int dtx_utf8_char_count(const char *str);

/* returns the number of utf-8 characters in the next N bytes starting from str */
int dtx_utf8_char_count_range(const char *str, int nbytes);

/* Converts a unicode code-point to a utf-8 character by filling in the buffer
 * passed at the second argument, and returns the number of bytes taken by that
 * utf-8 character.
 * It's valid to pass a null buffer pointer, in which case only the byte count is
 * returned (useful to figure out how much memory to allocate for a buffer).
 */
size_t dtx_utf8_from_char_code(int code, char *buf);

/* Converts a unicode utf-16 wchar_t string to utf-8, filling in the buffer passed
 * at the second argument. Returns the size of the resulting string in bytes.
 *
 * It's valid to pass a null buffer pointer, in which case only the size gets
 * calculated and returned, which is useful for figuring out how much memory to
 * allocate for the utf-8 buffer.
 */
size_t dtx_utf8_from_string(const wchar_t *str, char *buf);


/* ---- metrics ---- */
float dtx_line_height(void);
float dtx_baseline(void);

/* rendered dimensions of a single glyph */
void dtx_glyph_box(int code, struct dtx_box *box);
float dtx_glyph_width(int code);
float dtx_glyph_height(int code);

/* rendered dimensions of a string */
void dtx_string_box(const char *str, struct dtx_box *box);
void dtx_substring_box(const char *str, int start, int end, struct dtx_box *box);
float dtx_string_width(const char *str);
float dtx_string_height(const char *str);

/* returns the horizontal position of the n-th character of the rendered string
 * (useful for placing cursors)
 */
float dtx_char_pos(const char *str, int n);

int dtx_char_at_pt(const char *str, float pt);

#ifdef __cplusplus
}
#endif

#endif	/* LIBDRAWTEXT_H_ */
