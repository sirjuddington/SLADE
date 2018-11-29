
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/Drawing.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, browser_thing_tiles, true, CVAR_SAVE)
CVAR(Bool, use_zeth_icons, false, CVAR_SAVE)


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
	GLTexture* tex = MapEditor::textureManager().getSprite(type_.sprite(), type_.translation(), type_.palette());
	if (!tex && use_zeth_icons && type_.zethIcon() >= 0)
	{
		// Sprite not found, try the Zeth icon
		tex = MapEditor::textureManager().getEditorImage(S_FMT("zethicons/zeth%02d", type_.zethIcon()));
	}
	if (!tex)
	{
		// Sprite not found, try an icon
		tex = MapEditor::textureManager().getEditorImage(S_FMT("thing/%s", type_.icon()));
	}
	if (!tex)
	{
		// Icon not found either, use unknown icon
		tex = MapEditor::textureManager().getEditorImage("thing/unknown");
	}

	if (tex)
	{
		image_ = tex;
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
	SetTitle("Browse Thing Types");

	// Add 'Details view' checkbox
	cb_view_tiles_ = new wxCheckBox(this, -1, "Details view");
	cb_view_tiles_->SetValue(browser_thing_tiles);
	sizer_bottom_->Add(cb_view_tiles_, 0, wxEXPAND | wxRIGHT, UI::pad());

	// Populate tree
	auto& types = Game::configuration().allThingTypes();
	for (auto& i : types)
		addItem(new ThingBrowserItem(i.second.name(), i.second, i.first), i.second.group());
	populateItemTree();

	// Set browser options
	canvas_->setItemNameType(BrowserCanvas::NameType::Index);
	setupViewOptions();

	// Select initial item if any
	if (type >= 0)
		selectItem(Game::configuration().thingType(type).name());
	else
		openTree(items_root_); // Otherwise open 'all' category


	// Bind events
	cb_view_tiles_->Bind(wxEVT_CHECKBOX, &ThingTypeBrowser::onViewTilesClicked, this);

	Layout();
}

// -----------------------------------------------------------------------------
// Sets up appropriate browser view options
// -----------------------------------------------------------------------------
void ThingTypeBrowser::setupViewOptions()
{
	if (browser_thing_tiles)
	{
		setFont(Drawing::Font::Condensed);
		setItemSize(48);
		setItemViewType(BrowserCanvas::ItemView::Tiles);
	}
	else
	{
		setFont(Drawing::Font::Bold);
		setItemSize(80);
		setItemViewType(BrowserCanvas::ItemView::Normal);
	}

	canvas_->updateLayout();
	canvas_->showSelectedItem();
}

// -----------------------------------------------------------------------------
// Returns the currently selected thing type
// -----------------------------------------------------------------------------
int ThingTypeBrowser::getSelectedType()
{
	BrowserItem* selected = getSelectedItem();
	if (selected)
	{
		LOG_MESSAGE(1, "Selected item %d", selected->index());
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
