
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Compression.cpp
// Description: Many functions to encapsulate compression and decompression
//              streams into MemChunk operations.
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
#include "Compression.h"
#include "ZReaders/files.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Compression Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Inflates the content of [in] to [out]
// -----------------------------------------------------------------------------
#define CHUNK 4096
bool compression::genericInflate(const MemChunk& in, MemChunk& out, int windowbits, const char* function)
{
	in.seek(0, SEEK_SET);
	out.clear();
	MemoryReader source(in);
	FileReaderZ  stream(source, windowbits);
	uint8_t      buffer[CHUNK];
	size_t       gotten;
	do
	{
		gotten = stream.Read(buffer, CHUNK);
		if (gotten > 0)
			out.write(buffer, gotten);
	} while (gotten == CHUNK && stream.Status == Z_OK);
	return (stream.Status == Z_OK || stream.Status == Z_STREAM_END);
}

// -----------------------------------------------------------------------------
// Basically a copy of zpipe.
// Deflates the content of [in] to [out]
// -----------------------------------------------------------------------------
bool compression::genericDeflate(const MemChunk& in, MemChunk& out, int level, int windowbits, const char* function)
{
	in.seek(0, SEEK_SET);
	out.clear();

	int           ret, flush;
	unsigned      have;
	z_stream      strm;
	unsigned char bin[CHUNK];
	unsigned char bout[CHUNK];

	/* allocate deflate state */
	strm.zalloc = nullptr;
	strm.zfree  = nullptr;
	strm.opaque = nullptr;
	if (windowbits == 0)
		ret = deflateInit(&strm, level);
	else
		ret = deflateInit2(&strm, level, Z_DEFLATED, windowbits, 9, Z_DEFAULT_STRATEGY);
	if (ret != Z_OK)
	{
		log::error("{} init error {}: {}", function, ret, strm.msg);
		return false;
	}

	/* compress until end of file */
	do
	{
		// Find out how much stream remains
		have = in.size() - in.currentPos();
		// keep it in bite-sized chunks
		if (have > CHUNK)
		{
			have  = CHUNK;
			flush = Z_NO_FLUSH;
		}
		else
			flush = Z_FINISH;
		in.read(bin, have);

		strm.avail_in = have;
		strm.next_in  = bin;

		/*  run deflate() on input until output buffer not full, finish
			compression if all of source has been read in */
		do
		{
			strm.avail_out = CHUNK;
			strm.next_out  = bout;
			ret            = deflate(&strm, flush); /* no bad return value */
			assert(ret != Z_STREAM_ERROR);          /* state not clobbered */
			have = CHUNK - strm.avail_out;
			out.write(bout, have);
		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0); /* all input will be used */
	} while (flush != Z_FINISH); /* done when last data in file processed */
	assert(ret == Z_STREAM_END); /* stream will be complete */

	/* clean up and return */
	deflateEnd(&strm);
	return true;
}

// -----------------------------------------------------------------------------
// Inflates the content of [in] as a zip stream to [out].
// ZIP streams use a windowbits size of MAX_WBITS (15).
// The value is inverted to signify wrapping should be used.
// -----------------------------------------------------------------------------
bool compression::zipInflate(const MemChunk& in, MemChunk& out, size_t maxsize)
{
	bool ret = compression::genericInflate(in, out, -MAX_WBITS, "ZipInflate");

	if (maxsize && out.size() != maxsize)
		log::warning("Zip stream inflated to {}, expected {}", out.size(), maxsize);

	return ret;
}

// -----------------------------------------------------------------------------
// Deflates the content of [in] as a gzip stream to [out].
// GZip streams use a windowbits size of MAX_WBITS (15).
// The value is inverted to tell it shouldn't use headers or footers
// -----------------------------------------------------------------------------
bool compression::zipDeflate(const MemChunk& in, MemChunk& out, int level)
{
	return compression::genericDeflate(in, out, level, -MAX_WBITS, "ZipDeflate");
}

// -----------------------------------------------------------------------------
// Inflates the content of [in] as a gzip stream to [out].
// GZip streams use a windowbits size of MAX_WBITS (15).
// The +16 tells zlib to look out for a gzip header
// -----------------------------------------------------------------------------
bool compression::gzipInflate(const MemChunk& in, MemChunk& out, size_t maxsize)
{
	bool ret = compression::genericInflate(in, out, 16 + MAX_WBITS, "GZipInflate");

	if (maxsize && out.size() != maxsize)
		log::warning("Zip stream inflated to {}, expected {}", out.size(), maxsize);

	return ret;
}

// -----------------------------------------------------------------------------
// Deflates the content of [in] as a gzip stream to [out].
// GZip streams use a windowbits size of MAX_WBITS (15).
// The +16 tells zlib to use a gzip header
// -----------------------------------------------------------------------------
bool compression::gzipDeflate(const MemChunk& in, MemChunk& out, int level)
{
	return compression::genericDeflate(in, out, level, 16 + MAX_WBITS, "GZipDeflate");
}

// -----------------------------------------------------------------------------
// Inflates the content of [in] as a zlib stream to [out].
// Zlip streams use the default value, which is actually the same as MAX_WBITS
// as well, but the function used for initialization is different so we use 0
// here instead.
// -----------------------------------------------------------------------------
bool compression::zlibInflate(const MemChunk& in, MemChunk& out, size_t maxsize)
{
	bool ret = compression::genericInflate(in, out, 0, "ZlibInflate");

	if (maxsize && out.size() != maxsize)
		log::warning("Zlib stream inflated to {}, expected {}", out.size(), maxsize);

	return ret;
}

// -----------------------------------------------------------------------------
// Deflates the content of [in] as a zlib stream to [out]
// -----------------------------------------------------------------------------
bool compression::zlibDeflate(const MemChunk& in, MemChunk& out, int level)
{
	return compression::genericDeflate(in, out, level, 0, "ZlibDeflate");
}

// -----------------------------------------------------------------------------
// Decompress the content of [in] as a bzip2 stream to [out]
// -----------------------------------------------------------------------------
bool compression::bzip2Decompress(const MemChunk& in, MemChunk& out, size_t maxsize)
{
	in.seek(0, SEEK_SET);
	out.clear();

	MemoryReader  source(in);
	FileReaderBZ2 stream(source);
	uint8_t       buffer[4096];
	size_t        gotten;
	do
	{
		gotten = stream.Read(buffer, 4096);
		if (gotten > 0)
			out.write(buffer, gotten);
	} while (gotten == 4096 && stream.Status == BZ_OK);

	if (maxsize && out.size() != maxsize)
		log::warning("bzip2 stream inflated to {}, expected {}", out.size(), maxsize);

	return (stream.Status == BZ_OK || stream.Status == BZ_STREAM_END);
}

// -----------------------------------------------------------------------------
// Compress the content of [in] as a bzip2 stream to [out]
// -----------------------------------------------------------------------------
bool compression::bzip2Compress(const MemChunk& in, MemChunk& out)
{
	// Clear out
	out.clear();

	// Allocate a work buffer big enough to guarantee success
	unsigned bufferlen = in.size() + (in.size() >> 6) + 1024;
	char*    buffer    = new char[bufferlen];
	int      ok        = -1;
	if (buffer)
	{
		// Ugly workaround to get const memchunk data as non-const char* required by the function below
		auto c_data = reinterpret_cast<const char*>(in.data());
		auto data   = const_cast<char*>(c_data);

		ok = BZ2_bzBuffToBuffCompress(buffer, &bufferlen, data, in.size(), 9, 0, 0);
		out.write(buffer, bufferlen);
		delete[] buffer;
	}
	return (ok == BZ_OK);
}

// -----------------------------------------------------------------------------
// Decompress the content of [in] as an LZMA stream to [out]
// -----------------------------------------------------------------------------
bool compression::lzmaDecompress(const MemChunk& in, MemChunk& out, size_t size)
{
	in.seek(0, SEEK_SET);
	out.clear();

	MemoryReader   source(in);
	FileReaderLZMA stream(source, size, true);
	uint8_t*       cache = new uint8_t[size];
	if (stream.Read(cache, size))
	{
		out.write(cache, size);
		delete[] cache;
		return true;
	}
	delete[] cache;
	return false;
}
