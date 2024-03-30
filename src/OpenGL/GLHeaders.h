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
