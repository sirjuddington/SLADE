
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "STopWindow.h"
#include "General/Misc.h"
#include "SToolBar/SToolBar.h"
#include "General/SAction.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
STopWindow* STopWindow::current_active_window_ = nullptr;


// ----------------------------------------------------------------------------
//
// STopWindow Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// STopWindow::STopWindow
//
// STopWindow class constructor
// ----------------------------------------------------------------------------
STopWindow::STopWindow(string title, string id, int x, int y, int width, int height)
#ifndef __WXOSX__
	: wxFrame(nullptr, -1, title, wxPoint(x, y), wxSize(width, height))
#else
	: wxFrame(nullptr, -1, title, wxDefaultPosition, wxSize(width, height))
#endif
{
	// Enable fullscreen mode on OSX
#if __APPLE__ && ((wxMAJOR_VERSION == 3 && wxMINOR_VERSION >= 1) || wxMAJOR_VERSION > 3)
	EnableFullScreenView(true);
#endif

#ifndef __WXOSX__
	// Init size/pos
	auto info = Misc::getWindowInfo(id);
	if (!info.id.IsEmpty())
	{
		SetSize(info.width, info.height);
		SetPosition(wxPoint(info.left, info.top));
	}
	else
		Misc::setWindowInfo(id, width, height, x, y);
#endif

	// Init variables
	custom_menus_begin_ = 0;
	id_ = id;

	// Init toolbar menu action(s)
	action_toolbar_menu_ = new SAction(
		S_FMT("%s_toolbar_menu", CHR(id)),
		"Toolbars",
		"",
		"",
		"",
		SAction::Type::Check,
		-1,
		10
	);
	SAction::add(action_toolbar_menu_);

	// Bind events
	Bind(wxEVT_MENU, &STopWindow::onMenu, this);
	Bind(wxEVT_ACTIVATE, [&](wxActivateEvent& e)
	{
		if (e.GetActive())
			current_active_window_ = this;
	});
}

// ----------------------------------------------------------------------------
// STopWindow::~STopWindow
//
// STopWindow class destructor
// ----------------------------------------------------------------------------
STopWindow::~STopWindow()
{
	if (!wxFrame::IsMaximized() && !wxFrame::IsFullScreen())
		Misc::setWindowInfo(id_, GetSize().x, GetSize().y, GetPosition().x, GetPosition().y);
}

// ----------------------------------------------------------------------------
// STopWindow::addCustomMenu
//
// Adds [menu] to the menu bar after the 'Entry' menu
// ----------------------------------------------------------------------------
void STopWindow::addCustomMenu(wxMenu* menu, string title)
{
	// Check menu doesn't already exist
	for (unsigned a = 0; a < custom_menus_.size(); a++)
	{
		if (custom_menus_[a] == menu)
			return;
	}

	// Insert custom menu after the last existing custom menu
	GetMenuBar()->Insert(custom_menus_begin_ + custom_menus_.size(), menu, title);
	GetMenuBar()->Refresh();
	custom_menus_.push_back(menu);
}

// ----------------------------------------------------------------------------
// STopWindow::removeCustomMenu
//
// Removes the menu matching [title] from the menu bar
// ----------------------------------------------------------------------------
void STopWindow::removeCustomMenu(wxMenu* menu)
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

// ----------------------------------------------------------------------------
// STopWindow::removeAllCustomMenus
//
// Removes all custom menus from the menu bar
// ----------------------------------------------------------------------------
void STopWindow::removeAllCustomMenus()
{
	// Remove all custom menus from menubar
	while (custom_menus_.size() > 0)
		removeCustomMenu(custom_menus_[0]);
}

// ----------------------------------------------------------------------------
// STopWindow::enableToolBar
//
// Enables/disables the toolbar group matching [name]
// ----------------------------------------------------------------------------
void STopWindow::enableToolBar(string name, bool enable)
{
	toolbar_->enableGroup(name, enable);
}

// ----------------------------------------------------------------------------
// STopWindow::addCustomToolBar
//
// Adds a custom toolbar group to the toolbar, with buttons for each action in
// [actions]
// ----------------------------------------------------------------------------
void STopWindow::addCustomToolBar(string name, wxArrayString actions)
{
	toolbar_->addActionGroup(name, actions);
	populateToolbarsMenu();
}

// ----------------------------------------------------------------------------
// STopWindow::removeCustomToolBar
//
// Removes the toolbar group matching [name]
// ----------------------------------------------------------------------------
void STopWindow::removeCustomToolBar(string name)
{
	toolbar_->deleteGroup(name);
	populateToolbarsMenu();
}

// ----------------------------------------------------------------------------
// STopWindow::removeAllCustomToolBars
//
// Removes all custom toolbar groups
// ----------------------------------------------------------------------------
void STopWindow::removeAllCustomToolBars()
{
	toolbar_->deleteCustomGroups();
	populateToolbarsMenu();
}

// ----------------------------------------------------------------------------
// STopWindow::populateToolbarsMenu
//
// Populates the toolbars menu with options to toggle each toolbar group
// ----------------------------------------------------------------------------
void STopWindow::populateToolbarsMenu() const
{
	while (toolbar_menu_->FindItemByPosition(0))
		toolbar_menu_->Delete(toolbar_menu_->FindItemByPosition(0));

	for (auto a = 0u; a < toolbar_->groups().size(); ++a)
	{
		auto group = toolbar_->groups()[a];

		string name = group->name();
		name.Replace("_", "");

		action_toolbar_menu_->addToMenu(toolbar_menu_, name, "NO", a+1);
		toolbar_menu_->GetMenuItems()[toolbar_menu_->GetMenuItemCount() - 1]->Check(!group->hidden());
	}
}


// ----------------------------------------------------------------------------
//
// STopWindow Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// STopWindow::onMenu
//
// Called when a menu item is selected
// ----------------------------------------------------------------------------
void STopWindow::onMenu(wxCommandEvent& e)
{
	if (action_toolbar_menu_->isWxId(e.GetId()))
	{
		int offset = e.GetId() - action_toolbar_menu_->getWxId() - 1;
		auto group = toolbar_->groups()[offset];

		group->hide(!group->hidden());
		toolbar_->updateLayout(true);
		
		auto item = toolbar_menu_->FindItem(e.GetId());
		item->Check(!group->hidden());

		return;
	}

	e.Skip();
}
