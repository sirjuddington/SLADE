
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ThingTypeBrowser.cpp
// Description: Specialisation of BrowserWindow to show and browse for thing
//              types
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
#include "ThingTypeBrowser.h"
#include "Game/Configuration.h"
#include "General/UI.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/Drawing.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, browser_thing_tiles, true, CVar::Flag::Save)
CVAR(Bool, use_zeth_icons, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// ThingBrowserItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Loads the item image
// -----------------------------------------------------------------------------
bool ThingBrowserItem::loadImage()
{
	// Get sprite
	auto tex = mapeditor::textureManager()
				   .sprite(thing_type_->sprite(), thing_type_->translation(), thing_type_->palette())
				   .gl_id;
	if (!tex && use_zeth_icons && thing_type_->zethIcon() >= 0)
	{
		// Sprite not found, try the Zeth icon
		tex = mapeditor::textureManager()
				  .editorImage(fmt::format("zethicons/zeth{:02d}", thing_type_->zethIcon()))
				  .gl_id;
	}
	if (!tex)
	{
		// Sprite not found, try an icon
		tex = mapeditor::textureManager().editorImage(fmt::format("thing/{}", thing_type_->icon())).gl_id;
	}
	if (!tex)
	{
		// Icon not found either, use unknown icon
		tex = mapeditor::textureManager().editorImage("thing/unknown").gl_id;
	}

	if (tex)
	{
		image_tex_ = tex;
		return true;
	}
	else
		return false;
}


// -----------------------------------------------------------------------------
//
// ThingTypeBrowser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ThingTypeBrowser class constructor
// -----------------------------------------------------------------------------
ThingTypeBrowser::ThingTypeBrowser(wxWindow* parent, int type) : BrowserWindow(parent)
{
	// Set window title
	wxTopLevelWindow::SetTitle("Browse Thing Types");

	// Add 'Details view' checkbox
	cb_view_tiles_ = new wxCheckBox(this, -1, "Details view");
	cb_view_tiles_->SetValue(browser_thing_tiles);
	sizer_bottom_->Add(cb_view_tiles_, 0, wxEXPAND | wxRIGHT, ui::pad());

	// Populate tree
	auto& types = game::configuration().allThingTypes();
	for (auto& i : types)
		addItem(new ThingBrowserItem(i.second.name(), i.second, i.first), i.second.group());
	populateItemTree();

	// Set browser options
	canvas_->setItemNameType(BrowserCanvas::NameType::Index);
	setupViewOptions();

	// Select initial item if any
	if (type >= 0)
		selectItem(game::configuration().thingType(type).name());
	else
		openTree(items_root_); // Otherwise open 'all' category


	// Bind events
	cb_view_tiles_->Bind(wxEVT_CHECKBOX, &ThingTypeBrowser::onViewTilesClicked, this);

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Sets up appropriate browser view options
// -----------------------------------------------------------------------------
void ThingTypeBrowser::setupViewOptions()
{
	if (browser_thing_tiles)
	{
		setFont(drawing::Font::Condensed);
		setItemSize(48);
		setItemViewType(BrowserCanvas::ItemView::Tiles);
	}
	else
	{
		setFont(drawing::Font::Bold);
		setItemSize(80);
		setItemViewType(BrowserCanvas::ItemView::Normal);
	}

	canvas_->updateLayout();
	canvas_->showSelectedItem();
}

// -----------------------------------------------------------------------------
// Returns the currently selected thing type
// -----------------------------------------------------------------------------
int ThingTypeBrowser::selectedType() const
{
	auto selected = selectedItem();
	if (selected)
	{
		log::info(wxString::Format("Selected item %d", selected->index()));
		return selected->index();
	}
	else
		return -1;
}


// -----------------------------------------------------------------------------
//
// ThingTypeBrowser Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Details View' checkbox is changed
// -----------------------------------------------------------------------------
void ThingTypeBrowser::onViewTilesClicked(wxCommandEvent& e)
{
	browser_thing_tiles = cb_view_tiles_->GetValue();
	setupViewOptions();
	Refresh();
}
