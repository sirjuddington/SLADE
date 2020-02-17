
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ResourceManager.cpp
// Description: The ResourceManager class. Manages all editing resources
//              (patches, gfx, etc) in all open archives and the base resource
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
#include "ResourceManager.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/Console/Console.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
string ResourceManager::doom64_hash_table_[65536];


// -----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------
namespace
{
// ----------------------------------------------------------------------------
// Removes all entries in resource [map] that are within [archive]
// ----------------------------------------------------------------------------
void removeArchiveFromMap(EntryResourceMap& map, Archive* archive)
{
	for (auto& i : map)
		i.second.removeArchive(archive);
}

// ----------------------------------------------------------------------------
// Removes [entry] from resource [map].
// If [full_check] is true, all resources in the map are checked for the entry,
// otherwise only the resource [name] is checked
// ----------------------------------------------------------------------------
void removeEntryFromMap(EntryResourceMap& map, const string& name, shared_ptr<ArchiveEntry>& entry, bool full_check)
{
	if (full_check)
		for (auto& i : map)
			i.second.remove(entry);
	else
		map[name].remove(entry);
}
} // namespace


// ----------------------------------------------------------------------------
//
// EntryResource Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Adds matching [entry] to the resource
// -----------------------------------------------------------------------------
void EntryResource::add(shared_ptr<ArchiveEntry>& entry)
{
	if (entry->parent())
		entries_.push_back(entry);
}

// -----------------------------------------------------------------------------
// Removes matching [entry] from the resource
// -----------------------------------------------------------------------------
void EntryResource::remove(shared_ptr<ArchiveEntry>& entry)
{
	unsigned a = 0;
	while (a < entries_.size())
	{
		if (entries_[a].lock() == entry)
			entries_.erase(entries_.begin() + a);
		else
			++a;
	}
}

// ----------------------------------------------------------------------------
// Removes any entries in the resource that are in [archive]
// ----------------------------------------------------------------------------
void EntryResource::removeArchive(Archive* archive)
{
	unsigned a = 0;
	while (a < entries_.size())
	{
		if (entries_[a].expired())
			entries_.erase(entries_.begin() + a);
		else if (entries_[a].lock()->parent() == archive)
			entries_.erase(entries_.begin() + a);
		else
			++a;
	}
}

// ----------------------------------------------------------------------------
// Gets the most relevant entry for this resource, depending on [priority] and
// [nspace]. If [priority] is set, this will prioritize entries from the
// priority archive. If [nspace] is not empty, this will prioritize entries
// within that namespace, or if [ns_required] is true, ignore anything not in
// [nspace]
// -----------------------------------------------------------------------------
ArchiveEntry* EntryResource::getEntry(Archive* priority, string_view nspace, bool ns_required)
{
	// Check resoure has any entries
	if (entries_.empty())
		return nullptr;

	shared_ptr<ArchiveEntry> best;
	auto                     i = entries_.end();
	while (i != entries_.begin())
	{
		--i;
		// Check if expired
		if (i->expired())
		{
			i = entries_.erase(i);
			continue;
		}

		auto entry = i->lock();

		if (!best)
			best = entry;

		// Check namespace if required
		if (ns_required && !nspace.empty())
			if (!entry->isInNamespace(nspace))
				continue;

		// Check if in priority archive (or its parent)
		if (priority && (entry->parent() == priority || entry->parent()->parentArchive() == priority))
		{
			best = entry;
			break;
		}

		// Check namespace
		if (!ns_required && !nspace.empty() && !best->isInNamespace(nspace) && entry->isInNamespace(nspace))
		{
			best = entry;
			continue;
		}

		// Otherwise, if it's in a 'later' archive than the current resource entry, set it
		if (App::archiveManager().archiveIndex(best->parent()) <= App::archiveManager().archiveIndex(entry->parent()))
			best = entry;
	}

	return best.get();
}


// -----------------------------------------------------------------------------
//
// TextureResource Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Adds a texture to this resource
// -----------------------------------------------------------------------------
void TextureResource::add(CTexture* tex, Archive* parent)
{
	// Check args
	auto parent_shared = App::archiveManager().shareArchive(parent);
	if (!tex || !parent_shared)
		return;

	textures_.push_back(std::make_unique<Texture>(*tex, parent_shared));
}

// -----------------------------------------------------------------------------
// Removes any textures in this resource that are part of [parent] archive
// -----------------------------------------------------------------------------
void TextureResource::remove(Archive* parent)
{
	// Remove any textures with matching parent
	auto i = textures_.begin();
	while (i != textures_.end())
	{
		auto t_parent = i->get()->parent.lock().get();
		if (!t_parent || t_parent == parent)
			i = textures_.erase(i);
		else
			++i;
	}
}


// -----------------------------------------------------------------------------
//
// ResourceManager Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Adds an archive to be managed
// -----------------------------------------------------------------------------
void ResourceManager::addArchive(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return;

	// Go through entries
	vector<shared_ptr<ArchiveEntry>> entries;
	archive->putEntryTreeAsList(entries);
	for (auto& entry : entries)
		addEntry(entry);

	// Update entries from the archive when changed (added/removed/modified)
	archive->signals().entry_added.connect([this](Archive&, ArchiveEntry& e) { updateEntry(e, false, true); });
	archive->signals().entry_removed.connect([this](Archive&, ArchiveEntry& e) { updateEntry(e, true, false); });
	archive->signals().entry_state_changed.connect([this](Archive&, ArchiveEntry& e) { updateEntry(e, true, true); });

	// Update entries from the archive when renamed
	archive->signals().entry_renamed.connect([this](Archive&, ArchiveEntry& entry, string_view prev_name) {
		auto prev_upper   = StrUtil::upper(prev_name);
		auto entry_shared = entry.getShared();
		removeEntry(entry_shared, prev_upper);
		addEntry(entry_shared);

		signals_.resources_updated();
	});

	// Announce resource update
	signals_.resources_updated();
}

// -----------------------------------------------------------------------------
// Removes a managed archive
// -----------------------------------------------------------------------------
void ResourceManager::removeArchive(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return;

	// Remove from palettes
	removeArchiveFromMap(palettes_, archive);

	// Remove from patches
	removeArchiveFromMap(patches_, archive);
	removeArchiveFromMap(patches_fp_, archive);
	removeArchiveFromMap(patches_fp_only_, archive);

	// Remove from flats
	removeArchiveFromMap(flats_, archive);
	removeArchiveFromMap(flats_fp_, archive);
	removeArchiveFromMap(flats_fp_only_, archive);

	// Remove from stand-alone textures
	removeArchiveFromMap(satextures_, archive);
	removeArchiveFromMap(satextures_fp_, archive);

	// Remove any textures in the archive
	for (auto& i : textures_)
		i.second.remove(archive);

	// Announce resource update
	signals_.resources_updated();
}

// -----------------------------------------------------------------------------
// Returns the Doom64 hash of a given texture name, computed using the same
// hash algorithm as Doom64 EX itself
// -----------------------------------------------------------------------------
uint16_t ResourceManager::getTextureHash(string_view name) const
{
	char str[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	for (size_t c = 0; c < name.length() && c < 8; c++)
		str[c] = name[c];

	uint32_t hash = 1315423911;
	for (uint32_t i = 0; i < 8 && str[i] != '\0'; ++i)
		hash ^= ((hash << 5) + toupper((int)str[i]) + (hash >> 2));
	hash %= 65536;
	return (uint16_t)hash;
}

// -----------------------------------------------------------------------------
// Adds an entry to be managed
// -----------------------------------------------------------------------------
void ResourceManager::addEntry(shared_ptr<ArchiveEntry>& entry)
{
	if (!entry.get())
		return;

	// Detect type if unknown
	if (entry->type() == EntryType::unknownType())
		EntryType::detectEntryType(*entry);

	// Get entry type
	auto type = entry->type();

	// Get resource name (extension cut, uppercase)
	auto lname = entry->upperNameNoExt();
	auto name  = StrUtil::truncate(lname, 8);
	// Talon1024 - Get resource path (uppercase, without leading slash)
	auto path = entry->path(true);
	StrUtil::upperIP(path);
	path.erase(0, 1);

	Log::debug("Adding entry {} to resource manager", path);

	// Check for palette entry
	if (type->id() == "palette")
		palettes_[name].add(entry);

	// Check for various image entries, so only accept images
	if (type->editor() == "gfx")
	{
		// Reject graphics that are not in a valid namespace:
		// Patches in wads can be in the global namespace as well, and
		// ZDoom textures can use sprites and graphics as patches
		if (!entry->isInNamespace("global") && !entry->isInNamespace("patches") && !entry->isInNamespace("sprites")
			&& !entry->isInNamespace("graphics") &&
			// Stand-alone textures can also be found in the hires namespace
			!entry->isInNamespace("hires") && !entry->isInNamespace("textures") &&
			// Flats are kinda boring in comparison
			!entry->isInNamespace("flats"))
			return;

		bool addToFpOnly = true;

		// Check for patch entry
		if (type->extraProps().propertyExists("patch") || entry->isInNamespace("patches")
			|| entry->isInNamespace("sprites"))
		{
			if (patches_[name].length() == 0)
			{
				addToFpOnly = false;
			}
			patches_[name].add(entry);
			if (!entry->parent()->isTreeless())
			{
				patches_fp_[path].add(entry);
				if ((lname.size() > 8 || patches_[name].length() > 0) && addToFpOnly)
				{
					patches_fp_only_[path].add(entry);
				}
			}
		}

		addToFpOnly = true;

		// Check for flat entry
		if (type->id() == "gfx_flat" || entry->isInNamespace("flats"))
		{
			if (flats_[name].length() == 0)
			{
				addToFpOnly = false;
			}
			flats_[name].add(entry);
			if (!entry->parent()->isTreeless())
			{
				flats_fp_[path].add(entry);
				if ((lname.size() > 8 || flats_[name].length() > 0) && addToFpOnly)
				{
					flats_fp_only_[path].add(entry);
				}
			}
		}

		// Check for stand-alone texture entry
		if (entry->isInNamespace("textures") || entry->isInNamespace("hires"))
		{
			satextures_[name].add(entry);
			if (!entry->parent()->isTreeless())
			{
				satextures_fp_[path].add(entry);
			}

			// Add name to hash table
			doom64_hash_table_[getTextureHash(name)] = name;
		}
	}

	// Check for TEXTUREx entry
	int txentry = 0;
	if (type->id() == "texturex")
		txentry = 1;
	else if (type->id() == "zdtextures")
		txentry = 2;
	if (txentry > 0)
	{
		// Load patch table if needed
		PatchTable ptable;
		if (txentry == 1)
		{
			Archive::SearchOptions opt;
			opt.match_type = EntryType::fromId("pnames");
			auto pnames    = entry->parent()->findLast(opt);
			ptable.loadPNAMES(pnames, entry->parent());
		}

		// Read texture list
		TextureXList tx;
		if (txentry == 1)
			tx.readTEXTUREXData(entry.get(), ptable);
		else
			tx.readTEXTURESData(entry.get());

		// Add all textures to resources
		CTexture* tex;
		for (unsigned a = 0; a < tx.size(); a++)
		{
			tex = tx.texture(a);
			textures_[tex->name()].add(tex, entry->parent());
		}
	}
}

// ----------------------------------------------------------------------------
// Removes a managed entry
// -----------------------------------------------------------------------------
void ResourceManager::removeEntry(shared_ptr<ArchiveEntry>& entry, string_view entry_name, bool full_check)
{
	if (!entry.get())
		return;

	// Get resource name (extension cut, uppercase)
	auto name = StrUtil::truncate(entry_name.empty() ? entry->upperNameNoExt() : entry_name, 8);
	auto path = entry->path(true);
	StrUtil::upperIP(path);
	path.erase(0, 1);

	Log::debug("Removing entry {} from resource manager", path);

	// Remove from palettes
	removeEntryFromMap(palettes_, name, entry, full_check);

	// Remove from patches
	removeEntryFromMap(patches_, name, entry, full_check);
	removeEntryFromMap(patches_fp_, path, entry, full_check);
	removeEntryFromMap(patches_fp_only_, path, entry, full_check);

	// Remove from flats
	removeEntryFromMap(flats_, name, entry, full_check);
	removeEntryFromMap(flats_fp_, path, entry, full_check);
	removeEntryFromMap(flats_fp_only_, path, entry, full_check);

	// Remove from stand-alone textures
	removeEntryFromMap(satextures_, name, entry, full_check);
	removeEntryFromMap(satextures_fp_, path, entry, full_check);

	// Check for TEXTUREx entry
	int txentry = 0;
	if (entry->type()->id() == "texturex")
		txentry = 1;
	else if (entry->type()->id() == "zdtextures")
		txentry = 2;
	if (txentry > 0)
	{
		// Read texture list
		TextureXList tx;
		PatchTable   ptable;
		if (txentry == 1)
			tx.readTEXTUREXData(entry.get(), ptable);
		else
			tx.readTEXTURESData(entry.get());

		// Remove all texture resources
		for (unsigned a = 0; a < tx.size(); a++)
			textures_[tx.texture(a)->name()].remove(entry->parent());
	}
}

// -----------------------------------------------------------------------------
// Dumps all patch names and the number of matching entries for each
// -----------------------------------------------------------------------------
void ResourceManager::listAllPatches()
{
	for (auto& i : patches_)
	{
		if (i.second.length() == 0)
			continue;

		Log::info("{} ({})", i.first, i.second.length());
	}
}

// -----------------------------------------------------------------------------
// Adds all current patch entries to [list]
// -----------------------------------------------------------------------------
void ResourceManager::putAllPatchEntries(vector<ArchiveEntry*>& list, Archive* priority, bool fullPath)
{
	for (auto& i : patches_)
	{
		auto entry = i.second.getEntry(priority);
		if (entry)
			list.push_back(entry);
	}

	if (!fullPath)
		return;

	for (auto& i : patches_fp_only_)
	{
		auto entry = i.second.getEntry(priority);
		if (entry)
			list.push_back(entry);
	}
}

// -----------------------------------------------------------------------------
// Adds all current textures to [list]
// -----------------------------------------------------------------------------
void ResourceManager::putAllTextures(vector<TextureResource::Texture*>& list, Archive* priority, Archive* ignore)
{
	// Add all primary textures to the list
	for (auto& i : textures_)
	{
		// Skip if no entries
		if (i.second.length() == 0)
			continue;

		// Go through resource textures
		auto res = i.second.textures_[0].get();
		for (int a = 0; a < i.second.length(); a++)
		{
			res = i.second.textures_[a].get();

			// Skip if it's in the 'ignore' archive
			auto res_parent = res->parent.lock().get();
			if (!res_parent || res_parent == ignore)
				continue;

			// If it's in the 'priority' archive, exit loop
			if (priority && res_parent == priority)
				break;

			// Otherwise, if it's in a 'later' archive than the current resource, set it
			if (App::archiveManager().archiveIndex(res_parent) <= App::archiveManager().archiveIndex(res_parent))
				res = i.second.textures_[a].get();
		}

		// Add texture resource to the list
		if (res->parent.lock().get() != ignore)
			list.push_back(res);
	}
}

// -----------------------------------------------------------------------------
// Adds all current texture names to [list]
// -----------------------------------------------------------------------------
void ResourceManager::putAllTextureNames(vector<string>& list)
{
	// Add all primary textures to the list
	for (auto& i : textures_)
		if (i.second.length() > 0) // Ignore if no entries
			list.push_back(i.first);
}

// -----------------------------------------------------------------------------
// Adds all current flat entries to [list]
// -----------------------------------------------------------------------------
void ResourceManager::putAllFlatEntries(vector<ArchiveEntry*>& list, Archive* priority, bool fullPath)
{
	for (auto& i : flats_)
	{
		auto entry = i.second.getEntry(priority);
		if (entry)
			list.push_back(entry);
	}

	if (!fullPath)
		return;

	for (auto& i : flats_fp_only_)
	{
		auto entry = i.second.getEntry(priority);
		if (entry)
			list.push_back(entry);
	}
}

// -----------------------------------------------------------------------------
// Adds all current flat names to [list]
// -----------------------------------------------------------------------------
void ResourceManager::putAllFlatNames(vector<string>& list)
{
	// Add all primary flats to the list
	for (auto& i : flats_)
		if (i.second.length() > 0) // Ignore if no entries
			list.push_back(i.first);
}

// -----------------------------------------------------------------------------
// Returns the most appropriate managed resource entry for [palette], or
// nullptr if no match found
// -----------------------------------------------------------------------------
ArchiveEntry* ResourceManager::getPaletteEntry(string_view palette, Archive* priority)
{
	return palettes_[StrUtil::upper(palette)].getEntry(priority);
}

// -----------------------------------------------------------------------------
// Returns the most appropriate managed resource entry for [patch],
// or nullptr if no match found
// -----------------------------------------------------------------------------
ArchiveEntry* ResourceManager::getPatchEntry(string_view patch, string_view nspace, Archive* priority)
{
	// Are we wanting to use a flat as a patch?
	if (StrUtil::equalCI(nspace, "flats"))
		return getFlatEntry(patch, priority);

	// Are we wanting to use a stand-alone texture as a patch?
	if (StrUtil::equalCI(nspace, "textures"))
		return getTextureEntry(patch, "textures", priority);

	auto patch_upper = StrUtil::upper(patch);
	auto entry       = patches_[patch_upper].getEntry(priority, nspace, true);
	if (entry)
		return entry;

	entry = patches_fp_[patch_upper].getEntry(priority, nspace, true);
	if (entry)
		return entry;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the most appropriate managed resource entry for [flat], or nullptr
// if no match found
// -----------------------------------------------------------------------------
ArchiveEntry* ResourceManager::getFlatEntry(string_view flat, Archive* priority)
{
	// Check resource with matching name exists
	auto  flat_upper = StrUtil::upper(flat);
	auto& res        = flats_[flat_upper];

	// Return most relevant entry
	auto entry = res.getEntry(priority);
	if (entry)
		return entry;

	entry = flats_fp_[flat_upper].getEntry(priority, "flats", true);
	if (entry)
		return entry;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the most appropriate managed resource entry for [texture], or
// nullptr if no match found
// -----------------------------------------------------------------------------
ArchiveEntry* ResourceManager::getTextureEntry(string_view texture, string_view nspace, Archive* priority)
{
	auto tex_upper = StrUtil::upper(texture);
	auto entry     = satextures_[tex_upper].getEntry(priority, nspace, true);
	if (entry)
		return entry;

	entry = satextures_fp_[tex_upper].getEntry(priority, nspace, true);
	if (entry)
		return entry;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the most appropriate managed texture for [texture], or nullptr if no
// match found
// -----------------------------------------------------------------------------
CTexture* ResourceManager::getTexture(string_view texture, Archive* priority, Archive* ignore)
{
	// Check texture resource with matching name exists
	auto& res = textures_[StrUtil::upper(texture)];
	if (res.textures_.empty())
		return nullptr;

	// Go through resource textures
	auto tex    = &res.textures_[0]->tex;
	auto parent = res.textures_[0]->parent.lock().get();
	for (auto& res_tex : res.textures_)
	{
		// Skip if it's in the 'ignore' archive
		auto rt_parent = res_tex->parent.lock().get();
		if (!rt_parent || rt_parent == ignore)
			continue;

		// If it's in the 'priority' archive, return it
		if (priority && rt_parent == priority)
			return &res_tex->tex;

		// Otherwise, if it's in a 'later' archive than the current resource entry, set it
		if (App::archiveManager().archiveIndex(parent) <= App::archiveManager().archiveIndex(rt_parent))
		{
			tex    = &res_tex->tex;
			parent = rt_parent;
		}
	}

	// Return the most relevant texture
	if (parent != ignore)
		return tex;
	else
		return nullptr;
}

void ResourceManager::updateEntry(ArchiveEntry& entry, bool remove, bool add)
{
	auto sptr = entry.getShared();
	if (remove)
		removeEntry(sptr);
	if (add)
		addEntry(sptr);

	signals_.resources_updated();
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------


CONSOLE_COMMAND(list_res_patches, 0, false)
{
	App::resources().listAllPatches();
}

#include "App.h"
CONSOLE_COMMAND(test_res_speed, 0, false)
{
	vector<ArchiveEntry*> list;

	Log::console("Testing...");

	long times[5];

	for (long& time : times)
	{
		auto start = App::runTimer();
		for (unsigned a = 0; a < 100; a++)
		{
			App::resources().putAllPatchEntries(list, nullptr);
			list.clear();
		}
		for (unsigned a = 0; a < 100; a++)
		{
			App::resources().putAllFlatEntries(list, nullptr);
			list.clear();
		}
		auto end = App::runTimer();
		time     = end - start;
	}

	float avg = float(times[0] + times[1] + times[2] + times[3] + times[4]) / 5.0f;
	Log::console(fmt::format("Test took {}ms avg", (int)avg));
}
