
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Manager.cpp
 * Description: GLUI Manager class. Attached to an OGLCanvas, handles
 *              drawing, layout (docking) and animation for child
 *              widgets
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
#include "Manager.h"
#include "Widget.h"
#include "UI/Canvas/OGLCanvas.h"


/*******************************************************************
 * MANAGER CLASS FUNCTIONS
 *******************************************************************/

/* Manager::Manager
 * Manager class constructor
 *******************************************************************/
GLUI::Manager::Manager(OGLCanvas* canvas) : canvas(canvas)
{
}

/* Manager::~Manager
 * Manager class destructor
 *******************************************************************/
GLUI::Manager::~Manager()
{
}

/* Manager::addWidget
 * Adds [widget] as a child widget to be managed, with optional
 * [dock] location
 *******************************************************************/
void GLUI::Manager::addWidget(Widget* widget, uint8_t dock)
{
	// Update dock if widget is already managed
	for (auto inf : widgets)
		if (inf.widget == widget)
		{
			inf.dock = dock;
			return;
		}

	// Otherwise add widget
	widgets.push_back(widget_info_t(widget, dock));
}

/* Manager::update
 * Update and animate managed widgets
 *******************************************************************/
void GLUI::Manager::update(int time)
{
	// Update canvas size
	canvas_size.x = canvas->GetSize().x;
	canvas_size.y = canvas->GetSize().y;

	// Update widget animations
	for (auto inf : widgets)
		inf.widget->animate(time);
}

/* Manager::drawWidgets
 * Draw managed widgets
 *******************************************************************/
void GLUI::Manager::drawWidgets()
{
	for (auto inf : widgets)
	{
		applyDocking(inf);
		inf.widget->draw(point2_t(0, 0));
	}
}

/* Manager::applyDocking
 * Apply docking position to widget [inf]
 *******************************************************************/
void GLUI::Manager::applyDocking(widget_info_t& inf)
{
	switch (inf.dock)
	{
	case DOCK_LEFT:
		inf.widget->updateLayout(dim2_t(-1, canvas_size.y));
		inf.widget->setPosition(point2_t(0, 0));
		break;
	case DOCK_TOP:
		inf.widget->updateLayout(dim2_t(canvas_size.x, -1));
		inf.widget->setPosition(point2_t(0, 0));
		break;
	case DOCK_RIGHT:
		inf.widget->updateLayout(dim2_t(-1, canvas_size.y));
		inf.widget->setPosition(point2_t(canvas_size.x - inf.widget->getWidth(), 0));
		break;
	case DOCK_BOTTOM:
		inf.widget->updateLayout(dim2_t(canvas_size.x, -1));
		inf.widget->setPosition(point2_t(0, canvas_size.y - inf.widget->getHeight()));
		break;
	default:
		break;
	}
}
