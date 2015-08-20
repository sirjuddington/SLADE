
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    STopWindow.cpp
 * Description: Base class for top level windows, handles custom
 *              menus and toolbars, also saved position and size
 *              info
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
#include "WxStuff.h"
#include "STopWindow.h"
#include "SToolBar.h"
#include "Misc.h"
#include <wx/aui/aui.h>


/*******************************************************************
 * STOPWINDOW CLASS FUNCTIONS
 *******************************************************************/

/* STopWindow::STopWindow
 * STopWindow class constructor
 *******************************************************************/
STopWindow::STopWindow(string title, string id, int x, int y, int width, int height)
	: wxFrame(NULL, -1, title, wxPoint(x, y), wxSize(width, height))
{
	// Enable fullscreen mode on OSX
	EnableFullScreenView(true);

	// Init size/pos
	Misc::winf_t info = Misc::getWindowInfo(id);
	if (!info.id.IsEmpty())
	{
		SetSize(info.width, info.height);
		SetPosition(wxPoint(info.left, info.top));
	}
	else
		Misc::setWindowInfo(id, width, height, x, y);

	// Init variables
	custom_menus_begin = 0;
	this->id = id;

	// Bind events
	//Bind(wxEVT_SIZE, &STopWindow::onSize, this);
	//Bind(wxEVT_MOVE, &STopWindow::onMove, this);
}

/* STopWindow::~STopWindow
 * STopWindow class destructor
 *******************************************************************/
STopWindow::~STopWindow()
{
	if (!IsMaximized())
		Misc::setWindowInfo(id, GetSize().x, GetSize().y, GetPosition().x, GetPosition().y);
}

/* STopWindow::addCustomMenu
 * Adds [menu] to the menu bar after the 'Entry' menu
 *******************************************************************/
void STopWindow::addCustomMenu(wxMenu* menu, string title)
{
	// Check menu doesn't already exist
	for (unsigned a = 0; a < custom_menus.size(); a++)
	{
		if (custom_menus[a] == menu)
			return;
	}

	// Insert custom menu after the last existing custom menu
	GetMenuBar()->Insert(custom_menus_begin + custom_menus.size(), menu, title);
	GetMenuBar()->Refresh();
	custom_menus.push_back(menu);
}

/* STopWindow::removeCustomMenu
 * Removes the menu matching [title] from the menu bar
 *******************************************************************/
void STopWindow::removeCustomMenu(wxMenu* menu)
{
	// Go through custom menus
	for (unsigned a = 0; a < custom_menus.size(); a++)
	{
		if (custom_menus[a] == menu)
		{
			// Menu found, remove it
			custom_menus.erase(custom_menus.begin() + a);
			GetMenuBar()->Remove(custom_menus_begin + a);
			return;
		}
	}
}

/* STopWindow::removeAllCustomMenus
 * Removes all custom menus from the menu bar
 *******************************************************************/
void STopWindow::removeAllCustomMenus()
{
	// Remove all custom menus from menubar
	while (custom_menus.size() > 0)
		removeCustomMenu(custom_menus[0]);
}

/* STopWindow::enableToolBar
 * Enables/disables the toolbar group matching [name]
 *******************************************************************/
void STopWindow::enableToolBar(string name, bool enable)
{
	toolbar->enableGroup(name, enable);
}

/* STopWindow::addCustomToolBar
 * Adds a custom toolbar group to the toolbar, with buttons for each
 * action in [actions]
 *******************************************************************/
void STopWindow::addCustomToolBar(string name, wxArrayString actions)
{
	toolbar->addActionGroup(name, actions);
}

/* STopWindow::removeCustomToolBar
 * Removes the toolbar group matching [name]
 *******************************************************************/
void STopWindow::removeCustomToolBar(string name)
{
	toolbar->deleteGroup(name);
}

/* STopWindow::removeAllCustomToolBars
 * Removes all custom toolbar groups
 *******************************************************************/
void STopWindow::removeAllCustomToolBars()
{
	toolbar->deleteCustomGroups();
}


/*******************************************************************
 * STOPWINDOW CLASS FUNCTIONS
 *******************************************************************/

/* STopWindow::onSize
 * Called when the window is resized
 *******************************************************************/
void STopWindow::onSize(wxSizeEvent& e)
{
	// Update window size settings, but only if not maximized
	if (!IsMaximized())
		Misc::setWindowInfo(id, GetSize().x, GetSize().y, -2, -2);

	e.Skip();
}

/* STopWindow::onMove
 * Called when the window is moved
 *******************************************************************/
void STopWindow::onMove(wxMoveEvent& e)
{
	// Update window position settings, but only if not maximized
	if (!IsMaximized())
		Misc::setWindowInfo(id, -2, -2, GetPosition().x, GetPosition().y);

	e.Skip();
}
