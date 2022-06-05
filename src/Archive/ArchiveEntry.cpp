
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, wad_force_uppercase, true, CVar::Flag::Save)
CVAR(Int, max_entry_size_mb, 512, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
unsigned maxEntrySizeBytes()
{
	constexpr unsigned MB_TO_BYTES = 1024 * 1024;
	return max_entry_size_mb * MB_TO_BYTES;
}
} // namespace


// -----------------------------------------------------------------------------
//
// ArchiveEntry Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchiveEntry class constructor
// -----------------------------------------------------------------------------
ArchiveEntry::ArchiveEntry(string_view name, uint32_t size) :
	name_{ name }, upper_name_{ name }, data_{ size }, type_{ EntryType::unknownType() }
{
	strutil::upperIP(upper_name_);
}

// -----------------------------------------------------------------------------
// ArchiveEntry class copy constructor
// -----------------------------------------------------------------------------
ArchiveEntry::ArchiveEntry(const ArchiveEntry& copy) :
	name_{ copy.name_ },
	upper_name_{ copy.upper_name_ },
	data_{ copy.data_ },
	type_{ copy.type_ },
	ex_props_{ copy.ex_props_ },
	data_hash_{ copy.data_hash_ },
	encrypted_{ copy.encrypted_ },
	reliability_{ copy.reliability_ }
{
	// Clear properties that shouldn't be copied
	ex_props_.remove("ZipIndex");
	ex_props_.remove("Offset");
	ex_props_.remove("filePath");
}

// -----------------------------------------------------------------------------
// Returns the entry name with no file extension
// -----------------------------------------------------------------------------
string_view ArchiveEntry::nameNoExt() const
{
	auto ext_pos = name_.find('.');
	if (ext_pos != string::npos)
		return { name_.data(), ext_pos };

	return name_;
}

// -----------------------------------------------------------------------------
// Returns the entry name in uppercase with no file extension
// -----------------------------------------------------------------------------
string_view ArchiveEntry::upperNameNoExt() const
{
	auto ext_pos = upper_name_.find('.');
	if (ext_pos != string::npos)
		return { upper_name_.data(), ext_pos };

	return upper_name_;
}

// -----------------------------------------------------------------------------
// Returns the entry's parent archive
// -----------------------------------------------------------------------------
Archive* ArchiveEntry::parent() const
{
	return parent_ ? parent_->archive() : nullptr;
}

// -----------------------------------------------------------------------------
// Returns the entry's top-level parent archive
// -----------------------------------------------------------------------------
Archive* ArchiveEntry::topParent() const
{
	if (parent_)
	{
		if (!parent_->archive()->parentEntry())
			return parent_->archive();

		return parent_->archive()->parentEntry()->topParent();
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the entry path in its parent archive
// -----------------------------------------------------------------------------
string ArchiveEntry::path(bool include_name) const
{
	auto path = parent_ ? parent_->path() : "";
	return include_name ? path + name() : path;
}

// -----------------------------------------------------------------------------
// Returns the 'next' entry from this (ie. index + 1) in its parent ArchiveDir,
// or nullptr if it is the last entry or has no parent dir
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveEntry::nextEntry() const
{
	return parent_ ? parent_->entryAt(parent_->entryIndex(this) + 1) : nullptr;
}

// -----------------------------------------------------------------------------
// Returns the 'previous' entry from this (ie. index - 1) in its parent
// ArchiveDir, or nullptr if it is the first entry or has no parent dir
// -----------------------------------------------------------------------------
ArchiveEntry* ArchiveEntry::prevEntry() const
{
	return parent_ ? parent_->entryAt(parent_->entryIndex(this) - 1) : nullptr;
}

// -----------------------------------------------------------------------------
// Returns the parent ArchiveDir's shared pointer to this entry, or
// nullptr if this entry has no parent
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> ArchiveEntry::getShared() const
{
	return parent_ ? parent_->sharedEntry(this) : nullptr;
}

// -----------------------------------------------------------------------------
// Returns the entry's index within its parent ArchiveDir, or -1 if it isn't
// in a dir
// -----------------------------------------------------------------------------
int ArchiveEntry::index() const
{
	return parent_ ? parent_->entryIndex(this) : -1;
}

// -----------------------------------------------------------------------------
// Returns the hash (XXH128) of the entry's data, calculating it if needed
// -----------------------------------------------------------------------------
const string& ArchiveEntry::hash() const
{
	if (data_hash_.empty())
		data_hash_ = data_.hash();

	return data_hash_;
}

// -----------------------------------------------------------------------------
// Sets the entry's name (but doesn't change state to modified)
// -----------------------------------------------------------------------------
void ArchiveEntry::setName(string_view name)
{
	name_       = name;
	upper_name_ = strutil::upper(name);
}

// -----------------------------------------------------------------------------
// Sets the entry's state. Won't change state if the change would be redundant
// (eg new->modified, unmodified->unmodified)
// -----------------------------------------------------------------------------
void ArchiveEntry::setState(State state, bool silent)
{
	if (state_locked_ || (state == State::Unmodified && state_ == State::Unmodified))
		return;

	if (state == State::Unmodified)
		state_ = State::Unmodified;
	else if (state > state_)
		state_ = state;

	// Notify parent archive this entry has been modified
	if (!silent)
		stateChanged();
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
// Sanitizes the entry name so that it is valid for archive format [format]
// -----------------------------------------------------------------------------
void ArchiveEntry::formatName(const ArchiveFormat& format)
{
	bool changed = false;

	// Perform character substitution if needed
	name_ = misc::fileNameToLumpName(name_);

	// Max length
	if (format.max_name_length > 0 && static_cast<int>(name_.size()) > format.max_name_length)
	{
		strutil::truncateIP(name_, format.max_name_length);
		changed = true;
	}

	// Uppercase
	if (format.prefer_uppercase && wad_force_uppercase)
		strutil::upperIP(name_);

	// Remove \ or / if the format supports folders
	if (format.supports_dirs && (name_.find('/') != string::npos || name_.find('\\') != string::npos))
	{
		name_   = misc::lumpNameToFileName(name_);
		changed = true;
	}

	// Remove extension if the format doesn't have them
	if (!format.names_extensions)
		if (const auto pos = name_.find('.'); pos != string::npos)
			strutil::truncateIP(name_, pos);

	// Update upper name
	if (changed)
		upper_name_ = strutil::upper(name_);
}

// -----------------------------------------------------------------------------
// Renames the entry
// -----------------------------------------------------------------------------
bool ArchiveEntry::rename(string_view new_name)
{
	// Check if locked
	if (locked_)
	{
		global::error = "Entry is locked";
		return false;
	}

	// Update attributes
	setName(new_name);
	setState(State::Modified);

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
		global::error = "Entry is locked";
		return false;
	}

	// Check size
	if (new_size > maxEntrySizeBytes())
	{
		global::error = "Over maximum entry size";
		return false;
	}

	// Update attributes
	setState(State::Modified);

	// Resize the data
	if (data_.reSize(new_size, preserve_data))
	{
		data_hash_ = data_.hash();
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Clears entry data and resets its size to zero
// -----------------------------------------------------------------------------
bool ArchiveEntry::clearData()
{
	// Check if locked
	if (locked_)
	{
		global::error = "Entry is locked";
		return false;
	}

	// Delete the data
	data_.clear();
	data_hash_.clear();

	return true;
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
		global::error = "Entry is locked";
		return false;
	}

	// Check size
	if (size > maxEntrySizeBytes())
	{
		global::error = "Over maximum entry size";
		return false;
	}

	// Clear any current data
	clearData();

	// Copy data into the entry
	data_.importMem(static_cast<const uint8_t*>(data), size);

	// Update attributes
	data_hash_ = data_.hash();
	setType(EntryType::unknownType());
	setState(State::Modified);

	return true;
}

// -----------------------------------------------------------------------------
// Imports data from a MemChunk object into the entry, resizing it and clearing
// any currently existing data. If [size] is 0 all data from [offset] to the end
// of the MemChunk will be read.
// Returns false if the MemChunk has no data, or true otherwise.
// -----------------------------------------------------------------------------
bool ArchiveEntry::importMemChunk(const MemChunk& mc, uint32_t offset, uint32_t size)
{
	// Check that the given MemChunk has data
	if (mc.hasData())
	{
		// If no size given read to end of MemChunk
		if (size == 0)
			size = mc.size() - offset;

		// Check valid offset given
		if (offset >= mc.size())
		{
			log::error(
				"ArchiveEntry::importMemChunk: Invalid offset - {} is larger than MemChunk size {}", offset, size);
			return false;
		}

		// Ensure offset+size is valid
		if (offset + size > mc.size())
			size = mc.size() - offset;

		// Copy the data from the MemChunk into the entry
		return importMem(mc.data() + offset, size);
	}

	return false;
}

// -----------------------------------------------------------------------------
// Loads a portion of a file into the entry, overwriting any existing data
// currently in the entry. A size of 0 means load from the offset to the end of
// the file.
// Returns false if the file does not exist or the given offset/size are out of
// bounds, otherwise returns true.
// -----------------------------------------------------------------------------
bool ArchiveEntry::importFile(string_view filename, uint32_t offset, uint32_t size)
{
	// Check if locked
	if (locked_)
	{
		global::error = "Entry is locked";
		return false;
	}

	// Open the file
	wxFile file({ filename.data(), filename.size() });

	// Check that it opened ok
	if (!file.IsOpened())
	{
		global::error = "Unable to open file for reading";
		return false;
	}

	// Get the size to read, if zero
	if (size == 0)
		size = file.Length() - offset;

	// Check offset/size bounds
	if (offset + size > file.Length())
		return false;

	// Check size
	if (size > maxEntrySizeBytes())
	{
		global::error = fmt::format("File \"{}\" is over maximum entry size", filename);
		return false;
	}

	// Create temporary buffer and load file contents
	vector<uint8_t> temp_buf(size);
	file.Seek(offset, wxFromStart);
	file.Read(temp_buf.data(), size);

	// Import data into entry
	importMem(temp_buf.data(), size);

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
		global::error = "Entry is locked";
		return false;
	}

	// Check size
	if (len > maxEntrySizeBytes())
	{
		global::error = "Over maximum entry size";
		return false;
	}

	// Import data from the file stream
	if (data_.importFileStreamWx(file, len))
	{
		// Update attributes
		data_hash_ = data_.hash();
		setType(EntryType::unknownType());
		setState(State::Modified);

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Imports data from another entry into this entry, resizing it and clearing
// any currently existing data.
// Returns false if the entry is null, true otherwise
// -----------------------------------------------------------------------------
bool ArchiveEntry::importEntry(const ArchiveEntry* entry)
{
	// Check if locked
	if (locked_)
	{
		global::error = "Entry is locked";
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
bool ArchiveEntry::exportFile(string_view filename) const
{
	// Attempt to open file
	wxFile file({ filename.data(), filename.size() }, wxFile::write);

	// Check it opened ok
	if (!file.IsOpened())
	{
		global::error = fmt::format("Unable to open file {} for writing", filename);
		return false;
	}

	// Write entry data to the file, if any
	if (auto data = rawData())
		file.Write(data, size());

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
		global::error = "Entry is locked";
		return false;
	}

	// Perform the write
	if (data_.write(data, size))
	{
		// Update attributes
		data_hash_.clear();
		setState(State::Modified);

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Reads data from the entry MemChunk
// -----------------------------------------------------------------------------
bool ArchiveEntry::read(void* buf, uint32_t size) const
{
	return data_.read(buf, size);
}

// -----------------------------------------------------------------------------
// Returns the entry's size as a string
// -----------------------------------------------------------------------------
string ArchiveEntry::sizeString() const
{
	return misc::sizeAsString(size());
}

// -----------------------------------------------------------------------------
// ArchiveEntry::stateChanged
//
// Notifies the entry's parent archive that the entry has been modified
// -----------------------------------------------------------------------------
void ArchiveEntry::stateChanged()
{
	if (auto parent_archive = parent())
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
	strutil::Path fn(name_);

	// Set new extension
	fn.setExtension(type_->extension());

	// Rename
	auto parent_archive = parent();
	auto filename       = fn.fileName();
	if (parent_archive)
		parent_archive->renameEntry(this, { filename.data(), filename.size() });
	else
		rename(filename);
}

// -----------------------------------------------------------------------------
// Returns true if the entry is in the [ns] namespace within its parent, false
// otherwise
// -----------------------------------------------------------------------------
bool ArchiveEntry::isInNamespace(string_view ns)
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
ArchiveEntry* ArchiveEntry::relativeEntry(string_view path, bool allow_absolute_path) const
{
	if (!parent_)
		return nullptr;

	// Try relative to this entry
	auto include = parent_->archive()->entryAtPath(this->path().append(path));

	// Try absolute path
	if (!include && allow_absolute_path)
		include = parent_->archive()->entryAtPath(path);

	return include;
}
