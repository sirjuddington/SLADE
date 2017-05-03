
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    GfxCanvas.cpp
 * Description: GfxCanvas class. An OpenGL canvas that displays
 *              an image and can take offsets into account etc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "GfxCanvas.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
DEFINE_EVENT_TYPE(wxEVT_GFXCANVAS_OFFSET_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_GFXCANVAS_PIXELS_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_GFXCANVAS_COLOUR_PICKED)
CVAR(Bool, gfx_show_border, true, CVAR_SAVE)
CVAR(Bool, gfx_hilight_mouseover, true, CVAR_SAVE)
CVAR(Bool, gfx_arc, false, CVAR_SAVE)

#define BRCNTR 4	// Brush center
#define BRPIX(b, x, y) brushes[(b)][BRCNTR+(x)][BRCNTR+(y)]
static const uint8_t brushes[Brush::NUM_BRUSHES][9][9] =
{
	{	// Square 1
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 3
		{ 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // C
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 6
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Square 3
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 3
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // C
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 5
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 6
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Square 5
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 2
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 3
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // C
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 5
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 6
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Square 7
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 1
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 2
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 3
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // C
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 5
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 6
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Square 9
		//0  1  2  3  C  5  6  7  8
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 0
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 1
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 2
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 3
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // C
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 6
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 7
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 8
	},
	{	// Circle 5
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 2
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 3
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // C
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 5
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 6
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Circle 7
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 1
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 2
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 3
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // C
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 5
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 6
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Circle 9
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 0
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 1
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 2
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 3
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // C
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 6
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 7
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 8
	},
	{	// Diamond 3
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2
		{ 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 3
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // C
		{ 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 5
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 6
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Diamond 5
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
		{ 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 2
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 3
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // C
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 5
		{ 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 6
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Diamond 7
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 1
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 2
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 3
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // C
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 5
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 6
		{ 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Diamond 9
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 0
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 1
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 2
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 3
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // C
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0 }, // 5
		{ 0, 0, 1, 1, 1, 1, 1, 0, 0 }, // 6
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0 }, // 7
		{ 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // 8
	},
	{	// Dither pattern A
		//0  1  2  3  C  5  6  7  8
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // 0
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 3
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // C
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 6
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 7
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // 8
	},
	{	// Dither pattern B
		//0  1  2  3  C  5  6  7  8
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // 0
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
		{ 0, 0, 1, 0, 0, 0, 1, 0, 0 }, // 2
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 3
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // C
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
		{ 0, 0, 1, 0, 0, 0, 1, 0, 0 }, // 6
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 7
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // 8
	},
	{	// Dither pattern C
		//0  1  2  3  C  5  6  7  8
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 0
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // 2
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 3
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // C
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 5
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // 6
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 7
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 8
	},
	{	// Dither pattern D
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 1
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 3
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // C
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 5
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 6
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Dither pattern E
		//0  1  2  3  C  5  6  7  8
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 1
		{ 0, 0, 1, 0, 0, 0, 1, 0, 0 }, // 2
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 3
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // C
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 5
		{ 0, 0, 1, 0, 0, 0, 1, 0, 0 }, // 6
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 7
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
	},
	{	// Dither pattern F
		//0  1  2  3  C  5  6  7  8
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // 0
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 1
		{ 0, 0, 1, 0, 0, 0, 1, 0, 0 }, // 2
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 3
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // C
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 5
		{ 0, 0, 1, 0, 0, 0, 1, 0, 0 }, // 6
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 7
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // 8
	},
	{	// Dither pattern G
		//0  1  2  3  C  5  6  7  8
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 0
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 1
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // 2
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 3
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // C
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 5
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // 6
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 7
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 8
	},
	{	// Dither pattern H
		//0  1  2  3  C  5  6  7  8
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 0
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 1
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 2
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 3
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // C
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 5
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 6
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 7
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 8
	},
	{	// Dither pattern I
		//0  1  2  3  C  5  6  7  8
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 0
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 1
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // 2
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 3
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // C
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 5
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // 6
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 7
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 8
	},
	{	// Dither pattern J
		//0  1  2  3  C  5  6  7  8
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // 0
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 1
		{ 1, 1, 0, 1, 1, 1, 0, 1, 1 }, // 2
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 3
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // C
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 5
		{ 1, 1, 0, 1, 1, 1, 0, 1, 1 }, // 6
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 7
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // 8
	},
	{	// Dither pattern K
		//0  1  2  3  C  5  6  7  8
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 0
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 1
		{ 1, 1, 0, 1, 1, 1, 0, 1, 1 }, // 2
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 3
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // C
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 5
		{ 1, 1, 0, 1, 1, 1, 0, 1, 1 }, // 6
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 7
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 8
	},
	{	// Dither pattern L
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 0
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 1
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 2
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 3
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // C
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 5
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 6
		{ 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // 7
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 8
	},
	{	// Dither pattern M
		//0  1  2  3  C  5  6  7  8
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 0
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 1
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // 2
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 3
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // C
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // 6
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 7
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // 8
	},
	{	// Dither pattern N
		//0  1  2  3  C  5  6  7  8
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // 0
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 1
		{ 1, 1, 0, 1, 1, 1, 0, 1, 1 }, // 2
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 3
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // C
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
		{ 1, 1, 0, 1, 1, 1, 0, 1, 1 }, // 6
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 7
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // 8
	},
	{	// Dither pattern O
		//0  1  2  3  C  5  6  7  8
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // 0
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 1
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 2
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 3
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // C
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 5
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 6
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // 7
		{ 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // 8
	},
};

/*******************************************************************
 * GFXCANVAS CLASS FUNCTIONS
 *******************************************************************/

/* GfxCanvas::GfxCanvas
 * GfxCanvas class constructor
 *******************************************************************/
GfxCanvas::GfxCanvas(wxWindow* parent, int id)
	: OGLCanvas(parent, id)
{
	// Init variables
	image = new SImage();
	view_type = GFXVIEW_DEFAULT;
	scale = 1;
	tex_image = new GLTexture();
	update_texture = false;
	image_hilight = false;
	drag_pos.set(0, 0);
	drag_origin.set(POINT_OUTSIDE);
	allow_drag = false;
	allow_scroll = false;
	editing_mode = 0;
	paint_colour = COL_BLACK;
	translation = NULL;
	drawing = false;
	drawing_mask = NULL;
	brush = 0;
	tex_brush = new GLTexture();
	cursor_pos.set(POINT_OUTSIDE);
	prev_pos.set(POINT_OUTSIDE);

	// Listen to the image for changes
	listenTo(image);

	// Bind events
	Bind(wxEVT_LEFT_DOWN, &GfxCanvas::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &GfxCanvas::onMouseRightDown, this);
	Bind(wxEVT_LEFT_UP, &GfxCanvas::onMouseLeftUp, this);
	Bind(wxEVT_MOTION, &GfxCanvas::onMouseMovement, this);
	Bind(wxEVT_LEAVE_WINDOW, &GfxCanvas::onMouseLeaving, this);
	Bind(wxEVT_KEY_DOWN, &GfxCanvas::onKeyDown, this);
}

/* GfxCanvas::~GfxCanvas
 * GfxCanvas class destructor
 *******************************************************************/
GfxCanvas::~GfxCanvas()
{
	delete image;
	delete tex_image;
}

/* GfxCanvas::draw
 * Draws the image/background/etc
 *******************************************************************/
void GfxCanvas::draw()
{
	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, GetSize().y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw the background
	drawCheckeredBackground();

	// Pan by view offset
	if (allow_scroll)
		glTranslated(offset.x, offset.y, 0);

	// Pan if offsets
	if (view_type == GFXVIEW_CENTERED || view_type == GFXVIEW_SPRITE || view_type == GFXVIEW_HUD)
	{
		int mid_x = GetSize().x / 2;
		int mid_y = GetSize().y / 2;
		glTranslated(mid_x, mid_y, 0);
	}

	// Draw offset lines
	if (view_type == GFXVIEW_SPRITE || view_type == GFXVIEW_HUD)
		drawOffsetLines();

	// Draw the image
	drawImage();

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}

/* GfxCanvas::drawOffsetLines
 * Draws the offset center lines
 *******************************************************************/
void GfxCanvas::drawOffsetLines()
{
	if (view_type == GFXVIEW_SPRITE)
	{
		OpenGL::setColour(COL_BLACK);

		glBegin(GL_LINES);
		glVertex2d(-9999, 0);
		glVertex2d(9999, 0);
		glVertex2d(0, -9999);
		glVertex2d(0, 9999);
		glEnd();
	}
	else if (view_type == GFXVIEW_HUD)
	{
		double yscale = (gfx_arc ? scale * 1.2 : scale);
		glPushMatrix();
		glEnable(GL_LINE_SMOOTH);
		glScaled(scale, yscale, 1);
		Drawing::drawHud();
		glDisable(GL_LINE_SMOOTH);
		glPopMatrix();
	}
}

/* GfxCanvas::drawImage
 * Draws the image (reloads the image as a texture each time, will
 * change this later...)
 *******************************************************************/
void GfxCanvas::drawImage()
{
	// Check image is valid
	if (!image->isValid())
		return;

	// Save current matrix
	glPushMatrix();

	// Zoom
	double yscale = (gfx_arc ? scale * 1.2 : scale);
	glScaled(scale, yscale, 1.0);

	// Pan
	if (view_type == GFXVIEW_CENTERED)
		glTranslated(-(image->getWidth() * 0.5), -(image->getHeight() * 0.5), 0);	// Pan to center image
	else if (view_type == GFXVIEW_SPRITE)
		glTranslated(-image->offset().x, -image->offset().y, 0);	// Pan by offsets
	else if (view_type == GFXVIEW_HUD)
	{
		glTranslated(-160, -100, 0);								// Pan to hud 'top left'
		glTranslated(-image->offset().x, -image->offset().y, 0);	// Pan by offsets
	}

	// Enable textures
	glEnable(GL_TEXTURE_2D);

	// Update texture if needed
	if (update_texture)
	{
		// If the image change isn't caused by drawing, resize drawing mask
		if (!drawing)
		{
			if (drawing_mask)
				delete[] drawing_mask;
			drawing_mask = new bool[image->getWidth() * image->getHeight()];
			memset(drawing_mask, false, image->getWidth() * image->getHeight());
		}
		tex_image->loadImage(image, &palette);
		update_texture = false;
	}

	// Determine (texture)coordinates
	double x = (double)image->getWidth();
	double y = (double)image->getHeight();

	// If tiled view
	if (view_type == GFXVIEW_TILED)
	{
		// Draw tiled image
		OpenGL::setColour(255, 255, 255, 255, 0);
		tex_image->draw2dTiled(GetSize().x / scale, GetSize().y / scale);
	}
	else if (drag_origin.x < 0)  	// If not dragging
	{
		// Draw the image
		OpenGL::setColour(255, 255, 255, 255, 0);
		tex_image->draw2d();

		// Draw hilight otherwise
		if (image_hilight && gfx_hilight_mouseover && editing_mode == 0)
		{
			OpenGL::setColour(255, 255, 255, 80, 1);
			tex_image->draw2d();

			// Reset colour
			OpenGL::setColour(255, 255, 255, 255, 0);
		}
	}
	else  	// Dragging
	{
		// Draw the original
		OpenGL::setColour(rgba_t(0, 0, 0, 180, 0));
		tex_image->draw2d();

		// Draw the dragged image
		int off_x = (drag_pos.x - drag_origin.x) / scale;
		int off_y = (drag_pos.y - drag_origin.y) / scale;
		glTranslated(off_x, off_y, 0);
		OpenGL::setColour(255, 255, 255, 255, 0);
		tex_image->draw2d();
	}
	// Draw brush shadow when in editing mode
	if (editing_mode > 0 && cursor_pos != POINT_OUTSIDE)
	{
		OpenGL::setColour(255, 255, 255, 160, 0);
		tex_brush->draw2d();
		OpenGL::setColour(255, 255, 255, 255, 0);
	}

	// Disable textures
	glDisable(GL_TEXTURE_2D);

	// Draw outline
	if (gfx_show_border)
	{
		OpenGL::setColour(0, 0, 0, 64);
		glBegin(GL_LINE_LOOP);
		glVertex2d(0, 0);
		glVertex2d(0, y);
		glVertex2d(x, y);
		glVertex2d(x, 0);
		glEnd();
	}

	// Restore previous matrix
	glPopMatrix();
}

/* GfxCanvas::updateImageTexture
 * Forces (Re)Generation of the image texture
 *******************************************************************/
void GfxCanvas::updateImageTexture()
{
	update_texture = true;
	Refresh();
}

/* GfxCanvas::zoomToFit
 * Scales the image to fit within the gfx canvas. If mag is false,
 * the image will not be stretched to fit the canvas (only shrunk
 * if needed). Leaves a border around the image if <padding> is
 * specified (0.0f = no border, 1.0f = border 100% of canvas size)
 *******************************************************************/
void GfxCanvas::zoomToFit(bool mag, float padding)
{
	// Determine padding
	double pad = (double)MIN(GetSize().x, GetSize().y) * padding;

	// Get image dimensions
	double x_dim = (double)image->getWidth();
	double y_dim = (double)image->getHeight();

	// Get max scale for x and y (including padding)
	double x_scale = ((double)GetSize().x - pad) / x_dim;
	double y_scale = ((double)GetSize().y - pad) / y_dim;

	// Set scale to smallest of the 2 (so that none of the image will be clipped)
	scale = MIN(x_scale, y_scale);

	// If we don't want to magnify the image, clamp scale to a max of 1.0
	if (!mag && scale > 1)
		scale = 1;
}

/* GfxCanvas::onImage
 * Returns true if the given coordinates are 'on' top of the image
 *******************************************************************/
bool GfxCanvas::onImage(int x, int y)
{
	// Don't disable in editing mode; it can be quite useful 
	// to have a live preview of how a graphic will tile.
	if (view_type == GFXVIEW_TILED && editing_mode == 0)
		return false;

	// No need to duplicate the imageCoords code.
	return imageCoords(x, y) != POINT_OUTSIDE;
}

/* GfxCanvas::imageCoords
 * Returns the image coordinates at [x,y] in screen coordinates, or
 * [-1, -1] if not on the image
 *******************************************************************/
point2_t GfxCanvas::imageCoords(int x, int y)
{
	// Determine top-left coordinates of image in screen coords
	double left = GetSize().x * 0.5 + offset.x;
	double top = GetSize().y * 0.5 + offset.y;
	double yscale = scale * (gfx_arc ? 1.2 : 1);

	if (view_type == GFXVIEW_DEFAULT || view_type == GFXVIEW_TILED)
	{
		left = offset.x;
		top = offset.y;
	}
	else if (view_type == GFXVIEW_CENTERED)
	{
		left -= (double)image->getWidth() * 0.5 * scale;
		top -= (double)image->getHeight() * 0.5 * yscale;
	}
	else if (view_type == GFXVIEW_SPRITE)
	{
		left -= image->offset().x * scale;
		top -= image->offset().y * yscale;
	}
	else if (view_type == GFXVIEW_HUD)
	{
		left -= 160 * scale;
		top -= 100 * scale * (gfx_arc ? 1.2 : 1);
		left -= image->offset().x * scale;
		top -= image->offset().y * yscale;
	}

	// Determine bottom-right coordinates of image in screen coords
	double right = left + image->getWidth() * scale;
	double bottom = top + image->getHeight() * yscale;

	// Check if the pointer is within the image
	if (x >= left && x <= right && y >= top && y <= bottom)
	{
		// Determine where in the image it is
		double w = right - left;
		double h = bottom - top;
		double xpos = double(x - left) / w;
		double ypos = double(y - top) / h;

		return point2_t(xpos * image->getWidth(), ypos * image->getHeight());
	}
	else
		return POINT_OUTSIDE;
}

/* GfxCanvas::endOffsetDrag
 * Finishes an offset drag
 *******************************************************************/
void GfxCanvas::endOffsetDrag()
{
	// Get offset
	int x = (drag_pos.x - drag_origin.x) / scale;
	int y = (drag_pos.y - drag_origin.y) / scale;

	// If there was a change
	if (x != 0 || y != 0)
	{
		// Set image offsets
		image->setXOffset(image->offset().x - x);
		image->setYOffset(image->offset().y - y);

		// Generate event
		wxNotifyEvent e(wxEVT_GFXCANVAS_OFFSET_CHANGED, GetId());
		e.SetEventObject(this);
		GetEventHandler()->ProcessEvent(e);
	}

	// Stop drag
	drag_origin.set(POINT_OUTSIDE);
}

/* GfxCanvas::paintPixel
 * Paints a pixel from the image at the given image coordinates.
 *******************************************************************/
void GfxCanvas::paintPixel(int x, int y)
{
	// With large brushes, it's very possible that some of the pixels
	// are out of the image area; so don't process them.
	if (x < 0 || y < 0 || x >= image->getWidth() || y >= image->getHeight())
		return;

	// Do not process pixels that were already modified in the
	// current drawing operation. This mechanism is needed to
	// allow freehand drawing, because an unpredictable number
	// of mouse events can happen while the mouse moves, leading
	// to the same pixel being processed over and over, and that
	// does not play well when applying translations.
	size_t pos = x + image->getWidth() * y;
	if (drawing_mask[pos])
		return;

	bool painted = false;
	if (editing_mode == 2) // eraser
		painted = image->setPixel(x, y, 255, 0);
	else if (editing_mode == 3) // translator
	{
		if (translation != NULL)
		{
			rgba_t ocol = image->getPixel(x, y, getPalette());
			uint8_t alpha = ocol.a;
			rgba_t ncol = (translation->translate(ocol, getPalette()));
			ncol.a = alpha;
			if (!ocol.equals(ncol, false, true))
				painted = image->setPixel(x, y, ncol);
		}
	}
	else painted = image->setPixel(x, y, paint_colour);

	// Mark the modification, if any, and announce the modification
	drawing_mask[pos] = painted;
	if (painted)
	{
		// Generate event
		wxNotifyEvent e(wxEVT_GFXCANVAS_PIXELS_CHANGED, GetId());
		e.SetEventObject(this);
		GetEventHandler()->ProcessEvent(e);
	}
}

/* GfxCanvas::brushCanvas
 * Finds all the pixels under the brush, and paints them.
 *******************************************************************/
void GfxCanvas::brushCanvas(int x, int y)
{
	point2_t coord = imageCoords(x, y);
	for (int i = -4; i < 5; ++i)
		for (int j = -4; j < 5; ++j)
			if (BRPIX(brush, i, j))
				paintPixel(coord.x + i, coord.y + j);
}

/* GfxCanvas::pickColour
 * Finds the pixel under the cursor, and picks its colour.
 *******************************************************************/
void GfxCanvas::pickColour(int x, int y)
{
	// Get the pixel
	point2_t coord = imageCoords(x, y);

	// Pick its colour
	paint_colour = image->getPixel(coord.x, coord.y, getPalette());

	// Announce it triumphantly to the world
	wxNotifyEvent e(wxEVT_GFXCANVAS_COLOUR_PICKED, GetId());
	e.SetEventObject(this);
	GetEventHandler()->ProcessEvent(e);
}

/* GfxCanvas::generateBrushShadow
 * Creates a mask texture of the brush to preview its effect
 *******************************************************************/
void GfxCanvas::generateBrushShadow()
{
	// Generate image
	SImage img;
	img.create(image->getWidth(), image->getHeight(), SIType::RGBA);
	for (int i = -4; i < 5; ++i)
		for (int j = -4; j < 5; ++j)
			if (BRPIX(brush, i, j))
			{
				rgba_t col = paint_colour;
				if (editing_mode == 3 && translation)
					col = translation->translate(image->getPixel(cursor_pos.x + i,
						cursor_pos.y + j, getPalette()), getPalette());
				// Not sure what's the best way to preview cutting out
				// Mimicking the checkerboard pattern perhaps?
				// Cyan will do for now
				else if (editing_mode == 2)
					col = COL_CYAN;
				img.setPixel(cursor_pos.x + i, cursor_pos.y + j, col);
			}

	// Load it as a GL texture
	tex_brush->loadImage(&img);
}

/* GfxCanvas::onAnnouncement
 * Called when an announcement is recieved from the image that this
 * GfxCanvas is displaying
 *******************************************************************/
void GfxCanvas::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	if (announcer == image && event_name.Cmp("image_changed") == 0)
		update_texture = true;
}


/*******************************************************************
 * GFXCANVAS EVENTS
 *******************************************************************/

/* GfxCanvas::onMouseLeftDown
 * Called when the left button is pressed within the canvas
 *******************************************************************/
void GfxCanvas::onMouseLeftDown(wxMouseEvent& e)
{
	int x = e.GetPosition().x;
	int y = e.GetPosition().y;
	bool on_image = onImage(x, y-2);

	// Left mouse down
	if (e.LeftDown() && on_image)
	{
		// Paint in paint mode
		if (editing_mode)
		{
			drawing = true;
			brushCanvas(x, y);
		}

		// Begin drag if mouse is over image and dragging allowed
		else if (allow_drag)
		{
			drag_origin.set(x, y);
			drag_pos.set(x, y);
			Refresh();
		}
	}

	e.Skip();
}

/* GfxCanvas::onMouseRightDown
 * Called when the left button is pressed within the canvas
 *******************************************************************/
void GfxCanvas::onMouseRightDown(wxMouseEvent& e)
{
	int x = e.GetPosition().x;
	int y = e.GetPosition().y - 2;

	// Right mouse down
	if (e.RightDown() && onImage(x, y))
		pickColour(x, y);

	e.Skip();
}

/* GfxCanvas::onMouseLeftUp
 * Called when the left button is released within the canvas
 *******************************************************************/
void GfxCanvas::onMouseLeftUp(wxMouseEvent& e)
{
	// Stop drawing
	if (drawing)
	{
		drawing = false;
		memset(drawing_mask, false, image->getWidth() * image->getHeight());
	}
	// Stop dragging
	if (drag_origin.x >= 0)
	{
		endOffsetDrag();
		image_hilight = true;
		Refresh();
	}
}

/* GfxCanvas::onMouseMovement
 * Called when the mouse pointer is moved within the canvas
 *******************************************************************/
void GfxCanvas::onMouseMovement(wxMouseEvent& e)
{
	bool refresh = false;

	// Check if the mouse is over the image
	int x = e.GetPosition().x;
	int y = e.GetPosition().y - 2;
	bool on_image = onImage(x, y);
	cursor_pos = imageCoords(x, y);
	if (on_image && editing_mode)
	{
		if (cursor_pos != prev_pos)
			generateBrushShadow();
		prev_pos = cursor_pos;
	}
	if (on_image != image_hilight)
	{
		image_hilight = on_image;
		refresh = true;

		// Update cursor if drag allowed
		if (on_image)
		{
			if (editing_mode)
				SetCursor(wxCursor(wxCURSOR_PENCIL));
			else if (allow_drag)
				SetCursor(wxCursor(wxCURSOR_SIZING));
		}
		else if (allow_drag && !e.LeftIsDown())
			SetCursor(wxNullCursor);
	}
	// Drag
	if (e.LeftIsDown())
	{
		if (editing_mode)
		{
			brushCanvas(x, y);
		}
		else
		{
			drag_pos.set(e.GetPosition().x, e.GetPosition().y);
			refresh = true;
		}
	}
	else if (e.MiddleIsDown())
	{
		offset = offset + point2_t(e.GetPosition().x - mouse_prev.x, e.GetPosition().y - mouse_prev.y);
		refresh = true;
	}
	// Right mouse down
	if (e.RightIsDown() && on_image)
		pickColour(x, y);

	if (refresh)
		Refresh();

	mouse_prev.set(e.GetPosition().x, e.GetPosition().y);
}

/* GfxCanvas::onMouseLeaving
 * Called when the mouse pointer leaves the gfx canvas
 *******************************************************************/
void GfxCanvas::onMouseLeaving(wxMouseEvent& e)
{
	image_hilight = false;
	Refresh();
}

/* GfxCanvas::onKeyDown
 * Called when a key is pressed while the canvas has focus
 *******************************************************************/
void GfxCanvas::onKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_UP)
	{
		offset.y += 8;
		Refresh();
	}

	else if (e.GetKeyCode() == WXK_DOWN)
	{
		offset.y -= 8;
		Refresh();
	}

	else if (e.GetKeyCode() == WXK_LEFT)
	{
		offset.x += 8;
		Refresh();
	}

	else if (e.GetKeyCode() == WXK_RIGHT)
	{
		offset.x -= 8;
		Refresh();
	}

	else
		e.Skip();
}
