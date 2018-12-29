
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MemChunk.cpp
// Description: MemChunk class, a simple data structure for storing/handling
//              arbitrary sized chunks of memory.
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
#include "MemChunk.h"
#include "General/Misc.h"


// -----------------------------------------------------------------------------
//
// MemChunk Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MemChunk class constructor
// -----------------------------------------------------------------------------
MemChunk::MemChunk(uint32_t size) : size_{ size }
{
	// If a size is specified, allocate that much memory
	if (size)
		allocData(size);
	else
		data_ = nullptr;
}

// -----------------------------------------------------------------------------
// MemChunk class constructor taking initial data
// -----------------------------------------------------------------------------
MemChunk::MemChunk(const uint8_t* data, uint32_t size) : size_{ size }
{
	// Load given data
	importMem(data, size);
}

// -----------------------------------------------------------------------------
// MemChunk class destructor
// -----------------------------------------------------------------------------
MemChunk::~MemChunk()
{
	// Free memory
	delete[] data_;
}

// -----------------------------------------------------------------------------
// Returns true if the chunk contains data
// -----------------------------------------------------------------------------
bool MemChunk::hasData() const
{
	return size_ > 0 && data_;
}

// -----------------------------------------------------------------------------
// Deletes the memory chunk.
// Returns false if no data exists, true otherwise.
// -----------------------------------------------------------------------------
bool MemChunk::clear()
{
	if (hasData())
	{
		delete[] data_;
		data_    = nullptr;
		size_    = 0;
		cur_ptr_ = 0;
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Resizes the memory chunk, preserving existing data if specified.
// Returns false if new size is invalid, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::reSize(uint32_t new_size, bool preserve_data)
{
	// Check for invalid new size
	if (new_size == 0)
	{
		LOG_MESSAGE(1, "MemChunk::reSize: new_size cannot be 0");
		return false;
	}

	// Attempt to allocate memory for new size
	auto ndata = allocData(new_size, false);
	if (!ndata)
		return false;

	// Preserve existing data if specified
	if (preserve_data)
	{
		memcpy(ndata, data_, size_ * sizeof(uint8_t));
		delete[] data_;
		data_ = ndata;
	}
	else
	{
		clear();
		data_ = ndata;
	}

	// Update variables
	size_ = new_size;

	// Check position
	if (cur_ptr_ > size_)
		cur_ptr_ = size_;

	return true;
}

// -----------------------------------------------------------------------------
// Loads a file (or part of it) into the MemChunk.
// Returns false if file couldn't be opened, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::importFile(const string& filename, uint32_t offset, uint32_t len)
{
	// Open the file
	wxFile file(filename);

	// Return false if file open failed
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "MemChunk::importFile: Unable to open file %s", filename);
		Global::error = S_FMT("Unable to open file %s", filename);
		return false;
	}

	// Clear current data if it exists
	clear();

	// If length isn't specified or exceeds the file length,
	// only read to the end of the file
	if (offset + len > file.Length() || len == 0)
		len = file.Length() - offset;

	// Setup variables
	size_ = len;

	// Read the file
	if (size_ > 0)
	{
		// data = new uint8_t[size];
		if (allocData(size_))
		{
			// Read the file
			file.Seek(offset, wxFromStart);
			size_t count = file.Read(data_, size_);
			if (count != size_)
			{
				LOG_MESSAGE(
					1, "MemChunk::importFile: Unable to read full file %s, read %u out of %u", filename, count, size_);
				Global::error = S_FMT("Unable to read file %s", filename);
				clear();
				file.Close();
				return false;
			}
		}

		file.Close();
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads a file (or part of it) from a currently open file stream into memory.
// Returns false if file couldn't be opened, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::importFileStream(wxFile& file, uint32_t len)
{
	// Check file
	if (!file.IsOpened())
		return false;

	// Clear current data if it exists
	clear();

	// Get current file position
	uint32_t offset = file.Tell();

	// If length isn't specified or exceeds the file length,
	// only read to the end of the file
	if (offset + len > file.Length() || len == 0)
		len = file.Length() - offset;

	// Setup variables
	size_ = len;

	// Read the file
	if (size_ > 0)
	{
		// data = new uint8_t[size];
		if (allocData(size_))
			file.Read(data_, size_);
		else
			return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads a chunk of memory into the MemChunk.
// Returns false if size or data pointer is invalid, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::importMem(const uint8_t* start, uint32_t len)
{
	// Check that length & data to be loaded are valid
	if (!start)
		return false;

	// Clear current data if it exists
	clear();

	// Setup variables
	size_ = len;

	// Load new data
	if (size_ > 0)
	{
		if (allocData(size_))
			memcpy(data_, start, size_);
		else
			return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes the MemChunk data to a new file of [filename], starting from [start]
// to [start+size].
// If [size] is 0, writes from [start] to the end of the data
// -----------------------------------------------------------------------------
bool MemChunk::exportFile(const string& filename, uint32_t start, uint32_t size) const
{
	// Check data exists
	if (!hasData())
		return false;

	// Check parameters
	if (start >= this->size_ || start + size >= this->size_)
		return false;

	// Check size
	if (size == 0)
		size = this->size_ - start;

	// Open file for writing
	wxFile file(filename, wxFile::write);
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "Unable to write to file %s", filename);
		Global::error = "Unable to open file for writing";
		return false;
	}

	// Write the data
	file.Write(data_ + start, size);

	return true;
}

// -----------------------------------------------------------------------------
// Writes the MemChunk data to another MemChunk, starting from [start] to
// [start+size].
// If [size] is 0, writes from [start] to the end of the data
// -----------------------------------------------------------------------------
bool MemChunk::exportMemChunk(MemChunk& mc, uint32_t start, uint32_t size) const
{
	// Check data exists
	if (!hasData())
		return false;

	// Check parameters
	if (start >= this->size_ || start + size > this->size_)
		return false;

	// Check size
	if (size == 0)
		size = this->size_ - start;

	// Write data to MemChunk
	mc.reSize(size, false);
	return mc.importMem(data_ + start, size);
}

// -----------------------------------------------------------------------------
// Writes the given data at the current position.
// Expands the memory chunk if necessary
// -----------------------------------------------------------------------------
bool MemChunk::write(const void* data, uint32_t size)
{
	// Check pointers
	if (!data)
		return false;

	// If we're trying to write past the end of the memory chunk,
	// resize it so we can write at this point
	if (cur_ptr_ + size > this->size_)
		reSize(cur_ptr_ + size, true);

	// Write the data and move to the byte after what was written
	memcpy(this->data_ + cur_ptr_, data, size);
	cur_ptr_ += size;

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Writes the given data at the [start] position.
// Expands the memory chunk if necessary
// -----------------------------------------------------------------------------
bool MemChunk::write(const void* data, uint32_t size, uint32_t start)
{
	seek(start, SEEK_SET);
	return write(data, size);
}

// -----------------------------------------------------------------------------
// Reads data from the current position into [buf].
// Returns false if attempting to read data outside of the chunk, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::read(void* buf, uint32_t size)
{
	// Check pointers
	if (!this->data_ || !buf)
		return false;

	// If we're trying to read past the end
	// of the memory chunk, return failure
	if (cur_ptr_ + size > this->size_)
		return false;

	// Read the data and move to the byte after what was read
	memcpy(buf, this->data_ + cur_ptr_, size);
	cur_ptr_ += size;

	return true;
}

// -----------------------------------------------------------------------------
// Reads [size] bytes of data from [start] into [buf].
// Returns false if attempting to read data outside of the chunk, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::read(void* buf, uint32_t size, uint32_t start)
{
	// Check options
	if (start + size > this->size_)
		return false;

	// Do read
	seek(start, SEEK_SET);
	return read(buf, size);
}

// -----------------------------------------------------------------------------
// Moves the current position, works the same as fseek() etc.
// -----------------------------------------------------------------------------
bool MemChunk::seek(uint32_t offset, uint32_t start)
{
	if (start == SEEK_CUR)
	{
		// Move forward from the current position
		cur_ptr_ += offset;
		if (cur_ptr_ > size_)
			cur_ptr_ = size_;
	}
	else if (start == SEEK_SET)
	{
		// Move to the specified offset
		cur_ptr_ = offset;
		if (cur_ptr_ > size_)
			cur_ptr_ = size_;
	}
	else if (start == SEEK_END)
	{
		// Move to <offset> bytes before the end of the chunk
		if (offset > size_)
			cur_ptr_ = 0;
		else
			cur_ptr_ = size_ - offset;
	}

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Reads [size] bytes of data into [mc].
// Returns false if attempting to read outside the chunk, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::readMC(MemChunk& mc, uint32_t size)
{
	if (cur_ptr_ + size >= this->size_)
		return false;

	if (mc.write(data_ + cur_ptr_, size))
	{
		cur_ptr_ += size;
		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Overwrites all data bytes with [val] (basically is memset).
// Returns false if no data exists, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::fillData(uint8_t val) const
{
	// Check data exists
	if (!hasData())
		return false;

	// Fill data with value
	memset(data_, val, size_);

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Calculates the 32bit CRC value of the data.
// Returns the CRC or 0 if no data is present
// -----------------------------------------------------------------------------
uint32_t MemChunk::crc() const
{
	return hasData() ? Misc::crc(data_, size_) : 0;
}


// -----------------------------------------------------------------------------
// Allocates [size] bytes of data and returns it, or null if the allocation
// failed.
// If [set_data] is true, the MemChunk data will also be set to the allocated
// data if successful, or set to null and the size set to 0 if allocation failed
// -----------------------------------------------------------------------------
uint8_t* MemChunk::allocData(uint32_t size, bool set_data)
{
	uint8_t* ndata;
	try
	{
		ndata = new uint8_t[size];
	}
	catch (std::bad_alloc& ba)
	{
		LOG_MESSAGE(1, "MemChunk: Allocation of %d bytes failed: %s", size, ba.what());

		if (set_data)
		{
			cur_ptr_    = 0;
			this->size_ = 0;
		}

		return nullptr;
	}

	if (set_data)
		data_ = ndata;

	return ndata;
}
