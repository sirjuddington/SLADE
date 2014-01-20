
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Conversions.cpp
 * Description: Functions to perform various data type conversions
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
#include "Archive.h"
#include "Conversions.h"
#include "ArchiveEntry.h"
#include "mus2mid/mus2mid.h"
#include "zreaders/i_music.h"

/*******************************************************************
 * STRUCTS
 *******************************************************************/
// Some structs for wav conversion
struct wav_chunk_t
{
	char id[4];
	uint32_t size;
};

struct wav_fmtchunk_t
{
	wav_chunk_t header;
	uint16_t tag;
	uint16_t channels;
	uint32_t samplerate;
	uint32_t datarate;
	uint16_t blocksize;
	uint16_t bps;
};

// For doom sound conversion
struct dsnd_header_t
{
	uint16_t three;
	uint16_t samplerate;
	uint32_t samples;
};

// For Jaguar doom sound conversion
struct jsnd_header_t
{
	uint32_t samples;
	uint32_t loopstart;
	uint32_t loopend;
	uint32_t flags;
	uint32_t unity;
	uint32_t pitch;
	uint32_t decay;
};

/*******************************************************************
 * FUNCTIONS
 *******************************************************************/

/* Conversions::doomSndToWav
 * Converts doom sound data [in] to wav format, written to [out]
 *******************************************************************/
bool Conversions::doomSndToWav(MemChunk& in, MemChunk& out)
{
	// --- Read Doom sound ---

	// Read doom sound header
	dsnd_header_t header;
	in.seek(0, SEEK_SET);
	in.read(&header, 8);

	// Some sounds created on Mac platforms have their identifier and samplerate in BE format.
	// Curiously, the number of samples is still in LE format.
	header.three = wxUINT16_SWAP_ON_BE(header.three);
	header.samplerate = wxUINT16_SWAP_ON_BE(header.samplerate);
	header.samples = wxUINT32_SWAP_ON_BE(header.samples);
	if (header.three == 0x300)
		header.samplerate = wxUINT16_SWAP_ALWAYS(header.samplerate);

	// Format checks
	if (header.three != 3 && header.three != 0x300)  	// Check for magic number
	{
		Global::error = "Invalid Doom Sound";
		return false;
	}
	if (header.samples > (in.getSize() - 8) || header.samples <= 4)  	// Check for sane values
	{
		Global::error = "Invalid Doom Sound";
		return false;
	}

	// Read samples
	uint8_t* samples = new uint8_t[header.samples];
	in.read(samples, header.samples);


	// --- Write WAV ---

	wav_chunk_t whdr, wdhdr;
	wav_fmtchunk_t fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = header.samples;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag = 1;
	fmtchunk.channels = 1;
	fmtchunk.samplerate = header.samplerate;
	fmtchunk.datarate = header.samplerate;
	fmtchunk.blocksize = 1;
	fmtchunk.bps = 8;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 8;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(wav_fmtchunk_t));
	out.write(&wdhdr, 8);
	out.write(samples, header.samples);

	// Ensure data ends on even byte boundary
	if (header.samples % 2 != 0)
		out.write("\0", 1);

	return true;
}

/* Conversions::wavToDoomSnd
 * Converts wav data [in] to doom sound format, written to [out]
 *******************************************************************/
bool Conversions::wavToDoomSnd(MemChunk& in, MemChunk& out)
{
	// --- Read WAV ---
	wav_chunk_t chunk;

	// Read header
	in.seek(0, SEEK_SET);
	in.read(&chunk, 8);

	// Check header
	if (chunk.id[0] != 'R' || chunk.id[1] != 'I' || chunk.id[2] != 'F' || chunk.id[3] != 'F')
	{
		Global::error = "Invalid WAV";
		return false;
	}

	// Read format
	char format[4];
	in.read(format, 4);

	// Check format
	if (format[0] != 'W' || format[1] != 'A' || format[2] != 'V' || format[3] != 'E')
	{
		Global::error = "Invalid WAV format";
		return false;
	}

	// Read fmt chunk
	wav_fmtchunk_t fmtchunk;
	in.read(&fmtchunk, sizeof(wav_fmtchunk_t));

	// Check fmt chunk values
	if (fmtchunk.header.id[0] != 'f' || fmtchunk.header.id[1] != 'm' || fmtchunk.header.id[2] != 't' || fmtchunk.header.id[3] != ' ')
	{
		Global::error = "Invalid WAV";
		return false;
	}
	if (fmtchunk.channels != 1)
	{
		Global::error = "Cannot convert, must be mono";
		return false;
	}
	if (fmtchunk.bps != 8)
	{
		Global::error = "Cannot convert, must be 8bit";
		return false;
	}

	// Read data
	in.read(&chunk, 8);

	// Check data
	if (chunk.id[0] != 'd' || chunk.id[1] != 'a' || chunk.id[2] != 't' || chunk.id[3] != 'a')
	{
		Global::error = "Invalid WAV";
		return false;
	}

	uint8_t* data = new uint8_t[chunk.size];
	in.read(data, chunk.size);


	// --- Write Doom Sound ---

	// Write header
	dsnd_header_t ds_hdr;
	ds_hdr.three = 3;
	ds_hdr.samplerate = fmtchunk.samplerate;
	ds_hdr.samples = chunk.size;
	out.write(&ds_hdr, 8);

	// Write data
	out.write(data, chunk.size);

	return true;
}

/* Conversions::musToMidi
 * Converts mus data [in] to midi, written to [out]
 *******************************************************************/
bool Conversions::musToMidi(MemChunk& in, MemChunk& out)
{
	return mus2mid(in, out);
}

/* Conversions::zmusToMidi
 * Converts MIDI-like music data [in] to midi, written to [out],
 * using ZDoom MIDI system
 *******************************************************************/
bool Conversions::zmusToMidi(MemChunk& in, MemChunk& out)
{
	return zmus2mid(in, out);
}

/* Conversions::vocToWav
 * Creative Voice files to wav format
 *******************************************************************/
bool Conversions::vocToWav(MemChunk& in, MemChunk& out)
{
	if (in.getSize() < 26 || in[19] != 26 || in[20] != 26 || in[21] != 0
	        || (0x1234 + ~(READ_L16(in, 22)) != (READ_L16(in, 24))))
	{
		Global::error = "Invalid VOC";
		return false;
	}

	// --- Prepare WAV ---
	wav_chunk_t whdr, wdhdr;
	wav_fmtchunk_t fmtchunk;

	// --- Pre-process the file to make sure we can convert it ---
	int codec = -1;
	int blockcount = 0;
	size_t datasize = 0;
	size_t i = 26, e = in.getSize();
	bool gotextra = false;
	while (i < e)
	{
		// Parses through blocks
		uint8_t blocktype = in[i];
		size_t blocksize = READ_L24(in, i+1);
		i+=4;
		if (i + blocksize > e && blocktype != 0)
		{
			Global::error = S_FMT("VOC file cut abruptly in block %i", blockcount);
			return false;
		}
		blockcount++;
		switch (blocktype)
		{
		case 0: // Terminator, the rest should be ignored
			i = e; break;
		case 1: // Sound data
			if (!gotextra && codec >= 0 && codec != in[i+1])
			{
				Global::error = "VOC files with different codecs are not supported";
				return false;
			}
			else if (codec == -1)
			{
				fmtchunk.samplerate = 1000000/(256 - in[i]);
				fmtchunk.channels = 1;
				fmtchunk.tag = 1;
				codec = in[i+1];
			}
			datasize += blocksize - 2;
			break;
		case 2: // Sound data continuation
			if (codec == -1)
			{
				Global::error = "Sound data without codec in VOC file";
				return false;
			}
			datasize += blocksize;
			break;
		case 3: // Silence
		case 4: // Marker
		case 5: // Text
		case 6: // Repeat start point
		case 7: // Repeat end point
			break;
		case 8: // Extra info, overrides any following sound data codec info
			if (codec != -1)
			{
				Global::error = "Extra info block must precede sound data info block in VOC file";
				return false;
			}
			else
			{
				fmtchunk.samplerate = 256000000/((in[i+3] + 1) * (65536 - READ_L16(in, i)));
				fmtchunk.channels = in[i+3] + 1;
				fmtchunk.tag = 1;
				codec = in[i+2];
			}
			break;
		case 9: // Sound data in new format
			if (codec >= 0 && codec != READ_L16(in, i+6))
			{
				Global::error = "VOC files with different codecs are not supported";
				return false;
			}
			else if (codec == -1)
			{
				fmtchunk.samplerate = READ_L32(in, i);
				fmtchunk.bps = in[i+4];
				fmtchunk.channels = in[i+5];
				fmtchunk.tag = 1;
				codec = READ_L16(in, i+6);
			}
			datasize += blocksize - 12;
			break;
		}
		i += blocksize;
	}
	wdhdr.size = datasize;
	switch (codec)
	{
	case 0: // 8 bits unsigned PCM
		fmtchunk.bps = 8;
		fmtchunk.datarate = fmtchunk.samplerate;
		fmtchunk.blocksize = 1;
		break;
	case 4: // 16 bits signed PCM
		fmtchunk.bps = 16;
		fmtchunk.datarate = fmtchunk.samplerate<<1;
		fmtchunk.blocksize = 2;
		break;
	case 1: // 4 bits to 8 bits Creative ADPCM
	case 2: // 3 bits to 8 bits Creative ADPCM (AKA 2.6 bits)
	case 3: // 2 bits to 8 bits Creative ADPCM
	case 6: // alaw
	case 7: // ulaw
	case 0x200: // 4 bits to 16 bits Creative ADPCM (only valid in block type 0x09)
		Global::error = S_FMT("Unsupported codec %i in VOC file", codec);
		return false;
	default:
		Global::error = S_FMT("Unknown codec %i in VOC file", codec);
		return false;
	}

	// --- Write WAV ---

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 8;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(wav_fmtchunk_t));
	out.write(&wdhdr, 8);

	// Now go and copy sound data
	const uint8_t* src = in.getData();
	i = 26;
	while (i < e)
	{
		// Parses through blocks again
		uint8_t blocktype = in[i];
		size_t blocksize = READ_L24(in, i+1);
		i+=4;
		switch (blocktype)
		{
		case 1: // Sound data
			out.write(src+i+2, blocksize - 2);
			break;
		case 2: // Sound data continuation
			out.write(src+i, blocksize);
			break;
		case 3: // Silence
			// Not supported yet
			break;
		case 9: // Sound data in new format
			out.write(src+i+12, blocksize - 12);
		default:
			break;
		}
		i += blocksize;
	}

	return true;
}

/* Conversions::bloodToWav
 * Blood SFX files to wav format
 *******************************************************************/
bool Conversions::bloodToWav(ArchiveEntry* in, MemChunk& out)
{
	MemChunk& mc = in->getMCData();
	if (mc.getSize() < 22 || mc.getSize() > 29 || (mc[12] != 1 && mc[12] != 5 || mc[mc.getSize()-1] != 0))
	{
		Global::error = "Invalid SFX";
		return false;
	}
	string name;
	for (size_t i = 20; i < mc.getSize() - 1; ++i)
	{
		// Check that the entry does give a purely alphanumeric ASCII name
		if ((mc[i] < '0' || (mc[i] > '9' && mc[i] < 'A') ||
		        (mc[i] > 'Z' && mc[i] < 'a') || mc[i] > 'z') && mc[i] != '_')
		{
			Global::error = "Invalid SFX";
			return false;
		}
		else name += mc[i];
	}

	// Find raw data
	name += ".raw";
	ArchiveEntry* raw = in->getParent()->getEntry(name);
	if (!raw || raw->getSize() == 0)
	{
		Global::error = "No RAW data for SFX";
		return false;
	}

	size_t rawsize = raw->getSize();

	// --- Write WAV ---
	wav_chunk_t whdr, wdhdr;
	wav_fmtchunk_t fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = rawsize;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag = 1;
	fmtchunk.channels = 1;
	fmtchunk.samplerate = mc[12] == 5 ? 22050 : 11025;
	fmtchunk.datarate = fmtchunk.samplerate;
	fmtchunk.blocksize = 1;
	fmtchunk.bps = 8;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 8;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(wav_fmtchunk_t));
	out.write(&wdhdr, 8);
	out.write(raw->getData(), raw->getSize());

	return true;
}

/* Conversions::wolfSndToWav
 * Converts Wolf3D sound data [in] to wav format, written to [out]
 *******************************************************************/
bool Conversions::wolfSndToWav(MemChunk& in, MemChunk& out)
{
	// --- Read Doom sound ---

	// Read samples
	size_t numsamples = in.getSize();
	const uint8_t* samples = in.getData();

	// --- Write WAV ---

	wav_chunk_t whdr, wdhdr;
	wav_fmtchunk_t fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = numsamples;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag = 1;
	fmtchunk.channels = 1;
	fmtchunk.samplerate = 7042;
	fmtchunk.datarate = 7042;
	fmtchunk.blocksize = 1;
	fmtchunk.bps = 8;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 8;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(wav_fmtchunk_t));
	out.write(&wdhdr, 8);
	out.write(samples, numsamples);

	// Ensure data ends on even byte boundary
	if (numsamples % 2 != 0)
		out.write("\0", 1);

	return true;
}

/* Conversions::jagSndToWav
 * Converts Jaguar Doom sound data [in] to wav format, written to [out]
 *******************************************************************/
bool Conversions::jagSndToWav(MemChunk& in, MemChunk& out)
{
	// --- Read Jaguar Doom sound ---

	// Read Jaguar doom sound header
	jsnd_header_t header;
	in.seek(0, SEEK_SET);
	in.read(&header, 28);

	// Correct endianness for the one value we actually use
	// (The rest of the header is in big endian format too, but we
	//  don't use these values so we don't need to correct them.)
	header.samples = wxINT32_SWAP_ON_LE(header.samples);

	// Format checks
	if (header.samples > (in.getSize() - 28) || header.samples <= 4)  	// Check for sane values
	{
		Global::error = "Invalid Jaguar Doom Sound";
		return false;
	}

	// Read samples
	uint8_t* samples = new uint8_t[header.samples];
	in.read(samples, header.samples);


	// --- Write WAV ---

	wav_chunk_t whdr, wdhdr;
	wav_fmtchunk_t fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = header.samples;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag = 1;
	fmtchunk.channels = 1;
	fmtchunk.samplerate = 11025;
	fmtchunk.datarate = 11025;
	fmtchunk.blocksize = 1;
	fmtchunk.bps = 8;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 8;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(wav_fmtchunk_t));
	out.write(&wdhdr, 8);
	out.write(samples, header.samples);

	// Ensure data ends on even byte boundary
	if (header.samples % 2 != 0)
		out.write("\0", 1);

	return true;
}

/* Conversions::gmidToMidi
 * Dark Forces GMID file to Standard MIDI File
 *******************************************************************/
bool Conversions::gmidToMidi(MemChunk& in, MemChunk& out)
{
	// Skip beginning of file and look for MThd chunk
	// (the standard MIDI header)
	size_t size = in.getSize();
	if (size < 16)
		return false;
	if (in[0] != 'M' && in[1] != 'I' && in[2] != 'D' && in[3] != 'I' &&
	        ((READ_B32(in, 4) + 8) != size))
		return false;

	size_t offset = 8;
	bool notfound = true;
	while (notfound)
	{
		if (offset + 8 >  size)
			return false;
		// Look for header
		if (in[offset] == 'M' && in[offset+1] == 'T' && in[offset+2] == 'h' && in[offset+3] == 'd')
			notfound = false;
		else
			offset += (READ_B32(in, offset+4) + 8);
	}

	// Write the rest of the file
	out.write(in.getData() + offset, size - offset);

	return true;
}
