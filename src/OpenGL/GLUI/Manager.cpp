
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
	if (canvas)
	{
		canvas->Bind(wxEVT_MOTION, &Manager::onMouseMove, this);
		canvas->Bind(wxEVT_LEFT_DOWN, &Manager::onMouseDown, this);
		canvas->Bind(wxEVT_RIGHT_DOWN, &Manager::onMouseDown, this);
		canvas->Bind(wxEVT_MIDDLE_DOWN, &Manager::onMouseDown, this);
		canvas->Bind(wxEVT_LEFT_UP, &Manager::onMouseUp, this);
		canvas->Bind(wxEVT_RIGHT_UP, &Manager::onMouseUp, this);
		canvas->Bind(wxEVT_MIDDLE_UP, &Manager::onMouseUp, this);
		canvas->Bind(wxEVT_SIZE, &Manager::onSize, this);
	}
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
void GLUI::Manager::addWidget(Widget* widget, string id, uint8_t dock)
{
	// Update dock if widget is already managed
	for (auto inf : widgets)
		if (inf.widget == widget)
		{
			inf.dock = dock;
			applyDocking(inf);
			return;
		}

	// Otherwise add widget
	widgets.push_back({ widget, id, dock });
	applyDocking(widgets.back());
}

GLUI::Widget* GLUI::Manager::getWidget(string id)
{
	for (auto inf : widgets)
		if (inf.id == id)
			return inf.widget;

	return nullptr;
}

/* Manager::update
 * Update and animate managed widgets
 *******************************************************************/
void GLUI::Manager::update(int time)
{
	// Check if layout update is needed
	//bool layout_update = (canvas_size.x != canvas->GetSize().x || canvas_size.y != canvas->GetSize().y);

	// Update canvas size
	canvas_size.x = canvas->GetSize().x;
	canvas_size.y = canvas->GetSize().y;

	// Update layout if needed
	//if (layout_update)
		for (auto inf : widgets)
			applyDocking(inf);

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
		inf.widget->draw(point2_t(0, 0));
}

/* Manager::applyDocking
 * Apply docking position to widget [inf]
 *******************************************************************/
void GLUI::Manager::applyDocking(WidgetInfo& inf)
{
	switch (inf.dock)
	{
	case DOCK_LEFT:
		inf.widget->updateLayout(dim2_t(-1, canvas_size.y));
		inf.widget->setPosition(point2_t(0, 0));
		inf.widget->setSize(point2_t(inf.widget->getWidth(), canvas_size.y));
		break;
	case DOCK_TOP:
		inf.widget->updateLayout(dim2_t(canvas_size.x, -1));
		inf.widget->setPosition(point2_t(0, 0));
		inf.widget->setSize(point2_t(canvas_size.x, inf.widget->getHeight()));
		break;
	case DOCK_RIGHT:
		inf.widget->updateLayout(dim2_t(-1, canvas_size.y));
		inf.widget->setPosition(point2_t(canvas_size.x - inf.widget->getWidth(), 0));
		inf.widget->setSize(point2_t(inf.widget->getWidth(), canvas_size.y));
		break;
	case DOCK_BOTTOM:
		inf.widget->updateLayout(dim2_t(canvas_size.x, -1));
		inf.widget->setPosition(point2_t(0, canvas_size.y - inf.widget->getHeight()));
		inf.widget->setSize(point2_t(canvas_size.x, inf.widget->getHeight()));
		break;
	case DOCK_FILL:
		inf.widget->updateLayout(canvas_size);
		inf.widget->setPosition(point2_t(0, 0));
		inf.widget->setSize(canvas_size);
	default:
		break;
	}
}

void GLUI::Manager::mouseMove(int x, int y)
{
	for (auto inf : widgets)
	{
		if (x >= inf.widget->left() && x <= inf.widget->right() &&
			y >= inf.widget->top() && y <= inf.widget->bottom())
		{
			inf.widget->mouseMove(x, y);
			inf.widget->mouseOver(true);
		}
		else
			inf.widget->mouseOver(false);
	}
}


void GLUI::Manager::onMouseMove(wxMouseEvent& e)
{
	mouseMove(e.GetPosition().x, e.GetPosition().y);
	e.Skip();
}

void GLUI::Manager::onSize(wxSizeEvent& e)
{
	//// Update canvas size
	//canvas_size.x = e.GetSize().x;
	//canvas_size.y = e.GetSize().y;

	//// Update widgets
	//for (auto inf : widgets)
	//	applyDocking(inf);

	e.Skip();
}

void GLUI::Manager::onMouseDown(wxMouseEvent& e)
{
	// Get mouse button
	MouseBtn button;
	switch (e.GetButton())
	{
	case wxMOUSE_BTN_LEFT: button = MouseBtn::Left; break;
	case wxMOUSE_BTN_RIGHT: button = MouseBtn::Right; break;
	case wxMOUSE_BTN_MIDDLE: button = MouseBtn::Middle; break;
	default: button = MouseBtn::Left; break;
	}

	for (auto inf : widgets)
		if (inf.widget->mouseIsOver())
			inf.widget->mouseButtonDown(button, e.GetPosition().x, e.GetPosition().y);

	e.Skip();
}

void GLUI::Manager::onMouseUp(wxMouseEvent& e)
{
	// Get mouse button
	MouseBtn button;
	switch (e.GetButton())
	{
	case wxMOUSE_BTN_LEFT: button = MouseBtn::Left; break;
	case wxMOUSE_BTN_RIGHT: button = MouseBtn::Right; break;
	case wxMOUSE_BTN_MIDDLE: button = MouseBtn::Middle; break;
	default: button = MouseBtn::Left; break;
	}

	for (auto inf : widgets)
		if (inf.widget->mouseIsOver())
			inf.widget->mouseButtonUp(button, e.GetPosition().x, e.GetPosition().y);

	e.Skip();
}
