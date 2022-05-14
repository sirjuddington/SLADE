
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    OpenGL.cpp
// Description: OpenGL management stuff
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "OpenGL.h"
#include "Utility/Colour.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, gl_point_sprite, true, CVar::Flag::Save)
CVAR(Bool, gl_tweak_accuracy, true, CVar::Flag::Save)
CVAR(Bool, gl_vbo, true, CVar::Flag::Save)
CVAR(Int, gl_depth_buffer_size, 24, CVar::Flag::Save)
CVAR(Int, gl_version_major, 0, CVar::Flag::Save)
CVAR(Int, gl_version_minor, 0, CVar::Flag::Save)

namespace slade::gl
{
wxGLContext* context        = nullptr;
int          wx_gl_attrib[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, WX_GL_STENCIL_SIZE, 8, 0 };
bool         initialised    = false;
double       version        = 0;
unsigned     max_tex_size   = 128;
unsigned     pow_two[]      = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 };
uint8_t      n_pow_two      = 16;
float        max_point_size = -1.0f;
Blend        last_blend     = Blend::Normal;
Info         info;
} // namespace slade::gl


// -----------------------------------------------------------------------------
//
// OpenGL Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the global OpenGL context, and creates it if needed
// -----------------------------------------------------------------------------
wxGLContext* gl::getContext(wxGLCanvas* canvas)
{
	if (!context)
	{
		if (canvas->IsShown())
		{
			log::info("Setting up the OpenGL context");

			// Setup desired context attributes
			wxGLContextAttrs attr;
			if (gl_version_major > 0)
				attr.PlatformDefaults().CompatibilityProfile().OGLVersion(gl_version_major, gl_version_minor).EndList();
			else
				attr.PlatformDefaults().CompatibilityProfile().EndList();

			// Create context
			context = new wxGLContext(canvas, nullptr, &attr);
			if (!context->IsOK())
			{
				log::error("Failed to setup the OpenGL context");
				delete context;
				context = nullptr;
				return nullptr;
			}

			// Make current
			if (!context->SetCurrent(*canvas))
			{
				log::error("Failed to setup the OpenGL context");
				delete context;
				context = nullptr;
				return nullptr;
			}

			// Initialize OpenGL
			if (!init())
			{
				delete context;
				context = nullptr;
				return nullptr;
			}
		}
		else
			log::warning("Can't create global GL context, wxGLCanvas is hidden");
	}

	return context;
}

// -----------------------------------------------------------------------------
// Initialises general OpenGL variables and settings
// -----------------------------------------------------------------------------
bool gl::init()
{
	if (initialised)
		return true;

	log::info(1, "Initialising OpenGL...");

	// Initialise GLAD
	if (!gladLoadGL())
	{
		log::error("GLAD initialization failed");
		return false;
	}

	// Get OpenGL info
	info.vendor   = (const char*)glGetString(GL_VENDOR);
	info.renderer = (const char*)glGetString(GL_RENDERER);
	info.version  = (const char*)glGetString(GL_VERSION);
	// info.extensions = (const char*)glGetString(GL_EXTENSIONS);

	// Get OpenGL version
	string_view temp{ info.version.data(), 3 };
	strutil::toDouble(temp, version);
	log::info("OpenGL Version: {:1.1f}", version);

	// Get max texture size
	GLint val = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
	max_tex_size = val;
	log::info("Max Texture Size: {}x{}", max_tex_size, max_tex_size);

	// Test extensions
	log::info("Checking extensions...");
	if (GLAD_GL_ARB_vertex_buffer_object)
		log::info("Vertex Buffer Objects supported");
	else
		log::info("Vertex Buffer Objects not supported");
	if (GLAD_GL_ARB_point_sprite)
		log::info("Point Sprites supported");
	else
		log::info("Point Sprites not supported");
	if (GLAD_GL_ARB_framebuffer_object)
		log::info("Framebuffer Objects supported");
	else
		log::info("Framebuffer Objects not supported");

	initialised = true;
	return true;
}

// -----------------------------------------------------------------------------
// Returns true if the installed OpenGL version supports non-power-of-two
// textures, false otherwise
// -----------------------------------------------------------------------------
bool gl::np2TexSupport()
{
	return GLAD_GL_ARB_texture_non_power_of_two;
}

// -----------------------------------------------------------------------------
// Returns true if the installed OpenGL version supports point sprites, false
// otherwise
// -----------------------------------------------------------------------------
bool gl::pointSpriteSupport()
{
	return GLAD_GL_ARB_point_sprite && gl_point_sprite;
}

// -----------------------------------------------------------------------------
// Returns true if the installed OpenGL version supports vertex buffer objects,
// false otherwise
// -----------------------------------------------------------------------------
bool gl::vboSupport()
{
	return GLAD_GL_ARB_vertex_buffer_object && gl_vbo;
}

// -----------------------------------------------------------------------------
// Returns true if the installed OpenGL version supports framebuffer objects,
// false otherwise
// -----------------------------------------------------------------------------
bool gl::fboSupport()
{
	return GLAD_GL_ARB_framebuffer_object;
}

// -----------------------------------------------------------------------------
// Returns true if [dim] is a valid texture dimension on the system OpenGL
// version
// -----------------------------------------------------------------------------
bool gl::validTexDimension(unsigned dim)
{
	if (dim > max_tex_size)
		return false;
	else if (!np2TexSupport())
	{
		for (uint8_t a = 0; a < n_pow_two; a++)
		{
			if (dim == pow_two[a])
				return true;
		}

		return false;
	}
	else
		return true;
}

// -----------------------------------------------------------------------------
// Returns the implementation-dependant maximum size for GL_POINTS
// -----------------------------------------------------------------------------
float gl::maxPointSize()
{
	if (max_point_size < 0)
	{
		GLfloat sizes[2];
		glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, sizes);
		max_point_size = sizes[1];
	}

	return max_point_size;
}

// -----------------------------------------------------------------------------
// Returns the maximum texture size
// -----------------------------------------------------------------------------
unsigned gl::maxTextureSize()
{
	return max_tex_size;
}

// -----------------------------------------------------------------------------
// Returns true if OpenGL has been initialised
// -----------------------------------------------------------------------------
bool gl::isInitialised()
{
	return initialised;
}

// -----------------------------------------------------------------------------
// Returns true if the 'accuracy tweak' is enabled.
// This can fix inaccuracies when rendering 2d textures, but tends to cause
// fonts to blur when using FTGL
// -----------------------------------------------------------------------------
bool gl::accuracyTweak()
{
	return gl_tweak_accuracy;
}

// -----------------------------------------------------------------------------
// Returns the GL attributes array for use with wxGLCanvas
// -----------------------------------------------------------------------------
wxGLAttributes gl::getWxGLAttribs()
{
	wxGLAttributes attr;

	// Try 32bit depth buffer first
	attr.PlatformDefaults().MinRGBA(8, 8, 8, 8).DoubleBuffer().Depth(32).Stencil(8).EndList();
	if (wxGLCanvas::IsDisplaySupported(attr))
		return attr;

	// Then 24 bit depth buffer if not supported
	attr.Reset();
	attr.PlatformDefaults().MinRGBA(8, 8, 8, 8).DoubleBuffer().Depth(24).Stencil(8).EndList();
	if (wxGLCanvas::IsDisplaySupported(attr))
		return attr;

	// Then 16bit depth buffer (if this isn't supported then it's something else)
	attr.Reset();
	attr.PlatformDefaults().MinRGBA(8, 8, 8, 8).DoubleBuffer().Depth(16).Stencil(8).EndList();

	return attr;
}

// -----------------------------------------------------------------------------
// Sets the colour to [col], and changes the colour blend mode if needed and
// [set_blend] is true
// -----------------------------------------------------------------------------
void gl::setColour(const ColRGBA& col, Blend blend)
{
	// Colour
	glColor4ub(col.r, col.g, col.b, col.a);

	// Blend
	if (blend != Blend::Ignore && blend != last_blend)
	{
		if (blend == Blend::Normal)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else if (blend == Blend::Additive)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		last_blend = blend;
	}
}

// -----------------------------------------------------------------------------
// Sets the colour to [r,g,b,a], and changes the colour blend mode to [blend] if
// needed
// -----------------------------------------------------------------------------
void gl::setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a, Blend blend)
{
	// Colour
	glColor4ub(r, g, b, a);

	// Blend
	if (blend != Blend::Ignore && blend != last_blend)
	{
		if (blend == Blend::Normal)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else if (blend == Blend::Additive)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		last_blend = blend;
	}
}

// -----------------------------------------------------------------------------
// Sets the colour blend mode to [blend] if needed
// -----------------------------------------------------------------------------
void gl::setBlend(Blend blend)
{
	if (blend != Blend::Ignore && blend != last_blend)
	{
		if (blend == Blend::Normal)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else if (blend == Blend::Additive)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		last_blend = blend;
	}
}

// -----------------------------------------------------------------------------
// Resets colour blending to defaults
// -----------------------------------------------------------------------------
void gl::resetBlend()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	last_blend = Blend::Normal;
}

// -----------------------------------------------------------------------------
// Returns OpenGL system info
// -----------------------------------------------------------------------------
gl::Info gl::sysInfo()
{
	return info;
}
