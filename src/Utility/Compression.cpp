
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Compression.cpp
 * Description: Many functions to encapsulate compression and
 *              decompression streams into MemChunk operations.
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
#include "Compression.h"
#include "External/zreaders/files.h"


/*******************************************************************
 * FUNCTIONS
 *******************************************************************/


/* Compression::GenericInflate
 * Inflates the content of <in> to <out>
 *******************************************************************/
#define CHUNK 4096
bool Compression::GenericInflate(MemChunk& in, MemChunk& out, int windowbits, const char* function)
{
	in.seek(0, SEEK_SET);
	out.clear();
	MemoryReader source(in);
	FileReaderZ stream(source, windowbits);
	uint8_t buffer[CHUNK];
	size_t gotten;
	do
	{
		gotten = stream.Read(buffer, CHUNK);
		if (gotten > 0) out.write(buffer, gotten);
	}
	while (gotten == CHUNK && stream.Status == Z_OK);
	return (stream.Status == Z_OK || stream.Status == Z_STREAM_END);
}

/* Compression::GenericDeflate
 * Basically a copy of zpipe.
 * Deflates the content of <in> to <out>
 *******************************************************************/
bool Compression::GenericDeflate(MemChunk& in, MemChunk& out, int level, int windowbits, const char* function)
{
	in.seek(0, SEEK_SET);
	out.clear();

	int ret, flush;
	unsigned have;
	z_stream strm;
	unsigned char bin[CHUNK];
	unsigned char bout[CHUNK];

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	if (windowbits == 0) ret = deflateInit(&strm, level);
	else ret = deflateInit2(&strm, level, Z_DEFLATED, windowbits, 9, Z_DEFAULT_STRATEGY);
	if (ret != Z_OK)
	{
		LOG_MESSAGE(1, "%s init error %i: %s", function, ret, strm.msg);
		return false;
	}

	/* compress until end of file */
	do
	{
		// Find out how much stream remains
		have = in.getSize() - in.currentPos();
		// keep it in bite-sized chunks
		if (have > CHUNK)
		{
			have = CHUNK;
			flush = Z_NO_FLUSH;
		}
		else flush = Z_FINISH;
		in.read(bin, have);

		strm.avail_in = have;
		strm.next_in = bin;

		/*  run deflate() on input until output buffer not full, finish
			compression if all of source has been read in */
		do
		{
			strm.avail_out = CHUNK;
			strm.next_out = bout;
			ret = deflate(&strm, flush);    /* no bad return value */
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			have = CHUNK - strm.avail_out;
			out.write(bout, have);
		}
		while (strm.avail_out == 0);
		assert(strm.avail_in == 0);     /* all input will be used */
	}
	while (flush != Z_FINISH);		/* done when last data in file processed */
	assert(ret == Z_STREAM_END);        /* stream will be complete */

	/* clean up and return */
	deflateEnd(&strm);
	return true;
}

/* Compression::ZipInflate
 * Inflates the content of <in> as a zip stream to <out>
 * ZIP streams use a windowbits size of MAX_WBITS (15)
 * The value is inverted to signify wrapping should be used.
 *******************************************************************/
bool Compression::ZipInflate(MemChunk& in, MemChunk& out, size_t maxsize)
{
	bool ret = Compression::GenericInflate(in, out, -MAX_WBITS, "ZipInflate");

	if (maxsize && out.getSize() != maxsize)
		LOG_MESSAGE(1, "Zip stream inflated to %d, expected %d", out.getSize(), maxsize);

	return ret;
}

/* Compression::GZipInflate
 * Deflates the content of <in> as a gzip stream to <out>
 * GZip streams use a windowbits size of MAX_WBITS (15)
 * The value is inverted to tell it shouldn't use headers or footers
 *******************************************************************/
bool Compression::ZipDeflate(MemChunk& in, MemChunk& out, int level)
{
	return Compression::GenericDeflate(in, out, level, -MAX_WBITS, "ZipDeflate");
}

/* Compression::GZipInflate
 * Inflates the content of <in> as a gzip stream to <out>
 * GZip streams use a windowbits size of MAX_WBITS (15)
 * The +16 tells zlib to look out for a gzip header
 *******************************************************************/
bool Compression::GZipInflate(MemChunk& in, MemChunk& out, size_t maxsize)
{
	bool ret = Compression::GenericInflate(in, out, 16 + MAX_WBITS, "GZipInflate");

	if (maxsize && out.getSize() != maxsize)
		LOG_MESSAGE(1, "Zip stream inflated to %d, expected %d", out.getSize(), maxsize);

	return ret;
}

/* Compression::GZipInflate
 * Deflates the content of <in> as a gzip stream to <out>
 * GZip streams use a windowbits size of MAX_WBITS (15)
 * The +16 tells zlib to use a gzip header
 *******************************************************************/
bool Compression::GZipDeflate(MemChunk& in, MemChunk& out, int level)
{
	return Compression::GenericDeflate(in, out, level, 16 + MAX_WBITS, "GZipDeflate");
}

/* Compression::ZlibInflate
 * Inflates the content of <in> as a zlib stream to <out>
 * Zlip streams use the default value, which is actually the same
 * as MAX_WBITS as well, but the function used for initialization
 * is different so we use 0 here instead.
 *******************************************************************/
bool Compression::ZlibInflate(MemChunk& in, MemChunk& out, size_t maxsize)
{
	bool ret = Compression::GenericInflate(in, out, 0, "ZlibInflate");

	if (maxsize && out.getSize() != maxsize)
		LOG_MESSAGE(1, "Zlib stream inflated to %d, expected %d", out.getSize(), maxsize);

	return ret;
}

/* Compression::ZlibDeflate
 * Deflates the content of <in> as a zlib stream to <out>
 *******************************************************************/
bool Compression::ZlibDeflate(MemChunk& in, MemChunk& out, int level)
{
	return Compression::GenericDeflate(in, out, level, 0, "ZlibDeflate");
}

/* Compression::BZip2Decompress
 * Decompress the content of <in> as a bzip2 stream to <out>
 *******************************************************************/
bool Compression::BZip2Decompress(MemChunk& in, MemChunk& out, size_t maxsize)
{
	in.seek(0, SEEK_SET);
	out.clear();

	MemoryReader source(in);
	FileReaderBZ2 stream(source);
	uint8_t buffer[4096];
	size_t gotten;
	do
	{
		gotten = stream.Read(buffer, 4096);
		if (gotten > 0) out.write(buffer, gotten);
	}
	while (gotten == 4096 && stream.Status == BZ_OK);

	if (maxsize && out.getSize() != maxsize)
		LOG_MESSAGE(1, "bzip2 stream inflated to %d, expected %d", out.getSize(), maxsize);

	return (stream.Status == BZ_OK || stream.Status == BZ_STREAM_END);
}

/* Compression::BZip2Decompress
 * Compress the content of <in> as a bzip2 stream to <out>
 *******************************************************************/
bool Compression::BZip2Compress(MemChunk& in, MemChunk& out)
{
	// Clear out
	out.clear();

	// Allocate a work buffer big enough to guarantee success
	unsigned bufferlen = in.getSize() + (in.getSize()>>6) + 1024;
	char* buffer = new char[bufferlen];
	int ok = -1;
	if (buffer)
	{
		ok = BZ2_bzBuffToBuffCompress(buffer, &bufferlen,
		                              // How ridiculous is it that casting is needed here?
		                              (const char*)in.getData(), in.getSize(), 9, 0, 0);
		out.write(buffer, bufferlen);
		delete[] buffer;
	}
	return (ok == BZ_OK);
}

/* Compression::LZMADecompress
 * Decompress the content of <in> as an LZMA stream to <out>
 *******************************************************************/
bool Compression::LZMADecompress(MemChunk& in, MemChunk& out, size_t size)
{
	in.seek(0, SEEK_SET);
	out.clear();

	MemoryReader source(in);
	FileReaderLZMA stream(source, size, true);
	uint8_t* cache = new uint8_t[size];
	if (stream.Read(cache, size))
	{
		out.write(cache, size);
		delete[] cache;
		return true;
	}
	delete[] cache;
	return false;
}

