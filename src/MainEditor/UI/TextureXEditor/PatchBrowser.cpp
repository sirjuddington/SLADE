
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PatchBrowser.cpp
// Description: A specialisation of the Browser class for browsing the contents
//              of a patch table. Splits the patches into three categories -
//              'Base', 'Archive' and 'Unknown' for patches existing in the
//              base resource, the current archive, and entries not found,
//              respectively
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
#include "PatchBrowser.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
#include "General/ResourceManager.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "OpenGL/GLTexture.h"
#include "UI/Controls/PaletteChooser.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// PatchBrowserItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PatchBrowserItem class destructor
// -----------------------------------------------------------------------------
PatchBrowserItem::~PatchBrowserItem()
{
	gl::Texture::clear(image_tex_);
}

// -----------------------------------------------------------------------------
// Loads the item's image from its associated entry (if any)
// -----------------------------------------------------------------------------
bool PatchBrowserItem::loadImage()
{
	SImage img;

	// Load patch image
	if (type_ == Type::Patch)
	{
		// Find patch entry
		auto entry = app::resources().getPatchEntry(name_.ToStdString(), nspace_.ToStdString(), archive_);

		// Load entry to image, if it exists
		if (entry)
			misc::loadImageFromEntry(&img, entry);
		else
			return false;
	}

	// Or, load texture image
	if (type_ == Type::CTexture)
	{
		// Find texture
		auto tex = app::resources().getTexture(name_.ToStdString(), "", archive_);

		// Load texture to image, if it exists
		if (tex)
			tex->toImage(img, archive_, parent_->palette());
		else
			return false;
	}

	// Create gl texture from image
	gl::Texture::clear(image_tex_);
	image_tex_ = gl::Texture::createFromImage(img, parent_->palette());
	return image_tex_ > 0;
}

// -----------------------------------------------------------------------------
// Returns a string with extra information about the patch
// -----------------------------------------------------------------------------
wxString PatchBrowserItem::itemInfo()
{
	wxString info;

	// Add dimensions if known
	if (image_tex_)
	{
		auto& tex_info = gl::Texture::info(image_tex_);
		info += wxString::Format("%dx%d", tex_info.size.x, tex_info.size.y);
	}
	else
		info += "Unknown size";

	// Add patch type
	if (type_ == Type::Patch)
		info += ", Patch";
	else
		info += ", Texture";

	// Add namespace if it exists
	if (!nspace_.IsEmpty())
	{
		info += ", ";
		info += nspace_.Capitalize();
		info += " namespace";
	}

	return info;
}

// -----------------------------------------------------------------------------
// Clears the item image
// -----------------------------------------------------------------------------
void PatchBrowserItem::clearImage()
{
	gl::Texture::clear(image_tex_);
	image_tex_ = 0;
}


// -----------------------------------------------------------------------------
//
// PatchBrowser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PatchBrowser class constructor
// -----------------------------------------------------------------------------
PatchBrowser::PatchBrowser(wxWindow* parent) : BrowserWindow(parent)
{
	// Init browser tree
	items_root_->addChild("IWAD");
	items_root_->addChild("Custom");
	items_root_->addChild("Unknown");

	// Update when main palette changed
	sc_palette_changed_ = theMainWindow->paletteChooser()->signals().palette_changed.connect([this]() {
		// Update palette
		palette_.copyPalette(theMainWindow->paletteChooser()->selectedPalette());

		// Reload all items
		reloadItems();
		Refresh();
	});

	// Set dialog title
	wxTopLevelWindow::SetTitle("Browse Patches");
}

// -----------------------------------------------------------------------------
// Opens contents of the patch table [table] for browsing
// -----------------------------------------------------------------------------
bool PatchBrowser::openPatchTable(PatchTable* table)
{
	// Check table was given
	if (!table)
		return false;

	// Clear any existing browser items
	clearItems();

	// Setup palette chooser
	theMainWindow->paletteChooser()->setGlobalFromArchive(table->parent());

	// Go through patch table
	for (unsigned a = 0; a < table->nPatches(); a++)
	{
		// Get patch
		auto& patch = table->patch(a);

		// Init position to add
		wxString whereis = "Unknown";

		// Get patch entry
		auto entry = app::resources().getPatchEntry(patch.name);

		// Check its parent archive
		Archive* parent_archive = nullptr;
		if (entry)
		{
			parent_archive = entry->parent();

			if (parent_archive)
				whereis = parent_archive->filename(false);
		}

		// Add it
		auto item = new PatchBrowserItem(patch.name, parent_archive, PatchBrowserItem::Type::Patch, "", a);
		addItem(item, whereis);
	}

	// Update variables
	patch_table_ = table;

	// Open 'all' node
	openTree(items_root_);

	// Update tree control
	populateItemTree();

	return true;
}

// -----------------------------------------------------------------------------
// Opens all loaded resource patches and textures, prioritising those from
// [archive], except for the case of composite textures, which are ignored if
// in [archive]
// -----------------------------------------------------------------------------
bool PatchBrowser::openArchive(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return false;

	// Clear any existing browser items
	clearItems();

	// Init browser tree
	truncate_names_ = full_path_;
	items_root_->addChild("Patches");
	items_root_->addChild("Graphics");
	items_root_->addChild("Textures");
	items_root_->addChild("Flats");
	items_root_->addChild("Sprites");

	// Setup palette chooser
	theMainWindow->paletteChooser()->setGlobalFromArchive(archive);

	// Get a list of all available patch entries
	vector<ArchiveEntry*> patches;
	app::resources().putAllPatchEntries(patches, archive, full_path_);

	// Add flats, too
	{
		vector<ArchiveEntry*> flats;
		app::resources().putAllFlatEntries(flats, archive, full_path_);
		for (auto& flat : flats)
			if (flat->isInNamespace("flats") && flat->parent()->isTreeless())
				patches.push_back(flat);
		flats.clear();
	}

	// Determine whether one or more patches exists in a treeful archive
	if (full_path_)
	{
		bool bPatches = false, bGraphics = false, bTextures = false, bFlats = false, bSprites = false;
		for (auto entry : patches)
		{
			if (entry->parent()->isTreeless())
				continue;

			wxString ns = entry->parent()->detectNamespace(entry);
			if (ns == "patches")
			{
				if (bPatches)
					continue;
				bPatches = true;
				items_root_->addChild("Patches (Full Path)");
			}
			else if (ns == "flats")
			{
				if (bFlats)
					continue;
				bFlats = true;
				items_root_->addChild("Flats (Full Path)");
			}
			else if (ns == "sprites")
			{
				if (bSprites)
					continue;
				bSprites = true;
				items_root_->addChild("Sprites (Full Path)");
			}
			else if (ns == "textures")
			{
				if (bTextures)
					continue;
				bTextures = true;
				items_root_->addChild("Textures (Full Path)");
			}
			else
			{
				if (bGraphics)
					continue;
				bGraphics = true;
				items_root_->addChild("Graphics (Full Path)");
			}
			// This just adds the lists, so that they can be populated later.
			if (bPatches && bGraphics && bTextures && bFlats && bSprites)
				break;
		}
	}

	vector<wxString> usednames;

	// Go through the list
	for (auto entry : patches)
	{
		// Skip any without parent archives (shouldn't happen)
		if (!entry->parent())
			continue;

		// Check entry namespace
		wxString ns = entry->parent()->detectNamespace(entry);
		wxString nspace;
		if (ns == "patches")
			nspace = "Patches";
		else if (ns == "flats")
			nspace = "Flats";
		else if (ns == "sprites")
			nspace = "Sprites";
		else if (ns == "textures")
			nspace = "Textures";
		else
			nspace = "Graphics";

		// Check entry parent archive
		wxString arch = "Unknown";
		if (entry->parent())
			arch = entry->parent()->filename(false);

		PatchBrowserItem* item;

		// Add it
		if (full_path_ && !entry->parent()->isTreeless())
		{
			item = new PatchBrowserItem(entry->path(true).substr(1), archive, PatchBrowserItem::Type::Patch, ns);
			wxString fnspace = nspace + " (Full Path)";
			addItem(item, fnspace + "/" + arch);
		}

		wxString name = strutil::truncate(entry->upperNameNoExt(), 8);

		bool duplicate = false;
		for (auto& usedname : usednames)
		{
			if (usedname.Cmp(name) == 0)
			{
				duplicate = true;
				break;
			}
		}

		if (duplicate)
			continue;

		item = new PatchBrowserItem(name, archive, PatchBrowserItem::Type::Patch, ns);
		addItem(item, nspace + "/" + arch);
		usednames.push_back(name);
	}

	// Get list of all available textures (that aren't in the given archive)
	vector<TextureResource::Texture*> textures;
	app::resources().putAllTextures(textures, nullptr, archive);

	// Go through the list
	for (auto res : textures)
	{
		if (full_path_ || res->tex.name().size() <= 8)
		{
			auto parent = res->parent.lock().get();
			if (!parent)
				continue;

			// Create browser item
			auto item = new PatchBrowserItem(res->tex.name(), parent, PatchBrowserItem::Type::CTexture);

			// Add to textures node (under parent archive name)
			addItem(item, "Textures/" + parent->filename(false));
		}
	}

	// Update tree control
	populateItemTree();

	// Open 'patches' node
	openTree(dynamic_cast<BrowserTreeNode*>(items_root_->child("Patches")), true, true);


	return true;
}

// -----------------------------------------------------------------------------
// Adds all textures in [texturex] to the browser, in the tree at
// 'Textures/[parent archive filename]'
// -----------------------------------------------------------------------------
bool PatchBrowser::openTextureXList(TextureXList* texturex, Archive* parent)
{
	// Check tx list was given
	if (!texturex)
		return false;

	// Go through all textures in the list
	for (unsigned a = 0; a < texturex->size(); a++)
	{
		// Create browser item
		auto item = new PatchBrowserItem(texturex->texture(a)->name(), parent, PatchBrowserItem::Type::CTexture);

		// Set archive name
		wxString arch = "Unknown";
		if (parent)
			arch = parent->filename(false);

		// Add to textures node (under parent archive name)
		addItem(item, "Textures/" + arch);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns the index of the currently selected patch, or -1 if none are selected
// -----------------------------------------------------------------------------
int PatchBrowser::selectedPatch()
{
	// Get selected item
	auto item = dynamic_cast<PatchBrowserItem*>(selectedItem());

	if (item)
		return item->index();
	else
		return -1;
}

// -----------------------------------------------------------------------------
// Selects the patch at [pt_index] in the patch table
// -----------------------------------------------------------------------------
void PatchBrowser::selectPatch(int pt_index)
{
	// Can't without a patch table
	if (!patch_table_)
		return;

	// Check index
	if (pt_index < 0 || pt_index >= (int)patch_table_->nPatches())
		return;

	// Select by patch name
	selectPatch(patch_table_->patchName(pt_index));
}

// -----------------------------------------------------------------------------
// Selects the patch matching [name]
// -----------------------------------------------------------------------------
void PatchBrowser::selectPatch(const wxString& name)
{
	selectItem(name);
}
