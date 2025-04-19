
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapTextureBrowser.cpp
// Description: Specialisation of BrowserWindow to show and browse for map
//              textures/flats
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
#include "MapTextureBrowser.h"
#include "Game/Configuration.h"
#include "General/ResourceManager.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, map_tex_sort, 2, CVar::Flag::Save)
CVAR(String, map_tex_treespec, "type,archive,category", CVar::Flag::Save)
const string MapTexBrowserItem::TEXTURE = "texture";
const string MapTexBrowserItem::FLAT    = "flat";


// -----------------------------------------------------------------------------
//
// MapTexBrowserItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapTexBrowserItem class constructor
// -----------------------------------------------------------------------------
MapTexBrowserItem::MapTexBrowserItem(const string& name, const string& type, unsigned index) :
	BrowserItem(name, index, type)
{
	// Check for blank texture
	if (name == "-" && type == TEXTURE)
		blank_ = true;
}

// -----------------------------------------------------------------------------
// Loads the item image
// -----------------------------------------------------------------------------
bool MapTexBrowserItem::loadImage()
{
	const MapTextureManager::Texture* tex = nullptr;

	// Get texture or flat depending on type
	if (type_ == "texture")
		tex = &mapeditor::textureManager().texture(name_, false);
	else if (type_ == "flat")
		tex = &mapeditor::textureManager().flat(name_, false);

	if (tex)
	{
		image_tex_ = tex->gl_id;
		scale_     = tex->scale;
		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns a string with extra information about the texture/flat
// -----------------------------------------------------------------------------
string MapTexBrowserItem::itemInfo()
{
	string info;

	// Check for blank texture
	if (name_ == "-")
		return "No Texture";

	// Add dimensions if known
	auto& tex_info = gl::Texture::info(image_tex_);
	if (image_tex_ || loadImage())
		info += fmt::format("{}x{}", tex_info.size.x, tex_info.size.y);
	else
		info += "Unknown size";

	// Add type
	if (type_ == "texture")
		info += ", Texture";
	else
		info += ", Flat";

	// Add scaling info
	if (scale_.x != 1. || scale_.y != 1.)
		info += ", Scaled";

	// Add usage count
	info += fmt::format(", Used {} times", usage_count_);

	return info;
}


// -----------------------------------------------------------------------------
//
// MapTextureBrowser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapTextureBrowser class constructor
// -----------------------------------------------------------------------------
MapTextureBrowser::MapTextureBrowser(wxWindow* parent, TextureType type, string_view texture, SLADEMap* map) :
	BrowserWindow(parent, true),
	type_{ type },
	map_{ map }
{
	// Init sorting
	addSortType("Usage Count");
	setSortType(map_tex_sort);

	// Set window title
	wxTopLevelWindow::SetTitle(wxS("Browse Map Textures"));

	auto map_format = map->currentFormat();

	// Textures
	if (type == TextureType::Texture || game::configuration().featureSupported(game::Feature::MixTexFlats))
	{
		addGlobalItem(new MapTexBrowserItem("-", MapTexBrowserItem::TEXTURE, 0));

		auto& textures = mapeditor::textureManager().allTexturesInfo();
		for (unsigned a = 0; a < textures.size(); a++)
		{
			if ((map_format != MapFormat::UDMF || !game::configuration().featureSupported(game::Feature::LongNames))
				&& textures[a].short_name.size() > 8)
			{
				// Only UDMF supports texture/flat names longer than 8 characters
				continue;
			}

			// Don't add two textures with the same name
			bool dont_add = false;
			for (unsigned b = 0; b < textures.size(); b++)
			{
				if (textures[b].short_name == textures[a].short_name && b > a)
				{
					dont_add = true;
					break;
				}
			}

			if (dont_add)
				continue;

			// Add browser item
			addItem(
				new MapTexBrowserItem(textures[a].short_name, MapTexBrowserItem::TEXTURE, textures[a].index),
				determineTexturePath(textures[a].archive, textures[a].category, "Textures", textures[a].path));
		}
	}

	// Flats
	if (type == TextureType::Flat || game::configuration().featureSupported(game::Feature::MixTexFlats))
	{
		auto& flats = mapeditor::textureManager().allFlatsInfo();
		for (unsigned a = 0; a < flats.size(); a++)
		{
			if ((map_format != MapFormat::UDMF || !game::configuration().featureSupported(game::Feature::LongNames))
				&& flats[a].short_name.size() > 8)
			{
				// Only UDMF supports texture/flat names longer than 8 characters
				continue;
			}

			// Don't add two flats with the same name
			bool dont_add = false;
			for (unsigned b = 0; b < flats.size(); b++)
			{
				if (flats[b].short_name == flats[a].short_name && b > a)
				{
					dont_add = true;
					break;
				}
			}
			if (dont_add)
				continue;

			// Determine tree path
			auto flat_path = flats[a].path;
			if (strutil::startsWithCI(flat_path, "flats/"))
				flat_path = flat_path.substr(6);
			auto path = determineTexturePath(flats[a].archive, flats[a].category, "Flats", flat_path);

			// Add browser item
			if (flats[a].category == MapTextureManager::Category::ZDTextures)
				addItem(new MapTexBrowserItem(flats[a].short_name, MapTexBrowserItem::TEXTURE, flats[a].index), path);
			else
				addItem(new MapTexBrowserItem(flats[a].short_name, MapTexBrowserItem::FLAT, flats[a].index), path);
		}
	}

	// Full path textures
	if (map_format == MapFormat::UDMF && game::configuration().featureSupported(game::Feature::LongNames))
	{
		// Textures
		auto& fp_textures = mapeditor::textureManager().allTexturesInfo();
		for (auto& tex : fp_textures)
		{
			if (tex.category != MapTextureManager::Category::ZDTextures
				&& tex.category != MapTextureManager::Category::HiRes && !tex.path.empty() && tex.path != "/")
			{
				// Add browser item
				addItem(
					new MapTexBrowserItem(tex.long_name, MapTexBrowserItem::TEXTURE, tex.index),
					determineTexturePath(tex.archive, tex.category, "Textures (Full Path)", tex.path));
			}
		}

		// Flats
		auto& fp_flats = mapeditor::textureManager().allFlatsInfo();
		for (auto& flat : fp_flats)
		{
			if (!flat.path.empty() && flat.path != "/")
			{
				// Add browser item
				addItem(
					new MapTexBrowserItem(flat.long_name, MapTexBrowserItem::FLAT, flat.index),
					determineTexturePath(flat.archive, flat.category, "Textures (Full Path)", flat.path));
			}
		}
	}

	populateItemTree(false);

	// Select initial texture (if any)
	selectItem(texture);
}

// -----------------------------------------------------------------------------
// Builds and returns the tree item path for [info]
// -----------------------------------------------------------------------------
string MapTextureBrowser::determineTexturePath(
	Archive*                    archive,
	MapTextureManager::Category category,
	const string&               type,
	const string&               path) const
{
	auto   tree_spec = strutil::splitV(map_tex_treespec, ',');
	string ret;
	for (const auto& b : tree_spec)
	{
		if (b == "archive")
			ret += archive->filename(false);
		else if (b == "type")
			ret += type;
		else if (b == "category")
		{
			switch (category)
			{
			case MapTextureManager::Category::TextureX: ret += "TEXTUREx"; break;
			case MapTextureManager::Category::ZDTextures: ret += "TEXTURES"; break;
			case MapTextureManager::Category::HiRes: ret += "HIRESTEX"; break;
			case MapTextureManager::Category::Tx: ret += "Single (TX)"; break;
			default: continue;
			}
		}

		ret += "/";
	}

	if (category == MapTextureManager::Category::Tx && path.length() > 8)
		return ret + path.substr(9);
	else
		return ret + path;
}

// -----------------------------------------------------------------------------
// Returns true if [left] has higher usage count than [right].
// If both are equal it will go alphabetically by name
// -----------------------------------------------------------------------------
bool sortBIUsage(BrowserItem* left, BrowserItem* right)
{
	// Sort alphabetically if usage counts are equal
	if (dynamic_cast<MapTexBrowserItem*>(left)->usageCount() == dynamic_cast<MapTexBrowserItem*>(right)->usageCount())
		return left->name() < right->name();
	else
		return dynamic_cast<MapTexBrowserItem*>(left)->usageCount()
			   > dynamic_cast<MapTexBrowserItem*>(right)->usageCount();
}

// -----------------------------------------------------------------------------
// Sort the current items depending on [sort_type]
// -----------------------------------------------------------------------------
void MapTextureBrowser::doSort(unsigned sort_type)
{
	map_tex_sort = sort_type;

	// Default sorts
	if (sort_type < 2)
		return BrowserWindow::doSort(sort_type);

	// Sort by usage
	else if (sort_type == 2)
	{
		updateUsage();
		auto& items = canvas_->itemList();
		std::sort(items.begin(), items.end(), sortBIUsage);
	}
}

// -----------------------------------------------------------------------------
// Updates usage counts for all browser items
// -----------------------------------------------------------------------------
void MapTextureBrowser::updateUsage() const
{
	if (!map_)
		return;

	auto& items = canvas_->itemList();
	for (auto& i : items)
	{
		auto item = dynamic_cast<MapTexBrowserItem*>(i);
		if (type_ == TextureType::Texture)
			item->setUsage(map_->sides().texUsageCount(item->name()));
		else
			item->setUsage(map_->sectors().texUsageCount(item->name()));
	}
}
