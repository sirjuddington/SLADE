
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MemChunk.cpp
 * Description: MemChunk class, a simple data structure for
 *              storing/handling arbitrary sized chunks of memory.
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
#include "MemChunk.h"
#include "General/Misc.h"


/*******************************************************************
 * MEMCHUNK CLASS FUNCTIONS
 *******************************************************************/

/* MemChunk::MemChunk
 * MemChunk class constructor
 *******************************************************************/
MemChunk::MemChunk(uint32_t size)
{
	// Init variables
	this->size = size;
	this->cur_ptr = 0;

	// If a size is specified, allocate that much memory
	if (size)
		allocData(size);
	else
		data = NULL;
}

/* MemChunk::MemChunk
 * MemChunk class constructor taking initial data
 *******************************************************************/
MemChunk::MemChunk(const uint8_t* data, uint32_t size)
{
	// Init variables
	this->cur_ptr = 0;
	this->data = NULL;
	this->size = size;

	// Load given data
	importMem(data, size);
}

/* MemChunk::~MemChunk
 * MemChunk class destructor
 *******************************************************************/
MemChunk::~MemChunk()
{
	// Free memory
	if (data)
		delete[] data;
}

/* MemChunk::hasData
 * Returns true if the chunk contains data
 *******************************************************************/
bool MemChunk::hasData()
{
	if (size > 0 && data)
		return true;
	else
		return false;
}

/* MemChunk::clear
 * Deletes the memory chunk.
 * Returns false if no data exists, true otherwise.
 *******************************************************************/
bool MemChunk::clear()
{
	if (hasData())
	{
		delete[] data;
		data = NULL;
		size = 0;
		cur_ptr = 0;
		return true;
	}

	return false;
}

/* MemChunk::reSize
 * Resizes the memory chunk, preserving existing data if specified
 * Returns false if new size is invalid, true otherwise
 *******************************************************************/
bool MemChunk::reSize(uint32_t new_size, bool preserve_data)
{
	// Check for invalid new size
	if (new_size == 0)
	{
		LOG_MESSAGE(1, "MemChunk::reSize: new_size cannot be 0");
		return false;
	}

	// Attempt to allocate memory for new size
	uint8_t* ndata = allocData(new_size, false);
	if (!ndata)
		return false;

	// Preserve existing data if specified
	if (preserve_data)
	{
		memcpy(ndata, data, size * sizeof(uint8_t));
		delete[] data;
		data = ndata;
	}
	else
	{
		clear();
		data = ndata;
	}

	// Update variables
	size = new_size;

	// Check position
	if (cur_ptr > size)
		cur_ptr = size;

	return true;
}

/* MemChunk::importFile
 * Loads a file (or part of it) into the MemChunk
 * Returns false if file couldn't be opened, true otherwise
 *******************************************************************/
bool MemChunk::importFile(string filename, uint32_t offset, uint32_t len)
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
	size = len;

	// Read the file
	if (size > 0)
	{
		//data = new uint8_t[size];
		if (allocData(size))
		{
			// Read the file
			file.Seek(offset, wxFromStart);
			size_t count = file.Read(data, size);
			if (count != size)
			{
				LOG_MESSAGE(1, "MemChunk::importFile: Unable to read full file %s, read %u out of %u",
					filename, count, size);
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

/* MemChunk::importFileStream
 * Loads a file (or part of it) from a currently open file stream
 * into the MemChunk
 * Returns false if file couldn't be opened, true otherwise
 *******************************************************************/
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
	size = len;

	// Read the file
	if (size > 0)
	{
		//data = new uint8_t[size];
		if (allocData(size))
			file.Read(data, size);
		else
			return false;
	}

	return true;
}

/* MemChunk::importMem
 * Loads a chunk of memory into the MemChunk
 * Returns false if size or data pointer is invalid, true otherwise
 *******************************************************************/
bool MemChunk::importMem(const uint8_t* start, uint32_t len)
{
	// Check that length & data to be loaded are valid
	if (!start)
		return false;

	// Clear current data if it exists
	clear();

	// Setup variables
	size = len;

	// Load new data
	if (size > 0)
	{
		if (allocData(size))
			memcpy(data, start, size);
		else
			return false;
	}

	return true;
}

/* MemChunk::exportFile
 * Writes the MemChunk data to a new file of [filename], starting
 * from [start] to [start+size]. If [size] is 0, writes from [start]
 * to the end of the data
 *******************************************************************/
bool MemChunk::exportFile(string filename, uint32_t start, uint32_t size)
{
	// Check data exists
	if (!hasData())
		return false;

	// Check parameters
	if (start >= this->size || start + size >= this->size)
		return false;

	// Check size
	if (size == 0)
		size = this->size - start;

	// Open file for writing
	wxFile file(filename, wxFile::write);
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "Unable to write to file %s", filename);
		Global::error = "Unable to open file for writing";
		return false;
	}

	// Write the data
	file.Write(data+start, size);

	return true;
}

/* MemChunk::exportMemChunk
 * Writes the MemChunk data to another MemChunk, starting from
 * [start] to [start+size]. If [size] is 0, writes from [start] to
 * the end of the data
 *******************************************************************/
bool MemChunk::exportMemChunk(MemChunk& mc, uint32_t start, uint32_t size)
{
	// Check data exists
	if (!hasData())
		return false;

	// Check parameters
	if (start >= this->size || start + size > this->size)
		return false;

	// Check size
	if (size == 0)
		size = this->size - start;

	// Write data to MemChunk
	mc.reSize(size, false);
	return mc.importMem(data+start, size);
}

/* MemChunk::write
 * Writes the given data at the current position. Expands the memory
 * chunk if necessary.
 *******************************************************************/
bool MemChunk::write(const void* data, uint32_t size)
{
	// Check pointers
	if (!data)
		return false;

	// If we're trying to write past the end of the memory chunk,
	// resize it so we can write at this point
	if (cur_ptr + size > this->size)
		reSize(cur_ptr + size, true);

	// Write the data and move to the byte after what was written
	memcpy(this->data + cur_ptr, data, size);
	cur_ptr += size;

	// Success
	return true;
}

/* MemChunk::write
 * Writes the given data at the [start] position. Expands the memory
 * chunk if necessary.
 *******************************************************************/
bool MemChunk::write(const void* data, uint32_t size, uint32_t start)
{
	seek(start, SEEK_SET);
	return write(data, size);
}

/* MemChunk::read
 * Reads data from the current position into [buf]. Returns false if
 * attempting to read data outside of the chunk, true otherwise
 *******************************************************************/
bool MemChunk::read(void* buf, uint32_t size)
{
	// Check pointers
	if (!this->data || !buf)
		return false;

	// If we're trying to read past the end
	// of the memory chunk, return failure
	if (cur_ptr + size > this->size)
		return false;

	// Read the data and move to the byte after what was read
	memcpy(buf, this->data + cur_ptr, size);
	cur_ptr += size;

	return true;
}

/* MemChunk::read
 * Reads [size] bytes of data from [start] into [buf]. Returns false
 * if attempting to read data outside of the chunk, true otherwise
 *******************************************************************/
bool MemChunk::read(void* buf, uint32_t size, uint32_t start)
{
	// Check options
	if (start + size > this->size)
		return false;

	// Do read
	seek(start, SEEK_SET);
	return read(buf, size);
}

/* MemChunk::seek
 * Moves the current position, works the same as fseek() etc.
 *******************************************************************/
bool MemChunk::seek(uint32_t offset, uint32_t start)
{
	if (start == SEEK_CUR)
	{
		// Move forward from the current position
		cur_ptr += offset;
		if (cur_ptr > size)
			cur_ptr = size;
	}
	else if (start == SEEK_SET)
	{
		// Move to the specified offset
		cur_ptr = offset;
		if (cur_ptr > size)
			cur_ptr = size;
	}
	else if (start == SEEK_END)
	{
		// Move to <offset> bytes before the end of the chunk
		if (offset > size)
			cur_ptr = 0;
		else
			cur_ptr = size - offset;
	}

	// Success
	return true;
}

/* MemChunk::readMC
 * Reads [size] bytes of data into [mc]. Returns false if attempting
 * to read outside the chunk, true otherwise
 *******************************************************************/
bool MemChunk::readMC(MemChunk& mc, uint32_t size)
{
	if (cur_ptr + size >= this->size)
		return false;

	if (mc.write(data + cur_ptr, size))
	{
		cur_ptr += size;
		return true;
	}
	else
		return false;
}

/* MemChunk::fillData
 * Overwrites all data bytes with [val] (basically is memset).
 * Returns false if no data exists, true otherwise
 *******************************************************************/
bool MemChunk::fillData(uint8_t val)
{
	// Check data exists
	if (!hasData())
		return false;

	// Fill data with value
	memset(data, val, size);

	// Success
	return true;
}

/* MemChunk::crc
 * Calculates the 32bit CRC value of the data. Returns the CRC or 0
 * if no data is present
 *******************************************************************/
uint32_t MemChunk::crc()
{
	if (hasData())
		return Misc::crc(data, size);
	else
		return 0;
}


/* MemChunk::allocData
 * Allocates [size] bytes of data and returns it, or NULL if the
 * allocation failed. If [set_data] is true, the MemChunk data will
 * also be set to the allocated data if successful, or set to NULL
 * and the size set to 0 if allocation failed.
 *******************************************************************/
uint8_t* MemChunk::allocData(uint32_t size, bool set_data)
{
	uint8_t* ndata = NULL;
	try
	{
		ndata = new uint8_t[size];
	}
	catch (std::bad_alloc& ba)
	{
		LOG_MESSAGE(1, "MemChunk: Allocation of %d bytes failed: %s", size, ba.what());

		if (set_data)
		{
			cur_ptr = 0;
			this->size = 0;
		}

		return NULL;
	}

	if (set_data)
		data = ndata;

	return ndata;
}
