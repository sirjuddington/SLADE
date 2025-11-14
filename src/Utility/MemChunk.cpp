
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
#include "FileUtils.h"
#include "General/Misc.h"
#include "UI/WxUtils.h"
#include "thirdparty/xxhash/xxhash.h"
#include <algorithm>

using namespace slade;


// -----------------------------------------------------------------------------
//
// MemChunk Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MemChunk class constructor
// -----------------------------------------------------------------------------
MemChunk::MemChunk(u32 size)
{
	// If a size is specified, allocate that much memory
	if (size > 0)
		data_.resize(size);
}

// -----------------------------------------------------------------------------
// MemChunk class constructor taking initial data
// -----------------------------------------------------------------------------
MemChunk::MemChunk(const u8* data, u32 size)
{
	// Load given data
	if (data && size > 0)
		importMem(data, size);
}

// -----------------------------------------------------------------------------
// Returns true if the chunk contains data
// -----------------------------------------------------------------------------
bool MemChunk::hasData() const
{
	return !data_.empty();
}

// -----------------------------------------------------------------------------
// Deletes the memory chunk.
// Returns false if no data exists, true otherwise.
// -----------------------------------------------------------------------------
bool MemChunk::clear()
{
	if (hasData())
	{
		data_.clear();
		data_.shrink_to_fit();
		cur_ptr_ = 0;
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Resizes the memory chunk, preserving existing data if specified.
// Returns false if new size is invalid, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::reSize(u32 new_size, bool preserve_data)
{
	// Check for invalid new size
	if (new_size == 0)
	{
		log::error("MemChunk::reSize: new_size cannot be 0");
		return false;
	}

	try
	{
		if (preserve_data)
			data_.resize(new_size);
		else
		{
			data_.clear();
			data_.resize(new_size);
		}
	}
	catch (std::bad_alloc& ba)
	{
		log::error("MemChunk::reSize: Allocation of {} bytes failed: {}", new_size, ba.what());
		return false;
	}

	// Check position
	if (cur_ptr_ > data_.size())
		cur_ptr_ = static_cast<u32>(data_.size());

	return true;
}

// -----------------------------------------------------------------------------
// Loads a file (or part of it) into the MemChunk.
// Returns false if file couldn't be opened, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::importFile(string_view filename, u32 offset, u32 len)
{
	// Open the file
	wxFile file(wxutil::strFromView(filename));

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

	// Read the file
	if (len > 0)
	{
		try
		{
			data_.resize(len);
		}
		catch (std::bad_alloc& ba)
		{
			log::error("MemChunk::importFile: Allocation of {} bytes failed: {}", len, ba.what());
			global::error = fmt::format("Unable to allocate memory for file {}", filename);
			clear();
			file.Close();
			return false;
		}

		// Read the file
		file.Seek(offset, wxFromStart);
		size_t count = file.Read(data_.data(), len);
		if (count != len)
		{
			log::error("MemChunk::importFile: Unable to read full file {}, read {} out of {}", filename, count, len);
			global::error = fmt::format("Unable to read file {}", filename);
			clear();
			file.Close();
			return false;
		}

		file.Close();
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads a file (or part of it) from a currently open file stream into memory.
// Returns false if file couldn't be opened, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::importFileStreamWx(wxFile& file, u32 len)
{
	// Check file
	if (!file.IsOpened())
		return false;

	// Clear current data if it exists
	clear();

	// Get current file position
	u32 offset = file.Tell();

	// If length isn't specified or exceeds the file length,
	// only read to the end of the file
	if (offset + len > file.Length() || len == 0)
		len = file.Length() - offset;

	// Read the file
	if (len > 0)
	{
		try
		{
			data_.resize(len);
		}
		catch (std::bad_alloc& ba)
		{
			log::error("MemChunk::importFileStreamWx: Allocation of {} bytes failed: {}", len, ba.what());
			return false;
		}

		file.Read(data_.data(), len);
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
	u32 offset = file.currentPos();

	// If length isn't specified or exceeds the file length,
	// only read to the end of the file
	if (offset + len > file.size() || len == 0)
		len = file.size() - offset;

	// Read the file
	if (len > 0)
	{
		try
		{
			data_.resize(len);
		}
		catch (std::bad_alloc& ba)
		{
			log::error("MemChunk::importFileStream: Allocation of {} bytes failed: {}", len, ba.what());
			return false;
		}

		file.read(data_.data(), len);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads a chunk of memory into the MemChunk.
// Returns false if size or data pointer is invalid, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::importMem(const u8* start, u32 len)
{
	// Check that data to be loaded is valid
	if (!start)
		return false;

	// Clear current data if it exists
	clear();

	// Load new data
	if (len > 0)
	{
		try
		{
			data_.assign(start, start + len);
		}
		catch (std::bad_alloc& ba)
		{
			log::error("MemChunk::importMem: Allocation of {} bytes failed: {}", len, ba.what());
			return false;
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Writes the MemChunk data to a new file of [filename], starting from [start]
// to [start+size].
// If [size] is 0, writes from [start] to the end of the data
// -----------------------------------------------------------------------------
bool MemChunk::exportFile(string_view filename, u32 start, u32 size) const
{
	// Check data exists
	if (!hasData())
		return false;

	// Check parameters
	if (start >= data_.size() || start + size > data_.size())
		return false;

	// Check size
	if (size == 0)
		size = static_cast<u32>(data_.size()) - start;

	// Open file for writing
	wxFile file(wxutil::strFromView(filename), wxFile::write);
	if (!file.IsOpened())
	{
		log::error("Unable to write to file {}", filename);
		global::error = "Unable to open file for writing";
		return false;
	}

	// Write the data
	file.Write(data_.data() + start, size);

	return true;
}

// -----------------------------------------------------------------------------
// Writes the MemChunk data to another MemChunk, starting from [start] to
// [start+size].
// If [size] is 0, writes from [start] to the end of the data
// -----------------------------------------------------------------------------
bool MemChunk::exportMemChunk(MemChunk& mc, u32 start, u32 size) const
{
	// Check data exists
	if (!hasData())
		return false;

	// Check parameters
	if (start >= data_.size() || start + size > data_.size())
		return false;

	// Check size
	if (size == 0)
		size = static_cast<u32>(data_.size()) - start;

	// Write data to MemChunk
	mc.reSize(size, false);
	return mc.importMem(data_.data() + start, size);
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
	if (offset + size > data_.size())
	{
		if (expand)
			reSize(offset + size, true);
		else
			return false;
	}

	// Write the data
	memcpy(data_.data() + offset, data, size);

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
	if (data_.empty() || !buf)
		return false;

	// If we're trying to read past the end
	// of the memory chunk, return failure
	if (offset + size > data_.size())
		return false;

	// Read the data
	memcpy(buf, data_.data() + offset, size);

	return true;
}

// -----------------------------------------------------------------------------
// Writes [count] bytes from the given data [buffer] at the current position.
// Expands the memory chunk if necessary
// -----------------------------------------------------------------------------
bool MemChunk::write(const void* buffer, u32 count)
{
	// Check pointers
	if (!buffer)
		return false;

	// If we're trying to write past the end of the memory chunk,
	// resize it so we can write at this point
	if (cur_ptr_ + count > data_.size())
		reSize(cur_ptr_ + count, true);

	// Write the data and move to the byte after what was written
	memcpy(data_.data() + cur_ptr_, buffer, count);
	cur_ptr_ += count;

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Writes the given data at the [start] position.
// Expands the memory chunk if necessary
// -----------------------------------------------------------------------------
bool MemChunk::write(const void* data, u32 size, u32 start)
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
	if (data_.empty() || !buffer)
		return false;

	// If we're trying to read past the end
	// of the memory chunk, return failure
	if (cur_ptr_ + count > data_.size())
		return false;

	// Read the data and move to the byte after what was read
	memcpy(buffer, data_.data() + cur_ptr_, count);
	cur_ptr_ += count;

	return true;
}

// -----------------------------------------------------------------------------
// Reads [size] bytes of data from [start] into [buf].
// Returns false if attempting to read data outside of the chunk, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::read(void* buf, u32 size, u32 start) const
{
	// Check options
	if (start + size > data_.size())
		return false;

	// Do read
	seek(start, SEEK_SET);
	return read(buf, size);
}

// -----------------------------------------------------------------------------
// Moves the current position, works the same as fseek() etc.
// -----------------------------------------------------------------------------
bool MemChunk::seek(u32 offset, u32 start) const
{
	auto size = static_cast<u32>(data_.size());

	if (start == SEEK_CUR)
	{
		// Move forward from the current position
		cur_ptr_ += offset;
		cur_ptr_ = std::min(cur_ptr_, size);
	}
	else if (start == SEEK_SET)
	{
		// Move to the specified offset
		cur_ptr_ = offset;
		cur_ptr_ = std::min(cur_ptr_, size);
	}
	else if (start == SEEK_END)
	{
		// Move to <offset> bytes before the end of the chunk
		if (offset > size)
			cur_ptr_ = 0;
		else
			cur_ptr_ = size - offset;
	}

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Reads [size] bytes of data into [mc].
// Returns false if attempting to read outside the chunk, true otherwise
// -----------------------------------------------------------------------------
bool MemChunk::readMC(MemChunk& mc, u32 size) const
{
	if (cur_ptr_ + size > data_.size())
		return false;

	if (mc.write(data_.data() + cur_ptr_, size))
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
bool MemChunk::fillData(u8 val)
{
	// Check data exists
	if (!hasData())
		return false;

	// Fill data with value
	std::ranges::fill(data_, val);

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Calculates the 32bit CRC value of the data.
// Returns the CRC or 0 if no data is present
// -----------------------------------------------------------------------------
u32 MemChunk::crc() const
{
	return hasData() ? misc::crc(data_.data(), static_cast<u32>(data_.size())) : 0;
}

// -----------------------------------------------------------------------------
// Calculates a 128-bit hash of the data using xxHash (XXH128).
// Returns the hash as a hex string or empty if no data is present
// -----------------------------------------------------------------------------
string MemChunk::hash() const
{
	if (!hasData())
		return {};

	auto hash = XXH3_128bits(data_.data(), data_.size());
	return fmt::format("{:x}{:x}", hash.high64, hash.low64);
}

// -----------------------------------------------------------------------------
// Returns the data as a string
// -----------------------------------------------------------------------------
string MemChunk::asString(u32 offset, u32 length) const
{
	if (offset >= data_.size())
		offset = 0;
	if (length == 0 || offset + length > data_.size())
		length = static_cast<u32>(data_.size()) - offset;

	string s;
	s.assign(reinterpret_cast<const char*>(data_.data()) + offset, length);
	return s;
}

// -----------------------------------------------------------------------------
// 'Releases' the MemChunk's data, returning a pointer to it and resetting the
// MemChunk itself.
// !! Don't use this unless absolutely necessary - the pointer returned must be
//    deleted[] elsewhere or a memory leak will occur
// -----------------------------------------------------------------------------
u8* MemChunk::releaseData()
{
	// Allocate new memory and copy data
	auto size = data_.size();
	if (size == 0)
	{
		cur_ptr_ = 0;
		return nullptr;
	}

	u8* ptr = nullptr;
	try
	{
		ptr = new u8[size];
		memcpy(ptr, data_.data(), size);
	}
	catch (std::bad_alloc& ba)
	{
		log::error("MemChunk::releaseData: Allocation of {} bytes failed: {}", size, ba.what());
		return nullptr;
	}

	// Clear the vector
	data_.clear();
	data_.shrink_to_fit();
	cur_ptr_ = 0;

	return ptr;
}
