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
#include "drawtext.h"
#include "drawtext_impl.h"

struct quad {
	struct dtx_vertex v[6];
};

struct dtx_glyphmap *cur_gmap;

#define QBUF_SZ		512
static struct quad *qbuf;
static int num_quads;

static dtx_user_draw_func user_draw_func;
static void *user_cls;

static int dtx_draw_init(void);
static void cleanup(void);

static void set_glyphmap_texture(struct dtx_glyphmap *gmap);
static const char *drawchar(const char *str, float *pos_x, float *pos_y, int *should_flush);
static void flush_user(void);
static void add_glyph(struct glyph *g, float x, float y);


#ifndef NO_OPENGL
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef TARGET_IPHONE
#include <OpenGLES/ES2/gl.h>
#ifndef GL_ES
#define GL_ES
#endif

#elif defined(ANDROID)
#include <GLES2/gl2.h>
#ifndef GL_ES
#define GL_ES
#endif

#else	/* regular OpenGL */

#ifdef WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>

#else

#define GL_GLEXT_LEGACY		/* don't include glext.h internally in gl.h */
#include <GL/gl.h>
#ifndef NO_GLU
#include <GL/glu.h>
#endif

#ifdef __unix__
#define GLX_GLXEXT_LEGACY	/* don't include glxext.h internally in glx.h */
#include <GL/glx.h>
#endif

#endif	/* !__APPLE__ */
#endif	/* !TARGET_IPHONE */

#ifdef GL_ES
#define GL_CLAMP GL_CLAMP_TO_EDGE
#endif

static void dtx_gl_init(void);
static void cleanup(void);
static void flush(void);

static int vattr = -1;
static int tattr = -1;
static int cattr = -1;
static unsigned int font_tex;

#ifndef GL_ES
#ifndef GL_VERSION_1_5
#define GL_ARRAY_BUFFER 0x8892
#define GL_ARRAY_BUFFER_BINDING 0x8894
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
static PFNGLBINDBUFFERPROC glBindBuffer;
#endif

#ifndef GL_VERSION_2_0
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (APIENTRY *PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
#endif

#ifdef WIN32
#define load_glfunc(s)	wglGetProcAddress(s)
#elif defined(__unix__)
#define load_glfunc(s)	glXGetProcAddress((unsigned char*)s)
#endif

#endif	/* !GL_ES */

void dtx_target_opengl(void)
{
	dtx_draw_init();
	dtx_drawchar = drawchar;
	dtx_drawflush = flush;

	user_draw_func = 0;
}

int dtx_gl_setopt(enum dtx_option opt, int val)
{
	switch(opt) {
	case DTX_GL_ATTR_VERTEX:
		vattr = val;
		break;
	case DTX_GL_ATTR_TEXCOORD:
		tattr = val;
		break;
	case DTX_GL_ATTR_COLOR:
		cattr = val;
		break;
	default:
		return -1;
	}
	return 0;
}

int dtx_gl_getopt(enum dtx_option opt, int *res)
{
	switch(opt) {
	case DTX_GL_ATTR_VERTEX:
		*res = vattr;
		break;
	case DTX_GL_ATTR_TEXCOORD:
		*res = tattr;
		break;
	case DTX_GL_ATTR_COLOR:
		*res = cattr;
		break;
	default:
		return -1;
	}
	return 0;
}

static void dtx_gl_init(void)
{
#ifndef GL_ES
#ifndef GL_VERSION_1_5
	glBindBuffer = (PFNGLBINDBUFFERPROC)load_glfunc("glBindBuffer");
#endif
#ifndef GL_VERSION_2_0
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)load_glfunc("glEnableVertexAttribArray");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)load_glfunc("glDisableVertexAttribArray");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)load_glfunc("glVertexAttribPointer");
#endif
#endif	/* !GL_ES */
}


void dtx_vertex_attribs(int vert_attr, int tex_attr)
{
	vattr = vert_attr;
	tattr = tex_attr;
}

static void set_glyphmap_texture_gl(struct dtx_glyphmap *gmap)
{
	if(!gmap->tex) {
		glGenTextures(1, &gmap->tex);
		glBindTexture(GL_TEXTURE_2D, gmap->tex);
#if !defined(GL_ES) && defined(NO_GLU)
		/* TODO: ideally we want to have mipmaps even without GLU, and we should
		 * just use SGIS_generate_mipmaps if available
		 */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#endif
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		gmap->tex_valid = 0;
	}

	if(!gmap->tex_valid) {
		glBindTexture(GL_TEXTURE_2D, gmap->tex);
#ifdef GL_ES
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, gmap->xsz, gmap->ysz, 0, GL_ALPHA, GL_UNSIGNED_BYTE, gmap->pixels);
		glGenerateMipmap(GL_TEXTURE_2D);
#elif !defined(NO_GLU)
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_ALPHA, gmap->xsz, gmap->ysz, GL_ALPHA, GL_UNSIGNED_BYTE, gmap->pixels);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, gmap->xsz, gmap->ysz, 0, GL_ALPHA, GL_UNSIGNED_BYTE, gmap->pixels);
#endif
		gmap->tex_valid = 1;
	}

	if(font_tex != gmap->tex) {
		dtx_flush();
	}
	font_tex = gmap->tex;
}



static void flush(void)
{
	int vbo;

	if(!num_quads) {
		return;
	}

	if(glBindBuffer) {
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

#ifndef GL_ES
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
#endif
	glBindTexture(GL_TEXTURE_2D, font_tex);

	if(vattr != -1 && glEnableVertexAttribArray) {
		glEnableVertexAttribArray(vattr);
		glVertexAttribPointer(vattr, 2, GL_FLOAT, 0, sizeof(struct dtx_vertex), qbuf);
#ifndef GL_ES
	} else {
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, sizeof(struct dtx_vertex), qbuf);
#endif
	}
	if(tattr != -1 && glEnableVertexAttribArray) {
		glEnableVertexAttribArray(tattr);
		glVertexAttribPointer(tattr, 2, GL_FLOAT, 0, sizeof(struct dtx_vertex), &qbuf->v[0].s);
#ifndef GL_ES
	} else {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(struct dtx_vertex), &qbuf->v[0].s);
#endif
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthMask(0);

	glDrawArrays(GL_TRIANGLES, 0, num_quads * 6);

	glDepthMask(1);

	if(vattr != -1 && glDisableVertexAttribArray) {
		glDisableVertexAttribArray(vattr);
#ifndef GL_ES
	} else {
		glDisableClientState(GL_VERTEX_ARRAY);
#endif
	}
	if(tattr != -1 && glDisableVertexAttribArray) {
		glDisableVertexAttribArray(tattr);
#ifndef GL_ES
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
	}

#ifndef GL_ES
	glPopAttrib();
#else
	glDisable(GL_BLEND);
#endif

	if(glBindBuffer && vbo) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}

	num_quads = 0;
}
#else

/* no-opengl build, define all public gl functions as stubs */
void dtx_target_opengl(void) {}
int dtx_gl_setopt(enum dtx_option opt, int val) { return -1; }
int dtx_gl_getopt(enum dtx_option opt, int *val) { return -1; }

static void set_glyphmap_texture_gl(struct dtx_glyphmap *gmap) {}

#endif	/* !def NO_OPENGL */

static int dtx_draw_init(void)
{
	if(qbuf) {
		return 0;	/* already initialized */
	}

#ifndef NO_OPENGL
	dtx_gl_init();
#endif

	if(!(qbuf = malloc(QBUF_SZ * sizeof *qbuf))) {
		return -1;
	}
	num_quads = 0;

	atexit(cleanup);
	return 0;
}

static void cleanup(void)
{
	free(qbuf);
}


void dtx_target_user(dtx_user_draw_func func, void *cls)
{
	dtx_draw_init();

	user_draw_func = func;
	user_cls = cls;

	dtx_drawchar = drawchar;
	dtx_drawflush = flush_user;
}

void dtx_glyph(int code)
{
	struct dtx_glyphmap *gmap;

	if(!dtx_font || !(gmap = dtx_get_font_glyphmap(dtx_font, dtx_font_sz, code))) {
		return;
	}
	set_glyphmap_texture(gmap);

	add_glyph(gmap->glyphs + code - gmap->cstart, 0, 0);
	dtx_flush();
}

static void set_glyphmap_texture(struct dtx_glyphmap *gmap)
{
	if(!user_draw_func) {
		set_glyphmap_texture_gl(gmap);
	}

	if(cur_gmap && gmap != cur_gmap) {
		dtx_flush();
	}
	cur_gmap = gmap;
}

static const char *drawchar(const char *str, float *pos_x, float *pos_y, int *should_flush)
{
	struct dtx_glyphmap *gmap;
	float px, py;
	int code = dtx_utf8_char_code(str);
	str = dtx_utf8_next_char((char*)str);

	if(dtx_buf_mode == DTX_LBF && code == '\n') {
		*should_flush = 1;
	}

	px = *pos_x;
	py = *pos_y;

	if((gmap = dtx_proc_char(code, pos_x, pos_y))) {
		int idx = code - gmap->cstart;

		set_glyphmap_texture(gmap);
		add_glyph(gmap->glyphs + idx, px, py);
	}
	return str;
}

static void qvertex(struct dtx_vertex *v, float x, float y, float s, float t)
{
	v->x = x;
	v->y = y;
	v->s = s;
	v->t = t;
}

static void add_glyph(struct glyph *g, float x, float y)
{
	struct quad *qptr = qbuf + num_quads;

	x -= g->orig_x;
	y -= g->orig_y;

	qvertex(qptr->v + 0, x, y, g->nx, g->ny + g->nheight);
	qvertex(qptr->v + 1, x + g->width, y, g->nx + g->nwidth, g->ny + g->nheight);
	qvertex(qptr->v + 2, x + g->width, y + g->height, g->nx + g->nwidth, g->ny);

	qvertex(qptr->v + 3, x, y, g->nx, g->ny + g->nheight);
	qvertex(qptr->v + 4, x + g->width, y + g->height, g->nx + g->nwidth, g->ny);
	qvertex(qptr->v + 5, x, y + g->height, g->nx, g->ny);

	if(++num_quads >= QBUF_SZ) {
		dtx_flush();
	}
}

static void flush_user(void)
{
	struct dtx_pixmap pixmap;

	if(!num_quads || !user_draw_func || !cur_gmap) {
		return;
	}

	pixmap.pixels = cur_gmap->pixels;
	pixmap.width = cur_gmap->xsz;
	pixmap.height = cur_gmap->ysz;
	pixmap.udata = cur_gmap->udata;

	user_draw_func((struct dtx_vertex*)qbuf, num_quads * 6, &pixmap, user_cls);
	cur_gmap->udata = pixmap.udata;

	num_quads = 0;
}
