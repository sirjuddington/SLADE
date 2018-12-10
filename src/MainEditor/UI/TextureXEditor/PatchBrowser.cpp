
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
#include "General/ResourceManager.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "UI/Controls/PaletteChooser.h"


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
	// TODO: Why isn't this done in the BrowserItem destructor?
	if (image_)
		delete image_;
}

// -----------------------------------------------------------------------------
// Loads the item's image from its associated entry (if any)
// -----------------------------------------------------------------------------
bool PatchBrowserItem::loadImage()
{
	SImage img;

	// Load patch image
	if (type_ == 0)
	{
		// Find patch entry
		ArchiveEntry* entry = theResourceManager->getPatchEntry(name_, nspace_, archive_);

		// Load entry to image, if it exists
		if (entry)
			Misc::loadImageFromEntry(&img, entry);
		else
			return false;
	}

	// Or, load texture image
	if (type_ == 1)
	{
		// Find texture
		CTexture* tex = theResourceManager->getTexture(name_, archive_);

		// Load texture to image, if it exists
		if (tex)
			tex->toImage(img, archive_, parent_->palette());
		else
			return false;
	}

	// Create gl texture from image
	if (image_)
		delete image_;
	image_ = new GLTexture();
	return image_->loadImage(&img, parent_->palette());
}

// -----------------------------------------------------------------------------
// Returns a string with extra information about the patch
// -----------------------------------------------------------------------------
string PatchBrowserItem::itemInfo()
{
	string info;

	// Add dimensions if known
	if (image_)
		info += S_FMT("%dx%d", image_->width(), image_->height());
	else
		info += "Unknown size";

	// Add patch type
	if (type_ == 0)
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
//
// PatchBrowser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PatchBrowser class constructor
// -----------------------------------------------------------------------------
PatchBrowser::PatchBrowser(wxWindow* parent) : BrowserWindow(parent), full_path_(false)
{
	// Init variables
	this->patch_table_ = nullptr;

	// Init browser tree
	items_root_->addChild("IWAD");
	items_root_->addChild("Custom");
	items_root_->addChild("Unknown");

	// Init palette chooser
	listenTo(theMainWindow->paletteChooser());

	// Set dialog title
	SetTitle("Browse Patches");
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
		string whereis = "Unknown";

		// Get patch entry
		ArchiveEntry* entry = theResourceManager->getPatchEntry(patch.name);

		// Check its parent archive
		Archive* parent_archive = nullptr;
		if (entry)
		{
			parent_archive = entry->parent();

			if (parent_archive)
				whereis = parent_archive->filename(false);
		}

		// Add it
		PatchBrowserItem* item = new PatchBrowserItem(patch.name, parent_archive, 0, "", a);
		addItem(item, whereis);
	}

	// Update variables
	if (patch_table_)
		stopListening(patch_table_);
	patch_table_ = table;
	listenTo(table);

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
	this->truncate_names_ = full_path_;
	// Check archive was given
	if (!archive)
		return false;

	// Clear any existing browser items
	clearItems();

	// Init browser tree
	items_root_->addChild("Patches");
	items_root_->addChild("Graphics");
	items_root_->addChild("Textures");
	items_root_->addChild("Flats");
	items_root_->addChild("Sprites");

	// Setup palette chooser
	theMainWindow->paletteChooser()->setGlobalFromArchive(archive);

	// Get a list of all available patch entries
	vector<ArchiveEntry*> patches;
	theResourceManager->putAllPatchEntries(patches, archive, full_path_);

	// Add flats, too
	{
		vector<ArchiveEntry*> flats;
		theResourceManager->putAllFlatEntries(flats, archive, full_path_);
		for (unsigned a = 0; a < flats.size(); a++)
		{
			if (flats[a]->isInNamespace("flats") && flats[a]->parent()->isTreeless())
			{
				patches.push_back(flats[a]);
			}
		}
		flats.clear();
	}

	// Determine whether one or more patches exists in a treeful archive
	if (full_path_)
	{
		bool bPatches = false, bGraphics = false, bTextures = false, bFlats = false, bSprites = false;
		for (unsigned a = 0; a < patches.size(); a++)
		{
			if (patches[a]->parent()->isTreeless())
				continue;

			string ns = patches[a]->parent()->detectNamespace(patches[a]);
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

	vector<string> usednames;

	// Go through the list
	for (unsigned a = 0; a < patches.size(); a++)
	{
		ArchiveEntry* entry = patches[a];

		// Skip any without parent archives (shouldn't happen)
		if (!entry->parent())
			continue;

		// Check entry namespace
		string ns = entry->parent()->detectNamespace(entry);
		string nspace;
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
		string arch = "Unknown";
		if (entry->parent())
			arch = entry->parent()->filename(false);

		PatchBrowserItem* item;

		// Add it
		if (full_path_ && !entry->parent()->isTreeless())
		{
			item           = new PatchBrowserItem(entry->path(true).Mid(1), archive, 0, ns);
			string fnspace = nspace + " (Full Path)";
			addItem(item, fnspace + "/" + arch);
		}

		string name = entry->name(true).Truncate(8).Upper();

		bool duplicate = false;
		for (auto ustr = usednames.begin(); ustr != usednames.end(); ustr++)
		{
			if (ustr->Cmp(name) == 0)
			{
				duplicate = true;
				break;
			}
		}

		if (duplicate)
			continue;

		item = new PatchBrowserItem(name, archive, 0, ns);
		addItem(item, nspace + "/" + arch);
		usednames.push_back(name);
	}

	// Get list of all available textures (that aren't in the given archive)
	vector<TextureResource::Texture*> textures;
	theResourceManager->putAllTextures(textures, nullptr, archive);

	// Go through the list
	for (unsigned a = 0; a < textures.size(); a++)
	{
		TextureResource::Texture* res = textures[a];

		if (full_path_ || res->tex.name().Len() <= 8)
		{
			// Create browser item
			PatchBrowserItem* item = new PatchBrowserItem(res->tex.name(), res->parent, 1);

			// Add to textures node (under parent archive name)
			addItem(item, "Textures/" + res->parent->filename(false));
		}
	}

	// Open 'patches' node
	openTree((BrowserTreeNode*)items_root_->child("Patches"));

	// Update tree control
	populateItemTree();

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
	for (unsigned a = 0; a < texturex->nTextures(); a++)
	{
		// Create browser item
		PatchBrowserItem* item = new PatchBrowserItem(texturex->texture(a)->name(), parent, 1);

		// Set archive name
		string arch = "Unknown";
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
	PatchBrowserItem* item = (PatchBrowserItem*)selectedItem();

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
void PatchBrowser::selectPatch(string name)
{
	selectItem(name);
}

// -----------------------------------------------------------------------------
// Handles any announcements
// -----------------------------------------------------------------------------
void PatchBrowser::onAnnouncement(Announcer* announcer, const string& event_name, MemChunk& event_data)
{
	if (announcer != theMainWindow->paletteChooser())
		return;

	if (event_name == "main_palette_changed")
	{
		// Update palette
		palette_.copyPalette(theMainWindow->paletteChooser()->selectedPalette());

		// Reload all items
		reloadItems();
		Refresh();
	}
}
