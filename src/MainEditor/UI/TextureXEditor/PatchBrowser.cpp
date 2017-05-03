
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PatchBrowser.cpp
 * Description: A specialisation of the Browser class for browsing
 *              the contents of a patch table. Splits the patches
 *              into three categories - Base, Archive and Unknown
 *              for patches existing in the base resource, the
 *              current archive, and entries not found, respectively
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
#include "PatchBrowser.h"
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
#include "General/ResourceManager.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "UI/PaletteChooser.h"


/*******************************************************************
 * PATCHBROWSERITEM CLASS FUNCTIONS
 *******************************************************************/

/* PatchBrowserItem::PatchBrowserItem
 * PatchBrowserItem class constructor
 *******************************************************************/
PatchBrowserItem::PatchBrowserItem(string name, Archive* archive, uint8_t type, string nspace, unsigned index) : BrowserItem(name, index, "patch")
{
	// Init variables
	this->archive = archive;
	this->type = type;
	this->nspace = nspace;
}

/* PatchBrowserItem::~PatchBrowserItem
 * PatchBrowserItem class destructor
 *******************************************************************/
PatchBrowserItem::~PatchBrowserItem()
{
	if (image) delete image;
}

/* PatchBrowserItem::loadImage
 * Loads the item's image from its associated entry (if any)
 *******************************************************************/
bool PatchBrowserItem::loadImage()
{
	SImage img;

	// Load patch image
	if (type == 0)
	{
		// Find patch entry
		ArchiveEntry* entry = theResourceManager->getPatchEntry(name, nspace, archive);

		// Load entry to image, if it exists
		if (entry)
			Misc::loadImageFromEntry(&img, entry);
		else
			return false;
	}

	// Or, load texture image
	if (type == 1)
	{
		// Find texture
		CTexture* tex = theResourceManager->getTexture(name, archive);

		// Load texture to image, if it exists
		if (tex)
			tex->toImage(img, archive, parent->getPalette());
		else
			return false;
	}

	// Create gl texture from image
	if (image) delete image;
	image = new GLTexture();
	return image->loadImage(&img, parent->getPalette());
}

/* PatchBrowserItem::itemInfo
 * Returns a string with extra information about the patch
 *******************************************************************/
string PatchBrowserItem::itemInfo()
{
	string info;

	// Add dimensions if known
	if (image)
		info += S_FMT("%dx%d", image->getWidth(), image->getHeight());
	else
		info += "Unknown size";

	// Add patch type
	if (type == 0)
		info += ", Patch";
	else
		info += ", Texture";

	// Add namespace if it exists
	if (!nspace.IsEmpty())
	{
		info += ", ";
		info += nspace.Capitalize();
		info += " namespace";
	}

	return info;
}


/*******************************************************************
 * PATCHBROWSER CLASS FUNCTIONS
 *******************************************************************/

/* PatchBrowser::PatchBrowser
 * PatchBrowser class constructor
 *******************************************************************/
PatchBrowser::PatchBrowser(wxWindow* parent) : BrowserWindow(parent)
{
	// Init variables
	this->patch_table = NULL;

	// Init browser tree
	items_root->addChild("IWAD");
	items_root->addChild("Custom");
	items_root->addChild("Unknown");

	// Init palette chooser
	listenTo(theMainWindow->getPaletteChooser());

	// Set dialog title
	SetTitle("Browse Patches");
}

/* PatchBrowser::~PatchBrowser
 * PatchBrowser class destructor
 *******************************************************************/
PatchBrowser::~PatchBrowser()
{
}

/* PatchBrowser::openPatchTable
 * Opens contents of the patch table [table] for browsing
 *******************************************************************/
bool PatchBrowser::openPatchTable(PatchTable* table)
{
	// Check table was given
	if (!table)
		return false;

	// Clear any existing browser items
	clearItems();

	// Setup palette chooser
	theMainWindow->getPaletteChooser()->setGlobalFromArchive(table->getParent());

	// Go through patch table
	for (unsigned a = 0; a < table->nPatches(); a++)
	{
		// Get patch
		patch_t& patch = table->patch(a);

		// Init position to add
		string whereis = "Unknown";

		// Get patch entry
		ArchiveEntry* entry = theResourceManager->getPatchEntry(patch.name);

		// Check its parent archive
		Archive* parent_archive = NULL;
		if (entry)
		{
			parent_archive = entry->getParent();

			if (parent_archive)
				whereis = parent_archive->getFilename(false);
		}

		// Add it
		PatchBrowserItem* item = new PatchBrowserItem(patch.name, parent_archive, 0, "", a);
		addItem(item, whereis);
	}

	// Update variables
	if (patch_table) stopListening(patch_table);
	patch_table = table;
	listenTo(table);

	// Open 'all' node
	openTree(items_root);

	// Update tree control
	populateItemTree();

	return true;
}

/* PatchBrowser::openArchive
 * Opens all loaded resource patches and textures, prioritising those
 * from [archive], except for the case of composite textures, which
 * are ignored if in [archive]
 *******************************************************************/
bool PatchBrowser::openArchive(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return false;

	// Clear any existing browser items
	clearItems();

	// Init browser tree
	items_root->addChild("Patches");
	items_root->addChild("Graphics");
	items_root->addChild("Textures");
	items_root->addChild("Flats");
	items_root->addChild("Sprites");

	// Setup palette chooser
	theMainWindow->getPaletteChooser()->setGlobalFromArchive(archive);

	// Get a list of all available patch entries
	vector<ArchiveEntry*> patches;
	theResourceManager->getAllPatchEntries(patches, archive);

	// Add flats, too
	theResourceManager->getAllFlatEntries(patches, archive);

	// Go through the list
	for (unsigned a = 0; a < patches.size(); a++)
	{
		ArchiveEntry* entry = patches[a];

		// Skip any without parent archives (shouldn't happen)
		if (!entry->getParent())
			continue;

		// Check entry namespace
		string ns = entry->getParent()->detectNamespace(entry);
		string nspace;
		if (ns == "patches") nspace = "Patches";
		else if (ns == "flats") nspace = "Flats";
		else if (ns == "sprites") nspace = "Sprites";
		else if (ns == "textures") nspace = "Textures";
		else nspace = "Graphics";

		// Check entry parent archive
		string arch = "Unknown";
		if (entry->getParent())
			arch = entry->getParent()->getFilename(false);

		// Add it
		PatchBrowserItem* item = new PatchBrowserItem(entry->getName(true).Truncate(8).Upper(), archive, 0, ns);
		addItem(item, nspace + "/" + arch);
	}

	// Get list of all available textures (that aren't in the given archive)
	vector<TextureResource::Texture*> textures;
	theResourceManager->getAllTextures(textures, NULL, archive);

	// Go through the list
	for (unsigned a = 0; a < textures.size(); a++)
	{
		TextureResource::Texture* res = textures[a];

		// Create browser item
		PatchBrowserItem* item = new PatchBrowserItem(res->tex.getName(), res->parent, 1);

		// Add to textures node (under parent archive name)
		addItem(item, "Textures/" + res->parent->getFilename(false));
	}

	// Open 'patches' node
	openTree((BrowserTreeNode*)items_root->getChild("Patches"));

	// Update tree control
	populateItemTree();

	return true;
}

/* PatchBrowser::openTextureXList
 * Adds all textures in [texturex] to the browser, in the tree at
 * 'Textures/[parent archive filename]'
 *******************************************************************/
bool PatchBrowser::openTextureXList(TextureXList* texturex, Archive* parent)
{
	// Check tx list was given
	if (!texturex)
		return false;

	// Go through all textures in the list
	for (unsigned a = 0; a < texturex->nTextures(); a++)
	{
		// Create browser item
		PatchBrowserItem* item = new PatchBrowserItem(texturex->getTexture(a)->getName(), parent, 1);

		// Set archive name
		string arch = "Unknown";
		if (parent)
			arch = parent->getFilename(false);

		// Add to textures node (under parent archive name)
		addItem(item, "Textures/" + arch);
	}

	return true;
}

/* PatchBrowser::getSelectedPatch
 * Returns the index of the currently selected patch, or -1 if none
 * are selected
 *******************************************************************/
int PatchBrowser::getSelectedPatch()
{
	// Get selected item
	PatchBrowserItem* item = (PatchBrowserItem*)getSelectedItem();

	if (item)
		return item->getIndex();
	else
		return -1;
}

/* PatchBrowser::selectPatch
 * Selects the patch at [pt_index] in the patch table
 *******************************************************************/
void PatchBrowser::selectPatch(int pt_index)
{
	// Can't without a patch table
	if (!patch_table)
		return;

	// Check index
	if (pt_index < 0 || pt_index >= (int)patch_table->nPatches())
		return;

	// Select by patch name
	selectPatch(patch_table->patchName(pt_index));
}

/* PatchBrowser::selectPatch
 * Selects the patch matching [name]
 *******************************************************************/
void PatchBrowser::selectPatch(string name)
{
	selectItem(name);
}

/* PatchBrowser::onAnnouncement
 * Handles any announcements
 *******************************************************************/
void PatchBrowser::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	if (announcer != theMainWindow->getPaletteChooser())
		return;

	if (event_name == "main_palette_changed")
	{
		// Update palette
		palette.copyPalette(theMainWindow->getPaletteChooser()->getSelectedPalette());

		// Reload all items
		reloadItems();
		Refresh();
	}
}
