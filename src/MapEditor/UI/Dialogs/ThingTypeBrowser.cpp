
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
#include "Game/ThingType.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/Draw2D.h"
#include "UI/Browser/BrowserCanvas.h"
#include "UI/Browser/BrowserItem.h"
#include "UI/Layout.h"

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
// ThingBrowserItem Class
//
// -----------------------------------------------------------------------------
namespace slade
{
class ThingBrowserItem : public BrowserItem
{
public:
	ThingBrowserItem(const string& name, const game::ThingType& type, unsigned index) :
		BrowserItem{ name, index },
		thing_type_{ &type }
	{
	}
	~ThingBrowserItem() override = default;

	// Loads the item image
	bool loadImage() override
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

private:
	game::ThingType const* thing_type_;
};
} // namespace slade


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
	wxTopLevelWindow::SetTitle(wxS("Browse Thing Types"));

	// Add 'Details view' checkbox
	auto lh        = ui::LayoutHelper(this);
	cb_view_tiles_ = new wxCheckBox(this, -1, wxS("Details view"));
	cb_view_tiles_->SetValue(browser_thing_tiles);
	sizer_bottom_->Add(cb_view_tiles_, lh.sfWithBorder(0, wxRIGHT).Expand());

	// Populate tree
	auto& types = game::configuration().allThingTypes();
	for (auto& i : types)
		addItem(new ThingBrowserItem(i.second.name(), i.second, i.first), i.second.group());
	populateItemTree();

	// Set browser options
	canvas_->setItemNameType(browser::NameType::Index);
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
		setFont(gl::draw2d::Font::Condensed);
		setItemSize(48);
		setItemViewType(browser::ItemView::Tiles);
	}
	else
	{
		setFont(gl::draw2d::Font::Bold);
		setItemSize(80);
		setItemViewType(browser::ItemView::Normal);
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
		log::info("Selected item {}", selected->index());
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
