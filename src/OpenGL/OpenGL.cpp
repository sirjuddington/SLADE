
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
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, gl_depth_buffer_size, 24, CVar::Flag::Save)
CVAR(Int, gl_version_major, 0, CVar::Flag::Save)
CVAR(Int, gl_version_minor, 0, CVar::Flag::Save)
CVAR(Int, gl_msaa, 2, CVar::Flag::Save)
CVAR(Bool, gl_debug, false, CVar::Flag::Save)

namespace slade::gl
{
wxGLContext*     context      = nullptr;
bool             initialised  = false;
double           version      = 0;
unsigned         max_tex_size = 128;
vector<unsigned> pow_two      = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 };
Blend            last_blend   = Blend::Normal;
int              msaa         = -1;
Info             info;
unsigned         vbo_current;
unsigned         vao_current;
unsigned         ebo_current;
unsigned         drawcall_count = 0;
} // namespace slade::gl


// -----------------------------------------------------------------------------
//
// OpenGL Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::gl
{
wxGLAttributes& buildGlAttr(wxGLAttributes& attr, int depth)
{
	if (msaa < 0)
		msaa = gl_msaa;

	attr.Reset();
	attr.PlatformDefaults().MinRGBA(8, 8, 8, 8).DoubleBuffer().Depth(depth).Stencil(8);

	if (msaa > 1)
		attr.Samplers(msaa);

	attr.EndList();
	return attr;
}

void GLAPIENTRY glMessageCallback(
	GLenum        source,
	GLenum        type,
	GLuint        id,
	GLenum        severity,
	GLsizei       length,
	const GLchar* message,
	const void*   userParam)
{
	if (type == GL_DEBUG_TYPE_ERROR)
		log::error("OpenGL Error: {}", message);
	// else
	//	log::info("OpenGL: {}", message);
}

wxGLContext* createBestContext(wxGLCanvas* canvas)
{
	// Try OpenGL 4.6 -> 4.0
	int v_minor = 6;
	while (v_minor >= 0)
	{
		wxGLContextAttrs attr;
		attr.PlatformDefaults().CoreProfile().OGLVersion(4, v_minor).EndList();
		if (auto ctx = new wxGLContext(canvas, nullptr, &attr); ctx->IsOK())
			return ctx;
		else
			delete ctx;
	}

	// No 4.0, try 3.3 (minimum)
	wxGLContextAttrs attr;
	attr.PlatformDefaults().CoreProfile().OGLVersion(3, 3).EndList();
	auto ctx = new wxGLContext(canvas, nullptr, &attr);
	if (ctx->IsOK())
		return ctx;

	// Unable to create core context
	delete ctx;
	return nullptr;
}
} // namespace slade::gl

// -----------------------------------------------------------------------------
// Returns the global OpenGL context, and creates it if needed
// -----------------------------------------------------------------------------
wxGLContext* gl::getContext(wxGLCanvas* canvas)
{
	if (!context && canvas)
	{
		if (canvas->IsShown())
		{
			log::info("Setting up the OpenGL context");

			// Create context with requested gl version first (if any)
			if (gl_version_major > 0)
			{
				wxGLContextAttrs attr;
				attr.PlatformDefaults().CoreProfile().OGLVersion(gl_version_major, gl_version_minor).EndList();
				context = new wxGLContext(canvas, nullptr, &attr);
				if (!context->IsOK())
				{
					// Context creation failed
					delete context;
					context = nullptr;
				}
			}

			// Create core profile context with max supported GL version
			if (!context)
				context = createBestContext(canvas);

			// Check created context is valid
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

	// Get OpenGL version
	// string_view temp{ info.version.data(), 3 };
	// strutil::toDouble(temp, version);
	log::info("OpenGL Version: {}", info.version);

	// Get max texture size
	GLint val = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
	max_tex_size = val;
	log::info("Max Texture Size: {}x{}", max_tex_size, max_tex_size);

	// Test extensions
	log::info("Checking extensions...");
	if (GLAD_GL_ARB_framebuffer_object)
		log::info("Framebuffer Objects supported");
	else
		log::info("Framebuffer Objects not supported");

	// Log GL messages
	if (gl_debug)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(glMessageCallback, nullptr);
	}

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

	if (np2TexSupport())
		return true;

	// Check for power-of-two dimension
	for (auto s : pow_two)
		if (dim == s)
			return true;

	return false;
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
// Returns the GL attributes array for use with wxGLCanvas
// -----------------------------------------------------------------------------
wxGLAttributes gl::getWxGLAttribs()
{
	wxGLAttributes attr;

	// Try 32bit depth buffer first
	buildGlAttr(attr, 32);
	if (wxGLCanvas::IsDisplaySupported(attr))
		return attr;

	// Then 24 bit depth buffer if not supported
	buildGlAttr(attr, 24);
	if (wxGLCanvas::IsDisplaySupported(attr))
		return attr;

	// Then 16bit depth buffer (if this isn't supported then it's something else)
	buildGlAttr(attr, 16);

	return attr;
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
		else if (blend == Blend::Invert)
			glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);

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

unsigned gl::createBuffer()
{
	if (!initialised)
		return 0;

	unsigned vbo;
	glGenBuffers(1, &vbo);
	return vbo;
}

void gl::deleteBuffer(unsigned id)
{
	if (initialised && id > 0)
		glDeleteBuffers(1, &id);
}

unsigned gl::currentVBO()
{
	return vbo_current;
}

void gl::bindVBO(unsigned id)
{
	if (!initialised || vbo_current == id)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, id);
	vbo_current = id;
}

void gl::deleteVBO(unsigned id)
{
	if (!initialised)
		return;

	if (id > 0)
	{
		if (vbo_current == id)
		{
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			vbo_current = 0;
		}

		glDeleteBuffers(1, &id);
	}
}

unsigned gl::currentEBO()
{
	return ebo_current;
}

void gl::bindEBO(unsigned id)
{
	if (!initialised || ebo_current == id)
		return;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
	ebo_current = id;
}

void gl::deleteEBO(unsigned id)
{
	if (!initialised || id == 0)
		return;

	if (ebo_current == id)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		ebo_current = 0;
	}

	glDeleteBuffers(1, &id);
}

unsigned gl::currentVAO()
{
	return vao_current;
}

unsigned gl::createVAO()
{
	if (!initialised)
		return 0;

	unsigned vao = 0;
	glGenVertexArrays(1, &vao);
	return vao;
}

void gl::bindVAO(unsigned id)
{
	if (!initialised || vao_current == id)
		return;

	glBindVertexArray(id);
	vao_current = id;
}

void gl::deleteVAO(unsigned id)
{
	if (!initialised)
		return;

	if (id > 0)
	{
		if (vao_current == id)
		{
			glBindVertexArray(0);
			vao_current = 0;
		}

		glDeleteVertexArrays(1, &id);
	}
}

void gl::resetDrawCallCount()
{
	drawcall_count = 0;
}

unsigned gl::drawCallCount()
{
	return drawcall_count;
}

void gl::drawArrays(Primitive primitive, unsigned first, unsigned count)
{
	drawcall_count++;
	glDrawArrays(static_cast<GLenum>(primitive), first, count);
}

void gl::drawArraysInstanced(Primitive primitive, unsigned first, unsigned count, unsigned instance_count)
{
	drawcall_count++;
	glDrawArraysInstanced(static_cast<GLenum>(primitive), first, count, instance_count);
}

void gl::drawElements(Primitive primitive, unsigned count, GLenum type, const void* indices)
{
	drawcall_count++;
	glDrawElements(static_cast<GLenum>(primitive), count, type, indices);
}

void gl::drawElementsInstanced(
	Primitive   primitive,
	unsigned    count,
	GLenum      type,
	unsigned    instance_count,
	const void* indices)
{
	drawcall_count++;
	glDrawElementsInstanced(static_cast<GLenum>(primitive), count, type, indices, instance_count);
}
