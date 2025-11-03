#pragma once

// OpenGL
// glad.h replaces gl.h, still need glu.h
// clang-format off
#ifdef __APPLE__
// OSX GL headers
#include "thirdparty/glad/include/glad/glad.h"
#include <OpenGL/glu.h>
#else
// Windows/Unix GL headers
#include "thirdparty/glad/include/glad/glad.h"
#include <GL/glu.h>
#endif
// clang-format on

#include <wx/glcanvas.h>
#undef None // Why does <X11/X.h> #define this? Idiotic

namespace slade
{
// Forward declarations
struct ColRGBA;

namespace gl
{
	enum class Blend
	{
		Normal,
		Additive,

		Ignore
	};

	struct Info
	{
		string vendor;
		string renderer;
		string version;

		Info() { vendor = renderer = version = "OpenGL not initialised"; }
	};

	wxGLContext*   getContext(wxGLCanvas* canvas);
	bool           init();
	bool           np2TexSupport();
	bool           pointSpriteSupport();
	bool           vboSupport();
	bool           fboSupport();
	bool           validTexDimension(unsigned dim);
	float          maxPointSize();
	unsigned       maxTextureSize();
	bool           isInitialised();
	bool           accuracyTweak();
	wxGLAttributes getWxGLAttribs();
	void           setColour(const ColRGBA& col, Blend blend = Blend::Ignore);
	void           setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, Blend blend = Blend::Ignore);
	void           setBlend(Blend blend);
	void           resetBlend();
	Info           sysInfo();
} // namespace gl
} // namespace slade
