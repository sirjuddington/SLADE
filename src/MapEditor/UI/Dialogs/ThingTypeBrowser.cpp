
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ThingTypeBrowser.cpp
 * Description: Specialisation of BrowserWindow to show and browse
 *              for thing types
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
#include "Game/Configuration.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/Drawing.h"
#include "ThingTypeBrowser.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, browser_thing_tiles, true, CVAR_SAVE)
CVAR(Bool, use_zeth_icons, false, CVAR_SAVE)


/*******************************************************************
 * THINGBROWSERITEM CLASS FUNCTIONS
 *******************************************************************/

/* ThingBrowserItem::ThingBrowserItem
 * ThingBrowserItem class constructor
 *******************************************************************/
ThingBrowserItem::ThingBrowserItem(string name, const Game::ThingType& type, unsigned index) :
	BrowserItem(name, index),
	type{type}
{
}

/* ThingBrowserItem::~ThingBrowserItem
 * ThingBrowserItem class destructor
 *******************************************************************/
ThingBrowserItem::~ThingBrowserItem()
{
}

/* ThingBrowserItem::loadImage
 * Loads the item image
 *******************************************************************/
bool ThingBrowserItem::loadImage()
{
	// Get sprite
	GLTexture* tex = MapEditor::textureManager().getSprite(type.sprite(), type.translation(), type.palette());
	if (!tex && use_zeth_icons && type.zethIcon() >= 0)
	{
		// Sprite not found, try the Zeth icon
		tex = MapEditor::textureManager().getEditorImage(S_FMT("zethicons/zeth%02d", type.zethIcon()));
	}
	if (!tex)
	{
		// Sprite not found, try an icon
		tex = MapEditor::textureManager().getEditorImage(S_FMT("thing/%s", type.icon()));
	}
	if (!tex)
	{
		// Icon not found either, use unknown icon
		tex = MapEditor::textureManager().getEditorImage("thing/unknown");
	}

	if (tex)
	{
		image = tex;
		return true;
	}
	else
		return false;
}


/*******************************************************************
 * THINGTYPEBROWSER CLASS FUNCTIONS
 *******************************************************************/

/* ThingTypeBrowser::ThingTypeBrowser
 * ThingTypeBrowser class constructor
 *******************************************************************/
ThingTypeBrowser::ThingTypeBrowser(wxWindow* parent, int type) : BrowserWindow(parent)
{
	// Set window title
	SetTitle("Browse Thing Types");

	// Add 'Details view' checkbox
	cb_view_tiles = new wxCheckBox(this, -1, "Details view");
	cb_view_tiles->SetValue(browser_thing_tiles);
	sizer_bottom->Add(cb_view_tiles, 0, wxEXPAND|wxRIGHT, 4);

	// Populate tree
	auto& types = Game::configuration().allThingTypes();
	for (auto& i : types)
		addItem(new ThingBrowserItem(i.second.name(), i.second, i.first), i.second.group());
	populateItemTree();

	// Set browser options
	canvas->setItemNameType(BrowserCanvas::NAMES_INDEX);
	setupViewOptions();

	// Select initial item if any
	if (type >= 0)
		selectItem(Game::configuration().thingType(type).name());
	else
		openTree(items_root);	// Otherwise open 'all' category


	// Bind events
	cb_view_tiles->Bind(wxEVT_CHECKBOX, &ThingTypeBrowser::onViewTilesClicked, this);

	Layout();
}

/* ThingTypeBrowser::~ThingTypeBrowser
 * ThingTypeBrowser class destructor
 *******************************************************************/
ThingTypeBrowser::~ThingTypeBrowser()
{
}

/* ThingTypeBrowser::setupViewOptions
 * Sets up appropriate browser view options
 *******************************************************************/
void ThingTypeBrowser::setupViewOptions()
{
	if (browser_thing_tiles)
	{
		setFont(Drawing::FONT_CONDENSED);
		setItemSize(48);
		setItemViewType(BrowserCanvas::ITEMS_TILES);
	}
	else
	{
		setFont(Drawing::FONT_BOLD);
		setItemSize(80);
		setItemViewType(BrowserCanvas::ITEMS_NORMAL);
	}

	canvas->updateLayout();
	canvas->showSelectedItem();
}

/* ThingTypeBrowser::getSelectedType
 * Returns the currently selected thing type
 *******************************************************************/
int ThingTypeBrowser::getSelectedType()
{
	BrowserItem* selected = getSelectedItem();
	if (selected)
	{
		LOG_MESSAGE(1, "Selected item %d", selected->getIndex());
		return selected->getIndex();
	}
	else
		return -1;
}


/*******************************************************************
 * THINGTYPEBROWSER CLASS EVENTS
 *******************************************************************/

/* ThingTypeBrowser::onViewTilesClicked
 * Called when the 'Details View' checkbox is changed
 *******************************************************************/
void ThingTypeBrowser::onViewTilesClicked(wxCommandEvent& e)
{
	browser_thing_tiles = cb_view_tiles->GetValue();
	setupViewOptions();
	Refresh();
}
