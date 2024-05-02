
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "FileUtils.h"
#include "General/Misc.h"
#include "MD5.h"
#include <xxhash.h>

using namespace slade;


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
// MemChunk class copy constructor
// -----------------------------------------------------------------------------
MemChunk::MemChunk(const MemChunk& copy) : cur_ptr_{ copy.cur_ptr_ }, size_{ copy.size_ }
{
	data_ = allocData(size_, false);

	if (data_)
		memcpy(data_, copy.data_, size_);
	else
	{
		size_    = 0;
		cur_ptr_ = 0;
	}
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
		log::error("MemChunk::reSize: new_size cannot be 0");
		return false;
	}

	// Attempt to allocate memory for new size
	auto ndata = allocData(new_size, false);
	if (!ndata)
		return false;

	// Preserve existing data if specified
	if (!preserve_data)
	{
		clear();
		data_ = ndata;
	}
	else if (data_ != nullptr)
	{
		memcpy(ndata, data_, size_ * sizeof(uint8_t));
		delete[] data_;
		data_ = ndata;
	}
	else
	{
		memset(ndata, 0, size_ * sizeof(uint8_t));
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
bool MemChunk::importFile(string_view filename, uint32_t offset, uint32_t len)
{
	// Open the file
	wxFile file(wxString{ filename.data(), filename.size() });

	// Return false if file open failed
	if (!file.IsOpened())
	{
		log::error("MemChunk::importFile: Unable to open file {}", filename);
		global::error = fmt::format("Unable to open file {}", filename);
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
				log::error(
					"MemChunk::importFile: Unable to read full file {}, read {} out of {}", filename, count, size_);
				global::error = fmt::format("Unable to read file {}", filename);
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
bool MemChunk::importFileStreamWx(wxFile& file, uint32_t len)
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

bool MemChunk::importFileStream(const SFile& file, unsigned len)
{
	// Check file
	if (!file.isOpen())
		return false;

	// Clear current data if it exists
	clear();

	// Get current file position
	uint32_t offset = file.currentPos();

	// If length isn't specified or exceeds the file length,
	// only read to the end of the file
	if (offset + len > file.size() || len == 0)
		len = file.size() - offset;

	// Setup variables
	size_ = len;

	// Read the file
	if (size_ > 0)
	{
		if (allocData(size_))
			file.read(data_, size_);
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
bool MemChunk::exportFile(string_view filename, uint32_t start, uint32_t size) const
{
	// Check data exists
	if (!hasData())
		return false;

	// Check parameters
	if (start >= size_ || start + size >= size_)
		return false;

	// Check size
	if (size == 0)
		size = size_ - start;

	// Open file for writing
	wxFile file(wxString{ filename.data(), filename.size() }, wxFile::write);
	if (!file.IsOpened())
	{
		log::error("Unable to write to file {}", filename);
		global::error = "Unable to open file for writing";
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
	if (start >= size_ || start + size > size_)
		return false;

	// Check size
	if (size == 0)
		size = size_ - start;

	// Write data to MemChunk
	mc.reSize(size, false);
	return mc.importMem(data_ + start, size);
}

// -----------------------------------------------------------------------------
// Writes the given data at [offset].
// If [expand] is true, expands the memory chunk if necessary
// -----------------------------------------------------------------------------
bool MemChunk::write(unsigned offset, const void* data, unsigned size, bool expand)
{
	// Check pointers
	if (!data)
		return false;

	// If we're trying to write past the end of the memory chunk,
	// resize it so we can write at this point
	// (or return false if expanding is disallowed)
	if (offset + size > size_)
	{
		if (expand)
			reSize(offset + size, true);
		else
			return false;
	}

	// Write the data
	memcpy(data_ + offset, data, size);

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Reads data from [offset] to [offset]+[size] into [buf].
// Returns false if attempting to read data outside of the chunk, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::read(unsigned offset, void* buf, unsigned size) const
{
	// Check pointers
	if (!data_ || !buf)
		return false;

	// If we're trying to read past the end
	// of the memory chunk, return failure
	if (offset + size > size_)
		return false;

	// Read the data and move to the byte after what was read
	memcpy(buf, data_ + offset, size);

	return true;
}

// -----------------------------------------------------------------------------
// Writes [count] bytes from the given data [buffer] at the current position.
// Expands the memory chunk if necessary
// -----------------------------------------------------------------------------
bool MemChunk::write(const void* buffer, uint32_t count)
{
	// Check pointers
	if (!buffer)
		return false;

	// If we're trying to write past the end of the memory chunk,
	// resize it so we can write at this point
	if (cur_ptr_ + count > size_)
		reSize(cur_ptr_ + count, true);

	// Write the data and move to the byte after what was written
	memcpy(data_ + cur_ptr_, buffer, count);
	cur_ptr_ += count;

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
// Reads [count] bytes of data from the current position into [buffer].
// Returns false if attempting to read data outside of the chunk, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::read(void* buffer, unsigned count) const
{
	// Check pointers
	if (!data_ || !buffer)
		return false;

	// If we're trying to read past the end
	// of the memory chunk, return failure
	if (cur_ptr_ + count > size_)
		return false;

	// Read the data and move to the byte after what was read
	memcpy(buffer, data_ + cur_ptr_, count);
	cur_ptr_ += count;

	return true;
}

// -----------------------------------------------------------------------------
// Reads [size] bytes of data from [start] into [buf].
// Returns false if attempting to read data outside of the chunk, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::read(void* buf, uint32_t size, uint32_t start) const
{
	// Check options
	if (start + size > size_)
		return false;

	// Do read
	seek(start, SEEK_SET);
	return read(buf, size);
}

// -----------------------------------------------------------------------------
// Moves the current position, works the same as fseek() etc.
// -----------------------------------------------------------------------------
bool MemChunk::seek(uint32_t offset, uint32_t start) const
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
bool MemChunk::readMC(MemChunk& mc, uint32_t size) const
{
	if (cur_ptr_ + size >= size_)
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
	return hasData() ? misc::crc(data_, size_) : 0;
}

// -----------------------------------------------------------------------------
// Returns the data as a string
// -----------------------------------------------------------------------------
string MemChunk::asString(uint32_t offset, uint32_t length) const
{
	if (offset >= size_)
		offset = 0;
	if (length == 0 || offset + length > size_)
		length = size_ - offset;

	string s;
	s.assign(reinterpret_cast<char*>(data_) + offset, length);
	return s;
}

// -----------------------------------------------------------------------------
// Calculates the MD5 hash of the data.
// Returns the MD5 hash or an empty string if no data is present
// -----------------------------------------------------------------------------
string MemChunk::md5() const
{
	if (!hasData())
		return {};

	MD5 md5(*this);
	return md5.hexdigest();
}

// -----------------------------------------------------------------------------
// Calculates a 128-bit hash of the data using xxHash (XXH128).
// Returns the hash as a hex string or empty if no data is present
// -----------------------------------------------------------------------------
string MemChunk::hash() const
{
	if (!hasData())
		return {};

	auto hash = XXH3_128bits(data_, size_);
	return fmt::format("{:x}{:x}", hash.high64, hash.low64);
}

// -----------------------------------------------------------------------------
// 'Releases' the MemChunk's data, returning a pointer to it and resetting the
// MemChunk itself.
// !! Don't use this unless absolutely necessary - the pointer returned must be
//    deleted elsewhere or a memory leak will occur
// -----------------------------------------------------------------------------
uint8_t* MemChunk::releaseData()
{
	auto data = data_;

	data_    = nullptr;
	size_    = 0;
	cur_ptr_ = 0;

	return data;
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
		log::error("MemChunk: Allocation of {} bytes failed: {}", size, ba.what());

		if (set_data)
		{
			cur_ptr_ = 0;
			size_    = 0;
		}

		return nullptr;
	}

	if (set_data)
		data_ = ndata;

	return ndata;
}
