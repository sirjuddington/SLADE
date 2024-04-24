
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Canvas.cpp
// Description: Canvas related helper functions
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
#include "Canvas.h"
#include "GL/GfxGLCanvas.h"
#include "GL/MapPreviewGLCanvas.h"
#include "GfxCanvas.h"
#include "MapPreviewCanvas.h"
#include "OpenGL/OpenGL.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, use_gl_canvas, true, CVar::Save)


// -----------------------------------------------------------------------------
//
// UI Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::ui
{
// -----------------------------------------------------------------------------
// Creates a new MapPreviewGLCanvas if OpenGL is available, otherwise will fall
// back to a software-rendered MapPreviewCanvas
// -----------------------------------------------------------------------------
wxWindow* createMapPreviewCanvas(wxWindow* parent, MapPreviewData* data, bool allow_zoom, bool allow_pan)
{
	if (gl::contextCreationFailed() || !use_gl_canvas)
		return new MapPreviewCanvas(parent, data);
	else
		return new MapPreviewGLCanvas(parent, data, allow_zoom, allow_pan);
}

// -----------------------------------------------------------------------------
// Creates a new GfxGLCanvas if OpenGL is available, otherwise will fall back to
// a software-rendered GfxCanvas
// -----------------------------------------------------------------------------
GfxCanvasBase* createGfxCanvas(wxWindow* parent)
{
	if (gl::contextCreationFailed() || !use_gl_canvas)
		return new GfxCanvas(parent);
	else
		return new GfxGLCanvas(parent);
}
} // namespace slade::ui
