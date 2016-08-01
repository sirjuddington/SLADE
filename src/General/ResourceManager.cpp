
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ResourceManager.cpp
 * Description: The ResourceManager class. Manages all editing
 *              resources (patches, gfx, etc) in all open archives
 *              and the base resource.
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
#include "ResourceManager.h"
#include "Archive/ArchiveManager.h"
#include "General/Console/Console.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/TextureXList.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
ResourceManager* ResourceManager::instance = NULL;
string ResourceManager::Doom64HashTable[65536];


/*******************************************************************
 * ENTRYRESOURCE CLASS FUNCTIONS
 *******************************************************************/

/* EntryResource::EntryResource
 * EntryResource class constructor
 *******************************************************************/
EntryResource::EntryResource(ArchiveEntry* entry) : Resource("entry")
{
}

/* EntryResource::~EntryResource
 * EntryResource class destructor
 *******************************************************************/
EntryResource::~EntryResource()
{
}

/* EntryResource::add
 * Adds matching [entry] to the resource
 *******************************************************************/
void EntryResource::add(ArchiveEntry* entry)
{
	entries.push_back(entry);
}

/* EntryResource::remove
 * Removes matching [entry] from the resource
 *******************************************************************/
void EntryResource::remove(ArchiveEntry* entry)
{
	unsigned a = 0;
	while (a < entries.size())
	{
		if (entries[a] == entry)
			entries.erase(entries.begin() + a);
		else
			a++;
	}
}

/* EntryResource::length
 * Returns the number of entries matching this resource
 *******************************************************************/
int EntryResource::length()
{
	return entries.size();
}


/*******************************************************************
 * TEXTURERESOURCE CLASS FUNCTIONS
 *******************************************************************/

/* TextureResource::TextureResource
 * TextureResource class constructor
 *******************************************************************/
TextureResource::TextureResource() : Resource("texture")
{
}

/* TextureResource::~TextureResource
 * TextureResource class destructor
 *******************************************************************/
TextureResource::~TextureResource()
{
}

/* TextureResource::add
 * Adds a texture to this resource
 *******************************************************************/
void TextureResource::add(CTexture* tex, Archive* parent)
{
	// Check args
	if (!tex || !parent)
		return;

	// Create resource
	tex_res_t res;
	res.tex = new CTexture();
	res.tex->copyTexture(tex);
	res.parent = parent;

	// Add it
	textures.push_back(res);
}

/* TextureResource::remove
 * Removes any textures in this resource that are part of [parent]
 * archive
 *******************************************************************/
void TextureResource::remove(Archive* parent)
{
	// Remove any textures with matching parent
	unsigned a = 0;
	while (a < textures.size())
	{
		if (textures[a].parent == parent)
		{
			delete textures[a].tex;
			textures.erase(textures.begin() + a);
		}
		else
			a++;
	}
}

/* TextureResource::length
 * Returns the number of textures matching this resource
 *******************************************************************/
int TextureResource::length()
{
	return textures.size();
}


/*******************************************************************
 * RESOURCEMANAGER CLASS FUNCTIONS
 *******************************************************************/

/* ResourceManager::ResourceManager
 * ResourceManager class constructor
 *******************************************************************/
ResourceManager::ResourceManager()
{
}

/* ResourceManager::~ResourceManager
 * ResourceManager class destructor
 *******************************************************************/
ResourceManager::~ResourceManager()
{
}

/* ResourceManager::addArchive
 * Adds an archive to be managed
 *******************************************************************/
void ResourceManager::addArchive(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return;

	// Go through entries
	vector<ArchiveEntry*> entries;
	archive->getEntryTreeAsList(entries);
	for (unsigned a = 0; a < entries.size(); a++)
		addEntry(entries[a]);

	// Listen to the archive
	listenTo(archive);

	// Announce resource update
	announce("resources_updated");
}

/* ResourceManager::removeArchive
 * Removes a managed archive
 *******************************************************************/
void ResourceManager::removeArchive(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return;

	// Go through entries
	vector<ArchiveEntry*> entries;
	archive->getEntryTreeAsList(entries);
	for (unsigned a = 0; a < entries.size(); a++)
		removeEntry(entries[a]);

	// Announce resource update
	announce("resources_updated");
}

/* ResourceManager::getTextureHash
 * Returns the Doom64 hash of a given texture name, computed using
 * the same hash algorithm as Doom64 EX itself
 *******************************************************************/
uint16_t ResourceManager::getTextureHash(string name)
{
	char str[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	for (size_t c = 0; c < name.length() && c < 8; c++)
		str[c] = name[c];

	uint32_t hash = 1315423911;
	for(uint32_t i = 0; i < 8 && str[i] != '\0'; ++i)
		hash ^= ((hash << 5) + toupper((int)str[i]) + (hash >> 2));
	hash %= 65536;
	return (uint16_t)hash;
}

/* ResourceManager::addEntry
 * Adds an entry to be managed
 *******************************************************************/
void ResourceManager::addEntry(ArchiveEntry* entry)
{
	// Detect type if unknown
	if (entry->getType() == EntryType::unknownType())
		EntryType::detectEntryType(entry);

	// Get entry type
	EntryType* type = entry->getType();

	// Get resource name (extension cut, uppercase)
	string name = entry->getName(true).Upper();
	// Talon1024 - Get resource path (uppercase, without leading slash)
	string path = entry->getPath(true).Upper().Mid(1);

	// Check for palette entry
	if (type->getId() == "palette")
		palettes[name].add(entry);

	// Check for various image entries, so only accept images
	if (type->getEditor() == "gfx")
	{
		// Reject graphics that are not in a valid namespace:
		// Patches in wads can be in the global namespace as well, and
		// ZDoom textures can use sprites and graphics as patches
		if (!entry->isInNamespace("global")		&& !entry->isInNamespace("patches")		&&
		        !entry->isInNamespace("sprites")	&& !entry->isInNamespace("graphics")	&&
		        // Stand-alone textures can also be found in the hires namespace
		        !entry->isInNamespace("hires")		&& !entry->isInNamespace("textures")	&&
		        // Flats are kinda boring in comparison
		        !entry->isInNamespace("flats"))
			return;

		// Check for patch entry
		if (type->extraProps().propertyExists("patch") || entry->isInNamespace("patches"))
			patches[name].add(entry);

		// Check for flat entry
		if (type->getId() == "gfx_flat" || entry->isInNamespace("flats")) {
			flats[name].add(entry);
			flats[path].add(entry);
		}

		// Check for stand-alone texture entry
		if (entry->isInNamespace("textures") || entry->isInNamespace("hires"))
		{
			satextures[name].add(entry);
			satextures[path].add(entry);

			// Add name to hash table
			ResourceManager::Doom64HashTable[getTextureHash(name)] = name;

		}
	}

	// Check for TEXTUREx entry
	int txentry = 0;
	if (type->getId() == "texturex")
		txentry = 1;
	else if (type->getId() == "zdtextures")
		txentry = 2;
	if (txentry > 0)
	{
		// Load patch table if needed
		PatchTable ptable;
		if (txentry == 1)
		{
			Archive::search_options_t opt;
			opt.match_type = EntryType::getType("pnames");
			ArchiveEntry* pnames = entry->getParent()->findLast(opt);
			ptable.loadPNAMES(pnames, entry->getParent());
		}

		// Read texture list
		TextureXList tx;
		if (txentry == 1)
			tx.readTEXTUREXData(entry, ptable);
		else
			tx.readTEXTURESData(entry);

		// Add all textures to resources
		CTexture* tex = NULL;
		for (unsigned a = 0; a < tx.nTextures(); a++)
		{
			tex = tx.getTexture(a);
			textures[tex->getName()].add(tex, entry->getParent());
		}
	}
}

/* ResourceManager::removeEntry
 * Removes a managed entry
 *******************************************************************/
void ResourceManager::removeEntry(ArchiveEntry* entry)
{
	// Get resource name (extension cut, uppercase)
	string name = entry->getName(true).Upper();

	// Remove from palettes
	palettes[name].remove(entry);

	// Remove from patches
	patches[name].remove(entry);

	// Remove from flats
	flats[name].remove(entry);

	// Remove from stand-alone textures
	satextures[name].remove(entry);

	// Check for TEXTUREx entry
	int txentry = 0;
	if (entry->getType()->getId() == "texturex")
		txentry = 1;
	else if (entry->getType()->getId() == "zdtextures")
		txentry = 2;
	if (txentry > 0)
	{
		// Read texture list
		TextureXList tx;
		PatchTable ptable;
		if (txentry == 1)
			tx.readTEXTUREXData(entry, ptable);
		else
			tx.readTEXTURESData(entry);

		// Remove all texture resources
		for (unsigned a = 0; a < tx.nTextures(); a++)
			textures[tx.getTexture(a)->getName()].remove(entry->getParent());
	}
}

/* ResourceManager::listAllPatches
 * Dumps all patch names and the number of matching entries for each
 *******************************************************************/
void ResourceManager::listAllPatches()
{
	EntryResourceMap::iterator i = patches.begin();
	while (i != patches.end())
	{
		wxLogMessage("%s (%d)", i->first, i->second.length());
		i++;
	}
}

/* ResourceManager::getAllPatchEntries
 * Adds all current patch entries to [list]
 *******************************************************************/
void ResourceManager::getAllPatchEntries(vector<ArchiveEntry*>& list, Archive* priority)
{
	EntryResourceMap::iterator i = patches.begin();

	// Add all primary entries to the list
	while (i != patches.end())
	{
		// Skip if no entries
		if (i->second.length() == 0)
		{
			i++;
			continue;
		}

		// Go through resource entries
		ArchiveEntry* entry = i->second.entries[0];
		for (int a = 0; a < i->second.length(); a++)
		{
			entry = i->second.entries[a];

			// If it's in the 'priority' archive, exit loop
			if (priority && i->second.entries[a]->getParent() == priority)
				break;

			// Otherwise, if it's in a 'later' archive than the current resource entry, set it
			if (theArchiveManager->archiveIndex(entry->getParent()) <=
			        theArchiveManager->archiveIndex(i->second.entries[a]->getParent()))
				entry = i->second.entries[a];
		}

		// Add entry to the list
		list.push_back(entry);

		i++;
	}
}

/* ResourceManager::getAllTextures
 * Adds all current textures to [list]
 *******************************************************************/
void ResourceManager::getAllTextures(vector<TextureResource::tex_res_t>& list, Archive* priority, Archive* ignore)
{
	TextureResourceMap::iterator i = textures.begin();

	// Add all primary textures to the list
	while (i != textures.end())
	{
		// Skip if no entries
		if (i->second.length() == 0)
		{
			i++;
			continue;
		}

		// Go through resource textures
		TextureResource::tex_res_t res = i->second.textures[0];
		for (int a = 0; a < i->second.length(); a++)
		{
			res = i->second.textures[a];

			// Skip if it's in the 'ignore' archive
			if (res.parent == ignore)
				continue;

			// If it's in the 'priority' archive, exit loop
			if (priority && i->second.textures[a].parent == priority)
				break;

			// Otherwise, if it's in a 'later' archive than the current resource, set it
			if (theArchiveManager->archiveIndex(res.parent) <=
			        theArchiveManager->archiveIndex(i->second.textures[a].parent))
				res = i->second.textures[a];
		}

		// Add texture resource to the list
		if (res.parent != ignore)
			list.push_back(res);

		i++;
	}
}

/* ResourceManager::getAllTextureNames
 * Adds all current texture names to [list]
 *******************************************************************/
void ResourceManager::getAllTextureNames(vector<string>& list)
{
	TextureResourceMap::iterator i = textures.begin();

	// Add all primary textures to the list
	while (i != textures.end())
	{
		// Skip if no entries
		if (i->second.length() == 0)
		{
			i++;
			continue;
		}

		list.push_back(i->first);
		i++;
	}
}

/* ResourceManager::getAllFlatEntries
 * Adds all current flat entries to [list]
 *******************************************************************/
void ResourceManager::getAllFlatEntries(vector<ArchiveEntry*>& list, Archive* priority)
{
	EntryResourceMap::iterator i = flats.begin();

	// Add all primary entries to the list
	while (i != flats.end())
	{
		// Skip if no entries
		if (i->second.length() == 0)
		{
			i++;
			continue;
		}

		// Go through resource entries
		ArchiveEntry* entry = i->second.entries[0];
		for (int a = 0; a < i->second.length(); a++)
		{
			entry = i->second.entries[a];

			// If it's in the 'priority' archive, exit loop
			if (priority && i->second.entries[a]->getParent() == priority)
				break;

			// Otherwise, if it's in a 'later' archive than the current resource entry, set it
			if (theArchiveManager->archiveIndex(entry->getParent()) <=
			        theArchiveManager->archiveIndex(i->second.entries[a]->getParent()))
				entry = i->second.entries[a];
		}

		// Add entry to the list
		list.push_back(entry);

		i++;
	}
}

/* ResourceManager::getAllFlatNames
 * Adds all current flat names to [list]
 *******************************************************************/
void ResourceManager::getAllFlatNames(vector<string>& list)
{
	EntryResourceMap::iterator i = flats.begin();

	// Add all primary textures to the list
	while (i != flats.end())
	{
		// Skip if no entries
		if (i->second.length() == 0)
		{
			i++;
			continue;
		}

		list.push_back(i->first);
		i++;
	}
}

/* ResourceManager::getPaletteEntry
 * Returns the most appropriate managed resource entry for [palette],
 * or NULL if no match found
 *******************************************************************/
ArchiveEntry* ResourceManager::getPaletteEntry(string palette, Archive* priority)
{
	// Check resource with matching name exists
	EntryResource& res = palettes[palette.Upper()];
	if (res.entries.size() == 0)
		return NULL;

	// Go through resource entries
	ArchiveEntry* entry = res.entries[0];
	for (unsigned a = 0; a < res.entries.size(); a++)
	{
		// If it's in the 'priority' archive, return it
		if (priority && (res.entries[a]->getParent() == priority ||
		                 // PK3 and Doom64 maps are contained in an embedded .wad,
		                 // so for them the real priority archive is their parent
		                 // archive's own parent archive.
		                 res.entries[a]->getParent() == priority->getParentArchive()))
			return res.entries[a];

		// Otherwise, if it's in a 'later' archive than the current resource entry, set it
		if (theArchiveManager->archiveIndex(entry->getParent()) <=
		        theArchiveManager->archiveIndex(res.entries[a]->getParent()))
			entry = res.entries[a];
	}

	// Return most relevant entry
	return entry;
}

/* ResourceManager::getPatchEntry
 * Returns the most appropriate managed resource entry for [patch],
 * or NULL if no match found
 *******************************************************************/
ArchiveEntry* ResourceManager::getPatchEntry(string patch, string nspace, Archive* priority)
{
	// Are we wanting to use a flat as a patch?
	if (!nspace.CmpNoCase("flats"))
		return getFlatEntry(patch, priority);

	// Are we wanting to use a stand-alone texture as a patch?
	if (!nspace.CmpNoCase("textures"))
		return getTextureEntry(patch, "textures", priority);

	// Check resource with matching name exists
	EntryResource& res = patches[patch.Upper()];
	if (res.entries.size() == 0)
		return NULL;

	// Go through resource entries
	ArchiveEntry* entry = res.entries[0];
	for (unsigned a = 0; a < res.entries.size(); a++)
	{
		// If the entry is in the correct namespace (if namespace is important)
		if (nspace.IsEmpty() || res.entries[a]->isInNamespace(nspace))
		{
			// If it's in the 'priority' archive, return it
			if (priority && (res.entries[a]->getParent() == priority ||
			                 // PK3 and Doom64 maps are contained in an embedded .wad,
			                 // so for them the real priority archive is their parent
			                 // archive's own parent archive.
			                 res.entries[a]->getParent() == priority->getParentArchive()))
				return res.entries[a];

			// Regardless of priority, if the first entry is not in the chosen namespace but
			// the current entry is, then set it so that we'll be able to return something valid
			if (!nspace.IsEmpty() && !entry->isInNamespace(nspace) && res.entries[a]->isInNamespace(nspace))
				entry = res.entries[a];

			// Otherwise, if it's in a 'later' archive than the current resource entry, set it
			if (theArchiveManager->archiveIndex(entry->getParent()) <=
			        theArchiveManager->archiveIndex(res.entries[a]->getParent()))
				entry = res.entries[a];
		}
	}

	// Return most relevant entry
	return entry;
}

/* ResourceManager::getFlatEntry
 * Returns the most appropriate managed resource entry for [flat],
 * or NULL if no match found
 *******************************************************************/
ArchiveEntry* ResourceManager::getFlatEntry(string flat, Archive* priority)
{
	// Check resource with matching name exists
	EntryResource& res = flats[flat.Upper()];
	if (res.entries.size() == 0)
		return NULL;

	// Go through resource entries
	ArchiveEntry* entry = res.entries[0];
	for (unsigned a = 0; a < res.entries.size(); a++)
	{
		// If it's in the 'priority' archive, return it
		if (priority && (res.entries[a]->getParent() == priority ||
		                 // PK3 and Doom64 maps are contained in an embedded .wad,
		                 // so for them the real priority archive is their parent
		                 // archive's own parent archive.
		                 res.entries[a]->getParent() == priority->getParentArchive()))
			return res.entries[a];

		// Otherwise, if it's in a 'later' archive than the current resource entry, set it
		if (theArchiveManager->archiveIndex(entry->getParent()) <=
		        theArchiveManager->archiveIndex(res.entries[a]->getParent()))
			entry = res.entries[a];
	}

	// Return most relevant entry
	return entry;
}

/* ResourceManager::getTextureEntry
 * Returns the most appropriate managed resource entry for [texture],
 * or NULL if no match found
 *******************************************************************/
ArchiveEntry* ResourceManager::getTextureEntry(string texture, string nspace, Archive* priority)
{
	//wxLogMessage("getTextureEntry called - texture: %s, nspace: %s, priority: %s", texture, nspace, priority->getFilename());
	// Check resource with matching name exists
	EntryResource& res = satextures[texture.Upper()];
	if (res.entries.size() == 0)
		return NULL;

	// Go through resource entries
	ArchiveEntry* entry = NULL;
	for (unsigned a = 0; a < res.entries.size(); a++)
	{
		// If the entry is in the correct namespace (if namespace is important)
		// namespace ought to be either "textures" or "hires"
		if (nspace.IsEmpty() || res.entries[a]->isInNamespace(nspace))
		{
			// If it's in the 'priority' archive, return it
			if (priority && (res.entries[a]->getParent() == priority ||
			                 // PK3 and Doom64 maps are contained in an embedded .wad,
			                 // so for them the real priority archive is their parent
			                 // archive's own parent archive.
			                 res.entries[a]->getParent() == priority->getParentArchive()))
				return res.entries[a];

			// Otherwise, if it's in a 'later' archive than the current resource entry, set it
			if (!entry || theArchiveManager->archiveIndex(entry->getParent()) <=
			        theArchiveManager->archiveIndex(res.entries[a]->getParent()))
				entry = res.entries[a];
		}
	}

	// Return most relevant entry
	return entry;
}

/* ResourceManager::getTexture
 * Returns the most appropriate managed texture for [texture], or
 * NULL if no match found
 *******************************************************************/
CTexture* ResourceManager::getTexture(string texture, Archive* priority, Archive* ignore)
{
	// Check texture resource with matching name exists
	TextureResource& res = textures[texture.Upper()];
	if (res.textures.size() == 0)
		return NULL;

	// Go through resource textures
	CTexture* tex = res.textures[0].tex;
	Archive* parent = res.textures[0].parent;
	for (unsigned a = 0; a < res.textures.size(); a++)
	{
		// Skip if it's in the 'ignore' archive
		if (res.textures[a].parent == ignore)
			continue;

		// If it's in the 'priority' archive, return it
		if (priority && res.textures[a].parent == priority)
			return res.textures[a].tex;

		// Otherwise, if it's in a 'later' archive than the current resource entry, set it
		if (theArchiveManager->archiveIndex(parent) <=
		        theArchiveManager->archiveIndex(res.textures[a].parent))
		{
			tex = res.textures[a].tex;
			parent = res.textures[a].parent;
		}
	}

	// Return the most relevant texture
	if (parent != ignore)
		return tex;
	else
		return NULL;
}

/* ResourceManager::onAnnouncement
 * Called when an announcement is recieved from any managed archive
 *******************************************************************/
void ResourceManager::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	event_data.seek(0, SEEK_SET);

	// An entry is modified
	if (event_name == "entry_state_changed")
	{
		wxUIntPtr ptr;
		event_data.read(&ptr, sizeof(wxUIntPtr), 4);
		ArchiveEntry* entry = (ArchiveEntry*)wxUIntToPtr(ptr);
		removeEntry(entry);
		addEntry(entry);
		announce("resources_updated");
	}

	// An entry is removed or renamed
	if (event_name == "entry_removing" || event_name == "entry_renaming")
	{
		wxUIntPtr ptr;
		event_data.read(&ptr, sizeof(wxUIntPtr), sizeof(int));
		ArchiveEntry* entry = (ArchiveEntry*)wxUIntToPtr(ptr);
		removeEntry(entry);
		announce("resources_updated");
	}

	// An entry is added
	if (event_name == "entry_added")
	{
		wxUIntPtr ptr;
		event_data.read(&ptr, sizeof(wxUIntPtr), 4);
		ArchiveEntry* entry = (ArchiveEntry*)wxUIntToPtr(ptr);
		addEntry(entry);
		announce("resources_updated");
	}
}





CONSOLE_COMMAND(list_res_patches, 0, false)
{
	theResourceManager->listAllPatches();
}
