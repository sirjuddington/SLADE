
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "General/Misc.h"
#include "General/SAction.h"
#include "SToolBar/SToolBar.h"
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
#if __APPLE__ && ((wxMAJOR_VERSION == 3 && wxMINOR_VERSION >= 1) || wxMAJOR_VERSION > 3)
	EnableFullScreenView(true);
#endif

#ifndef __WXOSX__
	// Init size/pos
	auto info = misc::getWindowInfo(id_);
	if (!info.id.empty())
	{
		SetSize(info.width, info.height);
		SetPosition(wxPoint(info.left, info.top));
	}
	else
		misc::setWindowInfo(id_, width, height, x, y);
#endif

	// Init toolbar menu action(s)
	action_toolbar_menu_ = new SAction(
		fmt::format("{}_toolbar_menu", id), "Toolbars", "", "", "", SAction::Type::Check, -1, 10);
	action_toolbar_menu_->initWxId();
	SAction::add(action_toolbar_menu_);

	// Bind events
	Bind(wxEVT_MENU, &STopWindow::onMenu, this);
}

// -----------------------------------------------------------------------------
// STopWindow class destructor
// -----------------------------------------------------------------------------
STopWindow::~STopWindow()
{
	if (!wxFrame::IsMaximized() && !wxFrame::IsFullScreen())
		misc::setWindowInfo(id_, GetSize().x, GetSize().y, GetPosition().x, GetPosition().y);
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
// Enables/disables the toolbar group matching [name]
// -----------------------------------------------------------------------------
void STopWindow::enableToolBar(const string& name, bool enable) const
{
	toolbar_->enableGroup(name, enable);
}

// -----------------------------------------------------------------------------
// Adds a custom toolbar group to the toolbar, with buttons for each action in
// [actions]
// -----------------------------------------------------------------------------
void STopWindow::addCustomToolBar(const string& name, const vector<string>& actions) const
{
	toolbar_->addActionGroup(name, actions);
	populateToolbarsMenu();
}

// -----------------------------------------------------------------------------
// Removes the toolbar group matching [name]
// -----------------------------------------------------------------------------
void STopWindow::removeCustomToolBar(const string& name) const
{
	toolbar_->deleteGroup(name);
	populateToolbarsMenu();
}

// -----------------------------------------------------------------------------
// Removes all custom toolbar groups
// -----------------------------------------------------------------------------
void STopWindow::removeAllCustomToolBars() const
{
	toolbar_->deleteCustomGroups();
	populateToolbarsMenu();
}

// -----------------------------------------------------------------------------
// Populates the toolbars menu with options to toggle each toolbar group
// -----------------------------------------------------------------------------
void STopWindow::populateToolbarsMenu() const
{
	while (toolbar_menu_->GetMenuItemCount() > 0)
		toolbar_menu_->Delete(toolbar_menu_->FindItemByPosition(0));

	for (auto a = 0u; a < toolbar_->groups().size(); ++a)
	{
		auto group = toolbar_->groups()[a];

		auto name = group->name();
		strutil::replaceIP(name, "_", "");

		action_toolbar_menu_->addToMenu(toolbar_menu_, 0, name, "NO", a + 1);
		toolbar_menu_->GetMenuItems()[toolbar_menu_->GetMenuItemCount() - 1]->Check(!group->hidden());
	}
}


// -----------------------------------------------------------------------------
//
// STopWindow Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a menu item is selected
// -----------------------------------------------------------------------------
void STopWindow::onMenu(wxCommandEvent& e)
{
	if (action_toolbar_menu_->isWxId(e.GetId()))
	{
		int  offset = e.GetId() - action_toolbar_menu_->wxId() - 1;
		auto group  = toolbar_->groups()[offset];

		group->hide(!group->hidden());
		toolbar_->updateLayout(true);

		auto item = toolbar_menu_->FindItem(e.GetId());
		item->Check(!group->hidden());

		return;
	}

	e.Skip();
}
