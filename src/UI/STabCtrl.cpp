
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    STabCtrl.cpp
 * Description: A wxAuiNotebook specialisation that uses SAuiTabArt
 *              provider and has some other minor improvements
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
#include "SAuiTabArt.h"
#include "STabCtrl.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, tabs_condensed, false, CVAR_SAVE)


/*******************************************************************
 * STABCTRL CLASS FUNCTIONS
 *******************************************************************/

/* STabCtrl::STabCtrl
 * STabCtrl class constructor
 *******************************************************************/
STabCtrl::STabCtrl(
	wxWindow* parent,
	bool close_buttons,
	bool window_list,
	int height,
	bool main_tabs,
	bool move_tabs
) :	wxAuiNotebook()
{
	// Init height
	if (height < 0)
		height = tabs_condensed ? 24 : 27;

	// Determine style
	long style = wxAUI_NB_TOP | wxAUI_NB_SCROLL_BUTTONS;
	if (move_tabs)
		style |= wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE;
	if (window_list)
		style |= wxAUI_NB_WINDOWLIST_BUTTON;
	if (close_buttons)
		style |= wxAUI_NB_CLOSE_ON_ALL_TABS | wxAUI_NB_MIDDLE_CLICK_CLOSE;

	// Create tab control
	wxAuiNotebook::Create(parent, -1, wxDefaultPosition, wxDefaultSize, style);

	// Setup tabs
	SetArtProvider(new SAuiTabArt(close_buttons, main_tabs));
	SetTabCtrlHeight(height);
}

/* STabCtrl::~STabCtrl
 * STabCtrl class destructor
 *******************************************************************/
STabCtrl::~STabCtrl()
{
}

/* STabCtrl::DoGetBestClientSize
 * wxAuiNotebook doesn't automatically set its own minimum size to
 * the minimum size of its contents, so we have to do that for it.
 * See http://trac.wxwidgets.org/ticket/4698
 *******************************************************************/
wxSize STabCtrl::DoGetBestClientSize() const
{
	wxSize ret;
	for (unsigned i = 0; i < GetPageCount(); i++)
	{
		wxWindow* page = GetPage(i);
		ret.IncTo(page->GetBestSize());
	}

	ret.IncBy(0, GetTabCtrlHeight());

	return ret;
}


/*******************************************************************
 * STABCTRL CLASS STATIC FUNCTIONS
 *******************************************************************/

/* STabCtrl::createControl
 * Creates a platform-dependant tab control (STabCtrl in Windows,
 * wxNotebook anywhere else)
 *******************************************************************/
TabControl* STabCtrl::createControl(
	wxWindow* parent,
	bool close_buttons,
	bool window_list,
	int height,
	bool main_tabs,
	bool move_tabs)
{
#ifdef WIN32
	return new STabCtrl(parent, close_buttons, window_list, height, main_tabs, move_tabs);
#else
	return new wxNotebook(parent, -1);
#endif
}
