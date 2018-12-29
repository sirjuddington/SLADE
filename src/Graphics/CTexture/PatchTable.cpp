
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PatchTable.cpp
// Description: Handles a collection of Patches and their corresponding archive
//              entries (ie, encapsulates a PNAMES entry)
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
#include "PatchTable.h"
#include "App.h"
#include "CTexture.h"
#include "General/ResourceManager.h"


// -----------------------------------------------------------------------------
//
// PatchTable Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the patch at [index], or an 'invalid' patch if [index] is invalid
// -----------------------------------------------------------------------------
PatchTable::Patch& PatchTable::patch(size_t index)
{
	// Check index
	if (index >= patches_.size())
		return patch_invalid_;

	// Return patch at index
	return patches_[index];
}

// -----------------------------------------------------------------------------
// Returns the patch matching [name], or an 'invalid' patch if no match is found
// -----------------------------------------------------------------------------
PatchTable::Patch& PatchTable::patch(const string& name)
{
	// Go through list
	for (auto& patch : patches_)
	{
		if (S_CMP(patch.name, name))
			return patch;
	}

	return patch_invalid_;
}

// -----------------------------------------------------------------------------
// Returns the name of the patch at [index], or an empty string if [index] is
// invalid
// -----------------------------------------------------------------------------
string PatchTable::patchName(size_t index)
{
	// Check index
	if (index >= patches_.size())
		return wxEmptyString;

	// Return name at index
	return patches_[index].name;
}

// -----------------------------------------------------------------------------
// Returns the entry associated with the patch at [index], or null if [index] is
// invalid
// -----------------------------------------------------------------------------
ArchiveEntry* PatchTable::patchEntry(size_t index)
{
	// Check index
	if (index >= patches_.size())
		return nullptr;

	// Patches namespace > graphics
	auto entry = App::resources().getPatchEntry(patches_[index].name, "patches", parent_);
	if (!entry)
		entry = App::resources().getPatchEntry(patches_[index].name, "graphics", parent_);

	return entry;
}

// -----------------------------------------------------------------------------
// Returns the entry associated with the patch matching [name], or null if no
// match found
// -----------------------------------------------------------------------------
ArchiveEntry* PatchTable::patchEntry(const string& name)
{
	// Search for patch by name
	for (size_t a = 0; a < patches_.size(); a++)
	{
		if (!patches_[a].name.CmpNoCase(name))
			return patchEntry(a);
	}

	// Not found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the index of the patch matching [name], or -1 if no match found
// -----------------------------------------------------------------------------
int32_t PatchTable::patchIndex(const string& name)
{
	// Search for patch by name
	for (size_t a = 0; a < patches_.size(); a++)
	{
		if (!patches_[a].name.CmpNoCase(name))
			return a;
	}

	// Not found
	return -1;
}

// -----------------------------------------------------------------------------
// Returns the index of the patch associated with [entry], or null if no match
// found
// -----------------------------------------------------------------------------
int32_t PatchTable::patchIndex(ArchiveEntry* entry)
{
	// Search for patch by entry
	for (size_t a = 0; a < patches_.size(); a++)
	{
		if (App::resources().getPatchEntry(patches_[a].name, "patches", parent_) == entry)
			return a;
	}

	// Not found
	return -1;
}

// -----------------------------------------------------------------------------
// Removes the patch at [index].
// Returns false if [index] is out of range, true otherwise
// -----------------------------------------------------------------------------
bool PatchTable::removePatch(unsigned index)
{
	// Check index
	if (index >= patches_.size())
		return false;

	// Remove the patch
	patches_.erase(patches_.begin() + index);

	// Announce
	announce("modified");

	return true;
}

// -----------------------------------------------------------------------------
// Replaces the patch info at [index] with a new name (newname).
// Also attempts to find the ArchiveEntry matching [newname] in [parent] and
// resource archives.
// Returns false if [index] is out of range or no matching entry was found
// -----------------------------------------------------------------------------
bool PatchTable::replacePatch(unsigned index, const string& newname)
{
	// Check index
	if (index >= patches_.size())
		return false;

	// Change the patch name
	patches_[index].name = newname;

	// Announce
	announce("modified");

	return true;
}

// -----------------------------------------------------------------------------
// Adds a new patch with [name] to the end of the list
// -----------------------------------------------------------------------------
bool PatchTable::addPatch(const string& name, bool allow_dup)
{
	// Check patch doesn't already exist
	if (!allow_dup)
	{
		for (auto& patch : patches_)
		{
			if (S_CMP(name, patch.name))
				return false;
		}
	}

	// Add the patch
	patches_.emplace_back(name);

	// Announce
	announce("modified");

	return true;
}

// -----------------------------------------------------------------------------
// Loads a PNAMES entry, returns true on success, false otherwise
// -----------------------------------------------------------------------------
bool PatchTable::loadPNAMES(ArchiveEntry* pnames, Archive* parent)
{
	// Check entry was given
	if (!pnames)
		return false;

	// Mute while loading
	setMuted(true);

	// Clear current table
	patches_.clear();

	// Setup parent archive
	if (!parent)
		parent = pnames->parent();

	// Read number of pnames
	uint32_t n_pnames = 0;
	pnames->seek(0, SEEK_SET);
	if (!pnames->read(&n_pnames, 4))
	{
		Log::error("PNAMES lump is corrupt");
		return false;
	}

	// Read pnames content
	for (uint32_t a = 0; a < n_pnames; a++)
	{
		char pname[9] = "";
		pname[8]      = 0;

		// Try to read pname
		if (!pnames->read(&pname, 8))
		{
			Log::error(S_FMT("PNAMES entry %i is corrupt", a));
			return false;
		}

		// Add new patch
		addPatch(wxString(pname).Upper(), true);
	}

	// Update variables
	parent_ = parent;
	setMuted(false);

	// Announce
	announce("modified");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the patch table to the entry [pnames].
// Returns false if no entry was given, true otherwise
// -----------------------------------------------------------------------------
bool PatchTable::writePNAMES(ArchiveEntry* pnames)
{
	// Check entry was given
	if (!pnames)
		return false;

	// Determine entry size
	int32_t  npnames   = patches_.size();
	uint32_t entrysize = 4 + (npnames * 8);

	// Create MemChunk to write to
	MemChunk pndata(entrysize);

	// Write header
	pndata.write(&npnames, 4);

	// Write patch names
	for (auto& patch : patches_)
	{
		char name[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // Init name to all zeros for XWE compatibility
		strncpy(name, CHR(patch.name), patch.name.Len());

		pndata.write(name, 8);
	}

	// Load data to entry
	pnames->importMemChunk(pndata);

	// Update entry type
	EntryType::detectEntryType(pnames);

	return true;
}

// -----------------------------------------------------------------------------
// Clears all patch use count data
// -----------------------------------------------------------------------------
void PatchTable::clearPatchUsage()
{
	for (auto& patch : patches_)
		patch.used_in.clear();

	// Announce
	announce("modified");
}

// -----------------------------------------------------------------------------
// Updates patch usage data for [tex]
// -----------------------------------------------------------------------------
void PatchTable::updatePatchUsage(CTexture* tex)
{
	// Remove texture from all patch usage tables
	for (auto& patch : patches_)
		patch.removeTextureUsage(tex->name());

	// Update patch usage counts for texture
	for (unsigned a = 0; a < tex->nPatches(); a++)
		patch(tex->patch(a)->name()).used_in.push_back(tex->name());

	// Announce
	announce("modified");
}
