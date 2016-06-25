
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    VertexInfoOverlay.cpp
 * Description: VertexInfoOverlay class - a map editor overlay that
 *              displays information about the currently highlighted
 *              vertex in vertices mode
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
#include "VertexInfoOverlay.h"
#include "MapEditor/SLADEMap/MapVertex.h"
#include "OpenGL/GLUI/Animator.h"
#include "OpenGL/GLUI/TextBox.h"


/*******************************************************************
 * VERTEXINFOOVERLAY CLASS FUNCTIONS
 *******************************************************************/

/* VertexInfoOverlay::VertexInfoOverlay
 * VertexInfoOverlay class constructor
 *******************************************************************/
VertexInfoOverlay::VertexInfoOverlay()
{
	setSize(dim2_t(500, 20));
	text_coords = new GLUI::TextBox(this, "");

	setBGCol(rgba_t(0, 0, 0, 160));
}

/* VertexInfoOverlay::update
 * Update the info overlay with [vertex]
 *******************************************************************/
void VertexInfoOverlay::update(MapVertex* vertex)
{
	if (!vertex)
	{
		// Animate
		activate(false);
		return;
	}

	// Animate
	activate(true);

	// Update info string
	string text = S_FMT("Vertex %d: (%d, %d)", vertex->getIndex(), (int)vertex->xPos(), (int)vertex->yPos());

	if (Global::debug)
		text += S_FMT(" (%d)", vertex->getId());

	text_coords->setText(text);
}

/* VertexInfoOverlay::updateLayout
 * Update the widget layout to fit within [fit]. A negative [fit]
 * dimension indicates no size limit
 *******************************************************************/
void VertexInfoOverlay::updateLayout(dim2_t fit)
{
	text_coords->updateLayout(dim2_t(fit.x, -1));
	text_coords->setPosition(point2_t(4, 4));
	setSize(dim2_t(fit.x, text_coords->bottom() + 4));
}
