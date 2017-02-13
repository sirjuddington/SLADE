
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ArchiveEntry.cpp
 * Description: ArchiveEntry, a class that represents and holds
 *              information about a single 'entry', ie, a chunk of
 *              data with a name and various other properties, that
 *              is usually part of an archive (archive=wad, entry=lump)
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
#include "ArchiveEntry.h"
#include "Archive.h"
#include "General/Misc.h"
#include "Utility/StringUtils.h"


/*******************************************************************
 * ARCHIVEENTRY CLASS FUNCTIONS
 *******************************************************************/

/* ArchiveEntry::ArchiveEntry
 * ArchiveEntry class constructor
 *******************************************************************/
ArchiveEntry::ArchiveEntry(string name, uint32_t size)
{
	// Initialise attributes
	this->parent = NULL;
	this->name = name;
	this->upper_name = name.Upper();
	this->size = size;
	this->data_loaded = true;
	this->state = 2;
	this->type = EntryType::unknownType();
	this->locked = false;
	this->state_locked = false;
	this->reliability = 0;
	this->next = NULL;
	this->prev = NULL;
	this->encrypted = ENC_NONE;
	this->index_guess = 0;
}

/* ArchiveEntry::ArchiveEntry
 * ArchiveEntry class copy constructor
 *******************************************************************/
ArchiveEntry::ArchiveEntry(ArchiveEntry& copy)
{
	// Initialise (copy) attributes
	this->parent = NULL;
	this->name = copy.name;
	this->upper_name = copy.upper_name;
	this->size = copy.size;
	this->data_loaded = true;
	this->state = 2;
	this->type = copy.type;
	this->locked = false;
	this->state_locked = false;
	this->reliability = copy.reliability;
	this->next = NULL;
	this->prev = NULL;
	this->encrypted = copy.encrypted;
	this->index_guess = 0;

	// Copy data
	data.importMem(copy.getData(true), copy.getSize());

	// Copy extra properties
	copy.exProps().copyTo(ex_props);

	// Clear properties that shouldn't be copied
	ex_props.removeProperty("ZipIndex");
	ex_props.removeProperty("Offset");

	// Set entry state
	state = 2;
	state_locked = false;
}

/* ArchiveEntry::~ArchiveEntry
 * ArchiveEntry class destructor
 *******************************************************************/
ArchiveEntry::~ArchiveEntry()
{
}

/* ArchiveEntry::getName
 * Returns the entry name. If [cut_ext] is true and the name has an
 * extension, it will be cut from the returned name
 *******************************************************************/
string ArchiveEntry::getName(bool cut_ext)
{
	if (!cut_ext)
		return name;

	// Sanitize name if it contains the \ character (possible in WAD).
	string saname = Misc::lumpNameToFileName(name);

	if (saname.Contains(StringUtils::FULLSTOP))
		return saname.BeforeLast('.');
	else
		return saname;
	/*
	// cut extension through wx function
	wxFileName fn(saname);

	// Perform reverse operation and return
	return Misc::fileNameToLumpName(fn.GetName());
	*/
}


/* ArchiveEntry::getUpperName
 * Returns the entry name in uppercase
 *******************************************************************/
string ArchiveEntry::getUpperName()
{
	return upper_name;
}

/* ArchiveEntry::getUpperNameNoExt
 * Returns the entry name in uppercase with no file extension
 *******************************************************************/
string ArchiveEntry::getUpperNameNoExt()
{
	if (upper_name.Contains(StringUtils::FULLSTOP))
		return Misc::lumpNameToFileName(upper_name).BeforeLast('.');
	else
		return Misc::lumpNameToFileName(upper_name);
}

/* ArchiveEntry::getParent
 * Returns the entry's parent archive
 *******************************************************************/
Archive* ArchiveEntry::getParent()
{
	if (parent)
		return parent->getArchive();
	else
		return NULL;
}

/* ArchiveEntry::getParent
 * Returns the entry's top-level parent archive
 *******************************************************************/
Archive* ArchiveEntry::getTopParent()
{
	if (parent)
	{
		if (!parent->getArchive()->getParent())
			return parent->getArchive();
		else
			return parent->getArchive()->getParent()->getTopParent();
	}
	else
		return NULL;
}

/* ArchiveEntry::getPath
 * Returns the entry path in its parent archive
 *******************************************************************/
string ArchiveEntry::getPath(bool name)
{
	// Get the entry path
	string path = parent->getPath();

	if (name)
		return path + getName();
	else
		return path;
}

/* ArchiveEntry::getData
 * Returns a pointer to the entry data. If no entry data exists and
 * allow_load is true, entry data will be loaded from its parent
 * archive (if it exists)
 *******************************************************************/
const uint8_t* ArchiveEntry::getData(bool allow_load)
{
	// Return entry data
	return getMCData(allow_load).getData();
}

/* ArchiveEntry::getMCData
 * Returns the entry data MemChunk. If no entry data exists and
 * allow_load is true, entry data will be loaded from its parent
 * archive (if it exists)
 *******************************************************************/
MemChunk& ArchiveEntry::getMCData(bool allow_load)
{
	// Get parent archive
	Archive* parent_archive = getParent();

	// Load the data if needed (and possible)
	if (allow_load && !isLoaded() && parent_archive && size > 0)
	{
		data_loaded = parent_archive->loadEntryData(this);
		setState(0);
	}

	return data;
}

/* ArchiveEntry::getShared
 * Returns the parent ArchiveTreeNode's shared pointer to this entry,
 * or nullptr if this entry has no parent
 *******************************************************************/
ArchiveEntry::SPtr ArchiveEntry::getShared()
{
	if (parent)
		return parent->getEntryShared(this);
	else
		return nullptr;
}

/* ArchiveEntry::setState
 * Sets the entry's state. Won't change state if the change would be
 * redundant (eg new->modified, unmodified->unmodified)
 *******************************************************************/
void ArchiveEntry::setState(uint8_t state)
{
	if (state_locked || (state == 0 && this->state == 0))
		return;

	if (state == 0)
		this->state = 0;
	else
	{
		if (state > this->state)
			this->state = state;
	}

	// Notify parent archive this entry has been modified
	stateChanged();
}

/* ArchiveEntry::unloadData
 * 'Unloads' entry data from memory
 *******************************************************************/
void ArchiveEntry::unloadData()
{
	// Check there is any data to be 'unloaded'
	if (!data.hasData() || !data_loaded)
		return;

	// Only unload if the data wasn't modified
	if (getState() > 0)
		return;

	// Delete any data
	data.clear();

	// Update variables etc
	setLoaded(false);
}

/* ArchiveEntry::lock
 * Locks the entry. A locked entry cannot be modified
 *******************************************************************/
void ArchiveEntry::lock()
{
	// Lock the entry
	locked = true;

	// Inform parent
	stateChanged();
}

/* ArchiveEntry::unlock
 * Unlocks the entry
 *******************************************************************/
void ArchiveEntry::unlock()
{
	// Unlock the entry
	locked = false;

	// Inform parent
	stateChanged();
}

/* ArchiveEntry::rename
 * Renames the entry
 *******************************************************************/
bool ArchiveEntry::rename(string new_name)
{
	// Check if locked
	if (locked)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Update attributes
	name = new_name;
	upper_name = name.Upper();
	setState(1);

	return true;
}

/* ArchiveEntry::resize
 * Resizes the entry to [new_size]. If [preserve_data] is true, any
 * existing data is preserved
 *******************************************************************/
bool ArchiveEntry::resize(uint32_t new_size, bool preserve_data)
{
	// Check if locked
	if (locked)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Update attributes
	setState(1);

	return data.reSize(new_size, preserve_data);
}

/* ArchiveEntry::clearData
 * Clears entry data and resets its size to zero
 *******************************************************************/
void ArchiveEntry::clearData()
{
	// Check if locked
	if (locked)
	{
		Global::error = "Entry is locked";
		return;
	}

	// Delete the data
	data.clear();

	// Reset attributes
	size = 0;
	data_loaded = false;
}

/* ArchiveEntry::importMem
 * Imports a chunk of memory into the entry, resizing it and clearing
 * any currently existing data.
 * Returns false if data pointer is invalid, true otherwise
 *******************************************************************/
bool ArchiveEntry::importMem(const void* data, uint32_t size)
{
	// Check parameters
	if (!data)
		return false;

	// Check if locked
	if (locked)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Clear any current data
	clearData();

	// Copy data into the entry
	this->data.importMem((const uint8_t*)data, size);

	// Update attributes
	this->size = size;
	setLoaded();
	setType(EntryType::unknownType());
	setState(1);

	return true;
}

/* ArchiveEntry::importMemChunk
 * Imports data from a MemChunk object into the entry, resizing it
 * and clearing any currently existing data.
 * Returns false if the MemChunk has no data, or true otherwise.
 *******************************************************************/
bool ArchiveEntry::importMemChunk(MemChunk& mc)
{
	// Check that the given MemChunk has data
	if (mc.hasData())
	{
		// Copy the data from the MemChunk into the entry
		return importMem(mc.getData(), mc.getSize());
	}
	else
		return false;
}

/* ArchiveEntry::importFile
 * Loads a portion of a file into the entry, overwriting any existing
 * data currently in the entry. A size of 0 means load from the
 * offset to the end of the file.
 * Returns false if the file does not exist or the given offset/size
 * are out of bounds, otherwise returns true.
 *******************************************************************/
bool ArchiveEntry::importFile(string filename, uint32_t offset, uint32_t size)
{
	// Check if locked
	if (locked)
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

/* ArchiveEntry::importFileStream
 * Imports [len] data from [file]
 *******************************************************************/
bool ArchiveEntry::importFileStream(wxFile& file, uint32_t len)
{
	// Check if locked
	if (locked)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Import data from the file stream
	if (data.importFileStream(file, len))
	{
		// Update attributes
		this->size = data.getSize();
		setLoaded();
		setType(EntryType::unknownType());
		setState(1);

		return true;
	}

	return false;
}

/* ArchiveEntry::importEntry
 * Imports data from another entry into this entry, resizing it
 * and clearing any currently existing data.
 * Returns false if the entry is null, true otherwise
 *******************************************************************/
bool ArchiveEntry::importEntry(ArchiveEntry* entry)
{
	// Check if locked
	if (locked)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Check parameters
	if (!entry)
		return false;

	// Copy entry data
	importMem(entry->getData(), entry->getSize());

	return true;
}

/* ArchiveEntry::exportFile
 * Exports entry data to a file.
 * Returns false if file cannot be written, true otherwise
 *******************************************************************/
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
	const uint8_t* data = getData();
	if (data)
		file.Write(data, this->getSize());

	return true;
}

/* ArchiveEntry::write
 * Writes data to the entry MemChunk
 *******************************************************************/
bool ArchiveEntry::write(const void* data, uint32_t size)
{
	// Check if locked
	if (locked)
	{
		Global::error = "Entry is locked";
		return false;
	}

	// Load data if it isn't already
	if (isLoaded())
		getData(true);

	// Perform the write
	if (this->data.write(data, size))
	{
		// Update attributes
		this->size = this->data.getSize();
		setState(1);

		return true;
	}

	return false;
}

/* ArchiveEntry::read
 * Reads data from the entry MemChunk
 *******************************************************************/
bool ArchiveEntry::read(void* buf, uint32_t size)
{
	// Load data if it isn't already
	if (isLoaded())
		getData(true);

	return data.read(buf, size);
}

/* ArchiveEntry::getSizeString
 * Returns the entry's size as a string
 *******************************************************************/
string ArchiveEntry::getSizeString()
{
	return Misc::sizeAsString(getSize());
}

/* ArchiveEntry::stateChanged
 * Notifies the entry's parent archive that the entry has been
 * modified
 *******************************************************************/
void ArchiveEntry::stateChanged()
{
	Archive* parent_archive = getParent();
	if (parent_archive)
		parent_archive->entryStateChanged(this);
}

/* ArchiveEntry::setExtensionByType
 * Sets the entry's name extension to the extension defined in its
 * EntryType
 *******************************************************************/
void ArchiveEntry::setExtensionByType()
{
	// Ignore if the parent archive doesn't support entry name extensions
	if (getParent() && !getParent()->getDesc().names_extensions)
		return;

	// Convert name to wxFileName for processing
	wxFileName fn(name);

	// Set new extension
	fn.SetExt(type->getExtension());

	// Rename
	Archive* parent_archive = getParent();
	if (parent_archive)
		parent_archive->renameEntry(this, fn.GetFullName());
	else
		rename(fn.GetFullName());
}

/* ArchiveEntry::isInNamespace
 * Returns true if the entry is in the [ns] namespace within its
 * parent, false otherwise
 *******************************************************************/
bool ArchiveEntry::isInNamespace(string ns)
{
	// Can't do this without parent archive
	if (!getParent())
		return false;

	// Some special cases first
	if (ns == "graphics" && getParent()->getType() == ARCHIVE_WAD)
		ns = "global";	// Graphics namespace doesn't exist in wad files, use global instead

	return getParent()->detectNamespace(this) == ns;
}
