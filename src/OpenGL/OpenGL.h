#pragma once

// OpenGL
// glew.h needs to be #included before gl/glu headers
// clang-format off
#ifdef __WXMSW__
// Windows GL headers
#include "thirdparty/glew/glew.h" // Use built-in GLEW so we don't need any extra dlls
#include <gl/GL.h>
#include <gl/GLU.h>
#elif __APPLE__
// OSX GL headers
#include <GL/glew.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
// Unix GL headers
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif
// clang-format on

#ifndef USE_SFML_RENDERWINDOW
#include <wx/glcanvas.h>
#undef None // Why does <X11/X.h> #define this? Idiotic
#endif

// Forward declarations
struct ColRGBA;

namespace OpenGL
{
enum class Blend
{
	Normal,
	Additive,

	Ignore
};

struct Info
{
	std::string vendor;
	std::string renderer;
	std::string version;
	std::string extensions;

	Info() { vendor = renderer = version = extensions = "OpenGL not initialised"; }
};

#ifndef USE_SFML_RENDERWINDOW
wxGLContext* getContext(wxGLCanvas* canvas);
#endif
bool     init();
bool     np2TexSupport();
bool     pointSpriteSupport();
bool     vboSupport();
bool     validTexDimension(unsigned dim);
float    maxPointSize();
unsigned maxTextureSize();
bool     isInitialised();
bool     accuracyTweak();
#ifndef USE_SFML_RENDERWINDOW
int* getWxGLAttribs();
#endif
void setColour(const ColRGBA& col, Blend blend = Blend::Ignore);
void setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, Blend blend = Blend::Ignore);
void setBlend(Blend blend);
void resetBlend();
Info sysInfo();
} // namespace OpenGL
