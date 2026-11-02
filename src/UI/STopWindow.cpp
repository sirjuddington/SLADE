
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    STopWindow.cpp
// Description: Base class for top level windows. Handles custom menus and
//              toolbars, also saved position and size info
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
#include "STopWindow.h"
#include "SAuiToolBar.h"
#include "UI.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// STopWindow Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// STopWindow class constructor
// -----------------------------------------------------------------------------
STopWindow::STopWindow(const string& title, const string& id, int x, int y, int width, int height)
#ifndef __WXOSX__
	:
	wxFrame(nullptr, -1, wxString::FromUTF8(title), wxPoint(x, y), wxSize(width, height)),
#else
	:
	wxFrame(nullptr, -1, wxString::FromUTF8(title), wxDefaultPosition, wxSize(width, height)),
#endif
	id_{ id }
{
	// Enable fullscreen mode on OSX
#ifdef __APPLE__
	EnableFullScreenView(true);
#endif

#ifndef __WXOSX__
	// Init size/pos
	auto info = ui::getWindowInfo(this, id_);
	if (!info.id.empty())
	{
		SetSize(info.width, info.height);
		SetPosition(wxPoint(info.left, info.top));
	}
	else
		ui::setWindowInfo(this, id_, width, height, x, y);
#endif

	// Bind events
	Bind(
		wxEVT_CLOSE_WINDOW,
		[this](wxCloseEvent&)
		{
			if (!wxFrame::IsMaximized() && !wxFrame::IsFullScreen())
				ui::setWindowInfo(this, id_, GetSize().x, GetSize().y, GetPosition().x, GetPosition().y);
		});
}

// -----------------------------------------------------------------------------
// Adds [menu] to the menu bar after the 'Entry' menu
// -----------------------------------------------------------------------------
void STopWindow::addCustomMenu(wxMenu* menu, const string& title)
{
	// Check menu doesn't already exist
	for (auto& custom_menu : custom_menus_)
		if (custom_menu == menu)
			return;

	// Insert custom menu after the last existing custom menu
	GetMenuBar()->Insert(custom_menus_begin_ + custom_menus_.size(), menu, wxString::FromUTF8(title));
	GetMenuBar()->Refresh();
	custom_menus_.push_back(menu);
}

// -----------------------------------------------------------------------------
// Removes the menu matching [title] from the menu bar
// -----------------------------------------------------------------------------
void STopWindow::removeCustomMenu(const wxMenu* menu)
{
	// Go through custom menus
	for (unsigned a = 0; a < custom_menus_.size(); a++)
	{
		if (custom_menus_[a] == menu)
		{
			// Menu found, remove it
			custom_menus_.erase(custom_menus_.begin() + a);
			GetMenuBar()->Remove(custom_menus_begin_ + a);
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Removes all custom menus from the menu bar
// -----------------------------------------------------------------------------
void STopWindow::removeAllCustomMenus()
{
	// Remove all custom menus from menubar
	while (!custom_menus_.empty())
		removeCustomMenu(custom_menus_[0]);
}

// -----------------------------------------------------------------------------
// Shows/hides the toolbar group matching [name]
// -----------------------------------------------------------------------------
void STopWindow::showToolbarGroup(string_view name, bool show) const
{
	toolbar_->showGroup(name, show);
}

// -----------------------------------------------------------------------------
// Enables/disables the toolbar group matching [name]
// -----------------------------------------------------------------------------
void STopWindow::enableToolBarGroup(const string& name, bool enable) const
{
	toolbar_->enableGroup(name, enable);
}
