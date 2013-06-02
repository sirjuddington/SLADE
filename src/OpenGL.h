
#ifndef __OPENGL_H__
#define __OPENGL_H__

#include <wx/glcanvas.h>

namespace OpenGL
{
	wxGLContext*	getContext(wxGLCanvas* canvas);
	bool			init();
	bool			np2TexSupport();
	bool			pointSpriteSupport();
	bool			vboSupport();
	bool			validTexDimension(unsigned dim);
	float			maxPointSize();
	unsigned		maxTextureSize();
	bool			isInitialised();
	bool			accuracyTweak();
	int*			getWxGLAttribs();
}

#endif//__OPENGL_H__
