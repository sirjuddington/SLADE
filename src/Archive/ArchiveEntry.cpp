
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveEntry.cpp
// Description: ArchiveEntry, a class that represents and holds information
//              about a single 'entry', ie, a chunk of data with a name and
//              various other properties, that is usually part of an archive
//              (archive=wad, entry=lump)
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
#include "ArchiveEntry.h"
#include "Archive.h"
#include "General/Misc.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// ArchiveEntry Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchiveEntry class constructor
// -----------------------------------------------------------------------------
ArchiveEntry::ArchiveEntry(string name, uint32_t size)
{
	// Initialise attributes
	parent_       = nullptr;
	name_         = name;
	upper_name_   = name.Upper();
	size_         = size;
	data_loaded_  = true;
	state_        = 2;
	type_         = EntryType::unknownType();
	locked_       = false;
	state_locked_ = false;
	reliability_  = 0;
	next_         = nullptr;
	prev_         = nullptr;
	encrypted_    = ENC_NONE;
	index_guess_  = 0;
}

// -----------------------------------------------------------------------------
// ArchiveEntry class copy constructor
// -----------------------------------------------------------------------------
ArchiveEntry::ArchiveEntry(ArchiveEntry& copy)
{
	// Initialise (copy) attributes
	parent_       = nullptr;
	name_         = copy.name_;
	upper_name_   = copy.upper_name_;
	size_         = copy.size_;
	data_loaded_  = true;
	state_        = 2;
	type_         = copy.type_;
	locked_       = false;
	state_locked_ = false;
	reliability_  = copy.reliability_;
	next_         = nullptr;
	prev_         = nullptr;
	encrypted_    = copy.encrypted_;
	index_guess_  = 0;

	// Copy data
	data_.importMem(copy.rawData(true), copy.size());

	// Copy extra properties
	copy.exProps().copyTo(ex_props_);

	// Clear properties that shouldn't be copied
	ex_props_.removeProperty("ZipIndex");
	ex_props_.removeProperty("Offset");

	// Set entry state
	state_        = 2;
	state_locked_ = false;
}

// -----------------------------------------------------------------------------
// ArchiveEntry class destructor
// -----------------------------------------------------------------------------
ArchiveEntry::~ArchiveEntry() {}

// -----------------------------------------------------------------------------
// Returns the entry name. If [cut_ext] is true and the name has an extension,
// it will be cut from the returned name
// -----------------------------------------------------------------------------
string ArchiveEntry::name(bool cut_ext) const
{
	if (!cut_ext)
		return name_;

	// Sanitize name if it contains the \ character (possible in WAD).
	string saname = Misc::lumpNameToFileName(name_);

	if (saname.Contains(StringUtils::FULLSTOP))
		return saname.BeforeLast('.');
	else
		return saname;
}

// -----------------------------------------------------------------------------
// Returns the entry name in uppercase
// -----------------------------------------------------------------------------
string ArchiveEntry::upperName()
{
	return upper_name_;
}

// -----------------------------------------------------------------------------
// Returns the entry name in uppercase with no file extension
// -----------------------------------------------------------------------------
string ArchiveEntry::upperNameNoExt()
{
	if (upper_name_.Contains(StringUtils::FULLSTOP))
		return Misc::lumpNameToFileName(upper_name_).BeforeLast('.');
	else
		return Misc::lumpNameToFileName(upper_name_);
}

// -----------------------------------------------------------------------------
// Returns the entry's parent archive
// -----------------------------------------------------------------------------
Archive* ArchiveEntry::parent()
{
	if (parent_)
		return parent_->archive();
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the entry's top-level parent archive
// -----------------------------------------------------------------------------
Archive* ArchiveEntry::topParent()
{
	if (parent_)
	{
		if (!parent_->archive()->parentEntry())
			return parent_->archive();
		else
			return parent_->archive()->parentEntry()->topParent();
	}
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the entry path in its parent archive
// -----------------------------------------------------------------------------
string ArchiveEntry::path(bool include_name) const
{
	// Get the entry path
	string path = parent_->path();

	if (include_name)
		return path + name();
	else
		return path;
}

// -----------------------------------------------------------------------------
// Returns a pointer to the entry data. If no entry data exists and [allow_load]
// is true, entry data will be loaded from its parent archive (if it exists)
// -----------------------------------------------------------------------------
const uint8_t* ArchiveEntry::rawData(bool allow_load)
{
	// Return entry data
	return data(allow_load).data();
}

// -----------------------------------------------------------------------------
// Returns the entry data MemChunk. If no entry data exists and [allow_load]
// is true, entry data will be loaded from its parent archive (if it exists)
// -----------------------------------------------------------------------------
MemChunk& ArchiveEntry::data(bool allow_load)
{
	// Get parent archive
	Archive* parent_archive = parent();

	// Load the data if needed (and possible)
	if (allow_load && !isLoaded() && parent_archive && size_ > 0)
	{
		data_loaded_ = parent_archive->loadEntryData(this);
		setState(0);
	}

	return data_;
}

// -----------------------------------------------------------------------------
// Returns the parent ArchiveTreeNode's shared pointer to this entry, or
// nullptr if this entry has no parent
// -----------------------------------------------------------------------------
ArchiveEntry::SPtr ArchiveEntry::getShared()
{
	if (parent_)
		return parent_->sharedEntry(this);
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Sets the entry's state. Won't change state if the change would be redundant
// (eg new->modified, unmodified->unmodified)
// -----------------------------------------------------------------------------
void ArchiveEntry::setState(uint8_t state, bool silent)
{
	if (state_locked_ || (state == 0 && this->state_ == 0))
		return;

	if (state == 0)
		this->state_ = 0;
	else
	{
		if (state > this->state_)
			this->state_ = state;
	}

	// Notify parent archive this entry has been modified
	if (!silent)
		stateChanged();
}

// -----------------------------------------------------------------------------
// 'Unloads' entry data from memory
// -----------------------------------------------------------------------------
void ArchiveEntry::unloadData()
{
	// Check there is any data to be 'unloaded'
	if (!data_.hasData() || !data_loaded_)
		return;

	// Only unload if the data wasn't modified
	if (state() > 0)
		return;

	// Delete any data
	data_.clear();

	// Update variables etc
	setLoaded(false);
}

// -----------------------------------------------------------------------------
// Locks the entry. A locked entry cannot be modified
// -----------------------------------------------------------------------------
void ArchiveEntry::lock()
{
	// Lock the entry
	locked_ = true;

	// Inform parent
	stateChanged();
}

// -----------------------------------------------------------------------------
// Unlocks the entry
// -----------------------------------------------------------------------------
void ArchiveEntry::unlock()
{
	// Unlock the entry
	locked_ = false;

	// Inform parent
	stateChanged();
}

// -----------------------------------------------------------------------------
// Renames the entry
// -----------------------------------------------------------------------------
bool ArchiveEntry::rename(string new_name)
{
	// Check if locked
	if (locked_)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Update attributes
	name_       = new_name;
	upper_name_ = name_.Upper();
	setState(1);

	return true;
}

// -----------------------------------------------------------------------------
// Resizes the entry to [new_size]. If [preserve_data] is true, any existing
// data is preserved
// -----------------------------------------------------------------------------
bool ArchiveEntry::resize(uint32_t new_size, bool preserve_data)
{
	// Check if locked
	if (locked_)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Update attributes
	setState(1);

	return data_.reSize(new_size, preserve_data);
}

// -----------------------------------------------------------------------------
// Clears entry data and resets its size to zero
// -----------------------------------------------------------------------------
void ArchiveEntry::clearData()
{
	// Check if locked
	if (locked_)
	{
		Global::error = "Entry is locked";
		return;
	}

	// Delete the data
	data_.clear();

	// Reset attributes
	size_        = 0;
	data_loaded_ = false;
}

// -----------------------------------------------------------------------------
// Imports a chunk of memory into the entry, resizing it and clearing any
// currently existing data.
// Returns false if data pointer is invalid, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveEntry::importMem(const void* data, uint32_t size)
{
	// Check parameters
	if (!data)
		return false;

	// Check if locked
	if (locked_)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Clear any current data
	clearData();

	// Copy data into the entry
	this->data_.importMem((const uint8_t*)data, size);

	// Update attributes
	this->size_ = size;
	setLoaded();
	setType(EntryType::unknownType());
	setState(1);

	return true;
}

// -----------------------------------------------------------------------------
// Imports data from a MemChunk object into the entry, resizing it and clearing
// any currently existing data.
// Returns false if the MemChunk has no data, or true otherwise.
// -----------------------------------------------------------------------------
bool ArchiveEntry::importMemChunk(MemChunk& mc)
{
	// Check that the given MemChunk has data
	if (mc.hasData())
	{
		// Copy the data from the MemChunk into the entry
		return importMem(mc.data(), mc.size());
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Loads a portion of a file into the entry, overwriting any existing data
// currently in the entry. A size of 0 means load from the offset to the end of
// the file.
// Returns false if the file does not exist or the given offset/size are out of
// bounds, otherwise returns true.
// -----------------------------------------------------------------------------
bool ArchiveEntry::importFile(string filename, uint32_t offset, uint32_t size)
{
	// Check if locked
	if (locked_)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Open the file
	wxFile file(filename);

	// Check that it opened ok
	if (!file.IsOpened())
	{
		Global::error = "Unable to open file for reading";
		return false;
	}

	// Get the size to read, if zero
	if (size == 0)
		size = file.Length() - offset;

	// Check offset/size bounds
	if (offset + size > file.Length())
		return false;

	// Create temporary buffer and load file contents
	uint8_t* temp_buf = new uint8_t[size];
	file.Seek(offset, wxFromStart);
	file.Read(temp_buf, size);

	// Import data into entry
	importMem(temp_buf, size);

	// Delete temp buffer
	delete[] temp_buf;

	return true;
}

// -----------------------------------------------------------------------------
// Imports [len] data from [file]
// -----------------------------------------------------------------------------
bool ArchiveEntry::importFileStream(wxFile& file, uint32_t len)
{
	// Check if locked
	if (locked_)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Import data from the file stream
	if (data_.importFileStream(file, len))
	{
		// Update attributes
		this->size_ = data_.size();
		setLoaded();
		setType(EntryType::unknownType());
		setState(1);

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Imports data from another entry into this entry, resizing it and clearing
// any currently existing data.
// Returns false if the entry is null, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveEntry::importEntry(ArchiveEntry* entry)
{
	// Check if locked
	if (locked_)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Check parameters
	if (!entry)
		return false;

	// Copy entry data
	importMem(entry->rawData(), entry->size());

	return true;
}

// -----------------------------------------------------------------------------
// Exports entry data to a file.
// Returns false if file cannot be written, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveEntry::exportFile(string filename)
{
	// Attempt to open file
	wxFile file(filename, wxFile::write);

	// Check it opened ok
	if (!file.IsOpened())
	{
		Global::error = S_FMT("Unable to open file %s for writing", filename);
		return false;
	}

	// Write entry data to the file, if any
	const uint8_t* data = rawData();
	if (data)
		file.Write(data, this->size());

	return true;
}

// -----------------------------------------------------------------------------
// Writes data to the entry MemChunk
// -----------------------------------------------------------------------------
bool ArchiveEntry::write(const void* data, uint32_t size)
{
	// Check if locked
	if (locked_)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Load data if it isn't already
	if (isLoaded())
		rawData(true);

	// Perform the write
	if (this->data_.write(data, size))
	{
		// Update attributes
		this->size_ = this->data_.size();
		setState(1);

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Reads data from the entry MemChunk
// -----------------------------------------------------------------------------
bool ArchiveEntry::read(void* buf, uint32_t size)
{
	// Load data if it isn't already
	if (isLoaded())
		rawData(true);

	return data_.read(buf, size);
}

// -----------------------------------------------------------------------------
// Returns the entry's size as a string
// -----------------------------------------------------------------------------
string ArchiveEntry::sizeString()
{
	return Misc::sizeAsString(size());
}

// -----------------------------------------------------------------------------
// ArchiveEntry::stateChanged
//
// Notifies the entry's parent archive that the entry has been modified
// -----------------------------------------------------------------------------
void ArchiveEntry::stateChanged()
{
	Archive* parent_archive = parent();
	if (parent_archive)
		parent_archive->entryStateChanged(this);
}

// -----------------------------------------------------------------------------
// Sets the entry's name extension to the extension defined in its EntryType
// -----------------------------------------------------------------------------
void ArchiveEntry::setExtensionByType()
{
	// Ignore if the parent archive doesn't support entry name extensions
	if (parent() && !parent()->formatDesc().names_extensions)
		return;

	// Convert name to wxFileName for processing
	wxFileName fn(name_);

	// Set new extension
	fn.SetExt(type_->extension());

	// Rename
	Archive* parent_archive = parent();
	if (parent_archive)
		parent_archive->renameEntry(this, fn.GetFullName());
	else
		rename(fn.GetFullName());
}

// -----------------------------------------------------------------------------
// Returns true if the entry is in the [ns] namespace within its parent, false
// otherwise
// -----------------------------------------------------------------------------
bool ArchiveEntry::isInNamespace(string ns)
{
	// Can't do this without parent archive
	if (!parent())
		return false;

	// Some special cases first
	if (ns == "graphics" && parent()->formatId() == "wad")
		ns = "global"; // Graphics namespace doesn't exist in wad files, use global instead

	return parent()->detectNamespace(this) == ns;
}

// -----------------------------------------------------------------------------
// Returns the entry at [path] relative to [base], or failing that, the entry
// at absolute [path] in the archive (if [allow_absolute_path] is true)
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveEntry::relativeEntry(const string& at_path, bool allow_absolute_path) const
{
	if (!parent_)
		return nullptr;

	// Try relative to this entry
	auto include = parent_->archive()->entryAtPath(path() + at_path);

	// Try absolute path
	if (!include && allow_absolute_path)
		include = parent_->archive()->entryAtPath(at_path);

	return include;
}
