
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
#include "Archive/Archive.h"
#include "Conversions.h"
#include "Archive/ArchiveEntry.h"
#include "External/mus2mid/mus2mid.h"
#include "External/zreaders/i_music.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, dmx_padding, true, CVAR_SAVE)
CVAR(Int, wolfsnd_rate, 7042, CVAR_SAVE)


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

// For speaker sound conversion
struct spksnd_header_t
{
	uint16_t zero;
	uint16_t samples;
};
// Sun sound header
struct sunsnd_header_t
{
	uint32_t magic;
	uint32_t offset;
	uint32_t size;
	uint32_t format;
	uint32_t rate;
	uint32_t channels;
};


/*******************************************************************
 * FUNCTIONS
 *******************************************************************/

// More conversion functions for internal use only
namespace Conversions
{
	uint8_t pcm16to8bits(int16_t val);
	uint8_t pcm24to8bits(int32_t val);
	uint8_t pcm32to8bits(int32_t val);
	uint8_t stereoToMono(uint8_t left, uint8_t right);
	int16_t AlawToLinear(uint8_t alaw);
	int16_t mulawToLinear(uint8_t ulaw);
}

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

	// Detect if DMX padding is present
	// It was discovered ca. 2013 that the original DMX sound format used in Doom
	// contains 32 padding bytes that are ignored during playback: 16 leading pad
	// bytes and 16 trailing ones. The leading bytes are identical copies of the 
	// first actual sample, and trailing ones are copies of the last sample. The 
	// header's sample count does include these padding bytes.
	uint8_t samples_offset = 16;
	if (header.samples > 33 && dmx_padding)
	{
		size_t e = header.samples - 16;
		for (int i = 0; i < 16; ++i)
		{
			if (samples[i] != samples[16] || samples[e+i] != samples[e-1])
			{
				samples_offset = 0;
				break;
			}
		}
	} else samples_offset = 0;

	if (samples_offset)
		header.samples -= 32;


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
	out.write(samples+samples_offset, header.samples);

	// Ensure data ends on even byte boundary
	if (header.samples % 2 != 0)
		out.write("\0", 1);

	delete[] samples;

	return true;
}

/* Conversions::wavToDoomSnd
 * Converts wav data [in] to doom sound format, written to [out]
 *******************************************************************/
#define WAV_PCM 1
#define WAV_ALAW 6
#define WAV_ULAW 7
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

	// Find fmt chunk
	size_t ofs = 12;
	while (ofs < in.getSize())
	{
		if (in[ofs] == 'f' && in[ofs+1] == 'm' && in[ofs+2] == 't' && in[ofs+3] == ' ')
			break;
		ofs += 8 + READ_L32(in, (ofs + 4));
	}

	// Read fmt chunk
	if (ofs + sizeof(wav_fmtchunk_t) > in.getSize())
	{
		Global::error = "Invalid WAV: no 'fmt ' chunk";
		return false;
	}
	in.seek(ofs, SEEK_SET);
	wav_fmtchunk_t fmtchunk;
	in.read(&fmtchunk, sizeof(wav_fmtchunk_t));
	
	// Get format
	uint8_t wavfmt = fmtchunk.tag == 0xFFFE ? READ_L32(in, ofs + 32) : fmtchunk.tag;
	// Get byte per samples (from bits per sample)
	uint8_t wavbps = fmtchunk.bps / 8;

	// Check fmt chunk values
	if (fmtchunk.channels > 2 || fmtchunk.bps % 8 || wavbps > 4 ||
		(wavfmt != WAV_PCM && wavfmt != WAV_ALAW && wavfmt != WAV_ULAW))
	{
		Global::error = "Cannot convert WAV file, only stereo or monophonic sounds in PCM format can be converted";
		return false;
	}

	// Warn
	if (wavbps > 1 || wavfmt != WAV_PCM || fmtchunk.channels == 2)
	{
		if (!(wxMessageBox(S_FMT("Warning: conversion will result in loss of metadata and audio quality. Do you wish to proceed?"), "Conversion warning", wxOK | wxCANCEL) == wxOK))
		{
			Global::error = "Conversion aborted by user";
			return false;
		}
	}

	// Find data chunk
	ofs += 8 + wxUINT32_SWAP_ON_BE(fmtchunk.header.size);
	while (ofs < in.getSize())
	{
		if (in[ofs] == 'd' && in[ofs+1] == 'a' && in[ofs+2] == 't' && in[ofs+3] == 'a')
			break;
		ofs += 8 + READ_L32(in, (ofs + 4));
	}

	// Read data
	if (ofs + sizeof(wav_fmtchunk_t) > in.getSize())
	{
		Global::error = "Invalid WAV: no 'data' chunk";
		return false;
	}
	in.seek(ofs, SEEK_SET);
	in.read(&chunk, 8);

	if (wavbps > 1)
		chunk.size /= wavbps;

	uint8_t* data = new uint8_t[chunk.size];
	uint8_t padding[16];

	// Store sample data. A simple read for 8 bits per sample.
	if (fmtchunk.bps == 8)
		in.read(data, chunk.size);
	// For 16, 24, or 32 bits per sample, downsample to 8.
	else
	{
		int32_t val = 0;
		for (size_t i = 0; i < chunk.size; ++i)
		{
			in.read(&val, wavbps);
			if (wavbps == 4) data[i] = pcm32to8bits(val);
			else if (wavbps == 3) data[i] = pcm24to8bits(val);
			else if (wavbps == 2) data[i] = pcm16to8bits((int16_t)val);
		}
	}

	// Convert A-law or µ-law to 8-bit linear
	if (wavfmt == WAV_ALAW || wavfmt == WAV_ULAW)
	{
		for (size_t i = 0; i < chunk.size; ++i)
		{
			int16_t val = (wavfmt == WAV_ALAW) ? AlawToLinear(data[i]) : mulawToLinear(data[i]);
			data[i] = pcm16to8bits(val);
		}
	}

	// Merge stereo channels into a single mono one
	if (fmtchunk.channels == 2)
	{
		chunk.size /= 2;
		for (size_t i = 0; i < chunk.size; ++i)
		{
			size_t j = 2 * i;
			data[i] = stereoToMono(data[j], data[j + 1]);
		}
	}

	// --- Write Doom Sound ---

	// Write header
	dsnd_header_t ds_hdr;
	ds_hdr.three = 3;
	ds_hdr.samplerate = fmtchunk.samplerate;
	ds_hdr.samples = chunk.size;
	if (dmx_padding)
		ds_hdr.samples += 32;
	out.write(&ds_hdr, 8);

	// Write data
	if (dmx_padding)
	{
		memset(padding, data[0], 16);
		out.write(padding, 16);
	}
	out.write(data, chunk.size);
	if (dmx_padding)
	{
		memset(padding, data[chunk.size - 1], 16);
		out.write(padding, 16);
	}

	delete[] data;

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
bool Conversions::zmusToMidi(MemChunk& in, MemChunk& out, int subsong, int * num_tracks)
{
	return zmus2mid(in, out, subsong, num_tracks);
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
		size_t blocksize = (i + 4 < e) ? READ_L24(in, i+1) : 0x1000000;
		i+=4;
		if (i + blocksize > e && i < e && blocktype != 0)
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
	if (mc.getSize() < 22 || mc.getSize() > 29 || ((mc[12] != 1 && mc[12] != 5) || mc[mc.getSize()-1] != 0))
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
	// --- Read Wolf sound ---

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
	fmtchunk.samplerate = wolfsnd_rate;
	fmtchunk.datarate = wolfsnd_rate;
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

/* Conversions::rmidToMidi
 * RMID file to Standard MIDI File
 *******************************************************************/
bool Conversions::rmidToMidi(MemChunk& in, MemChunk& out)
{
	// Skip beginning of file and look for MThd chunk
	// (the standard MIDI header)
	size_t size = in.getSize();
	if (size < 36)
		return false;
	if (in[0] != 'R' && in[1] != 'I' && in[2] != 'F' && in[3] != 'F' &&
	        ((READ_L32(in, 4) + 8) != size))
		return false;

	size_t offset = 12;
	size_t datasize = 0;
	bool notfound = true;
	while (notfound)
	{
		if (offset + 20 >  size)
			return false;
		// Look for header
		if (in[offset] == 'd' && in[offset+1] == 'a' && in[offset+2] == 't' && in[offset+3] == 'a' &&
			in[offset+8] == 'M' && in[offset+9] == 'T' && in[offset+10] == 'h' && in[offset+11] == 'd')
		{
			notfound = false;
			datasize = READ_L32(in, offset+4);
			offset += 8;
		}
		else
			offset += (READ_L32(in, offset+4) + 8);
	}

	// Write the rest of the file
	if (offset + datasize <= size)
	{
		out.write(in.getData() + offset, datasize);
		return true;
	}
	return false;
}

/* Conversions::addImfHeader
 * Automatizes this: http://zdoom.org/wiki/Using_OPL_music_in_ZDoom
 *******************************************************************/
bool Conversions::addImfHeader(MemChunk& in, MemChunk& out)
{
	if (in.getSize() == 0)
		return false;

	uint32_t newsize = in.getSize() + 9;
	uint8_t start = 0;
	if (in[0] | in[1])
	{
		// non-zero start
		newsize += 2;
		start = 2;
	}
	else newsize += 4;

	out.reSize(newsize, false);
	out[0] = 'A'; out[1] = 'D'; out[2] = 'L'; out[3] = 'I'; out[4] = 'B';
	out[5] = 1; out[6] = 0; out[7] = 0; out[8] = 1;
	if (in[0] | in[1])
	{
		out[9] = in[0]; out[10] = in[1]; out[11] = 0; out[12] = 0;
	}
	else
	{
		out[9] = 0; out[10] = 0; out[11] = 0; out[12] = 0;
	}
	out.seek(13, SEEK_SET);
	in.seek(start, SEEK_SET);
	//size_t size = MIN(in.getSize() - start, newsize - 13);
	//return in.readMC(out, size);
	for (size_t i = 0; ((i + start < in.getSize()) && (13 + i < newsize)); ++i)
	{
		out[13 + i] = in[i+start];
	}
	return true;
}

/* Conversions::spkSndToWav
 * Converts Doom PC speaker sound data [in] to wav format, written 
 * to [out]. This code is partly adapted from info found on
 * http://www.shikadi.net/moddingwiki/AudioT_Format and
 * http://www.shikadi.net/moddingwiki/Inverse_Frequency_Sound_format 
 *
 *******************************************************************/
#define ORIG_RATE 140.0
#define FACTOR 315	// 315*140 = 44100
#define FREQ 1193181.0
#define RATE (ORIG_RATE * FACTOR)
#define PC_VOLUME 20
uint16_t counters[128] =
{
	   0, 6818, 6628, 6449, 6279, 6087, 5906, 5736,
	5575, 5423, 5279, 5120, 4971, 4830, 4697, 4554,
	4435, 4307, 4186, 4058, 3950, 3836, 3728, 3615,
	3519, 3418, 3323, 3224, 3131, 3043, 2960, 2875,
	2794, 2711, 2633, 2560, 2485, 2415, 2348, 2281,
	2213, 2153, 2089, 2032, 1975, 1918, 1864, 1810,
	1757, 1709, 1659, 1612, 1565, 1521, 1478, 1435,
	1395, 1355, 1316, 1280, 1242, 1207, 1173, 1140,
	1107, 1075, 1045, 1015,  986,  959,  931,  905,
	 879,  854,  829,  806,  783,  760,  739,  718,
	 697,  677,  658,  640,  621,  604,  586,  570,
	 553,  538,  522,  507,  493,  479,  465,  452,
	 439,  427,  415,  403,  391,  380,  369,  359,
	 348,  339,  329,  319,  310,  302,  293,  285,
	 276,  269,  261,  253,  246,  239,  232,  226,
	 219,  213,  207,  201,  195,  190,  184,  179
};
bool Conversions::spkSndToWav(MemChunk& in, MemChunk& out, bool audioT)
{
	// --- Read Doom sound ---
	// -- Also AudioT sound --

	size_t minsize = 4 + (audioT ? 3 : 0);
	if (in.getSize() < minsize)
	{
		Global::error = "Invalid PC Speaker Sound";
		return false;
	}

	// Read doom sound header
	spksnd_header_t header;
	in.seek(0, SEEK_SET);
	in.read(&header, 4);
	size_t numsamples;

	// Format checks
	if (audioT)
	{
		numsamples = READ_L32(in, 0);
		uint16_t priority;
		in.read(&priority, 2);
		if (in.getSize() < 6 + numsamples)
		{
			Global::error = "Invalid AudioT PC Speaker Sound";
			return false;
		}
	}
	else
	{
		if (header.zero != 0)  	// Check for magic number
		{
			Global::error = "Invalid Doom PC Speaker Sound";
			return false;
		}
		if (header.samples > (in.getSize() - 4) || header.samples <= 4)  	// Check for sane values
		{
			Global::error = "Invalid Doom PC Speaker Sound";
			return false;
		}
		numsamples = header.samples;
	}

	// Read samples
	uint8_t* osamples = new uint8_t[numsamples];
	uint8_t* nsamples = new uint8_t[numsamples*FACTOR];
	in.read(osamples, numsamples);

	int sign = -1;
	uint32_t phase_tic = 0;

	// Convert counter values to sample values
	for (size_t s = 0; s < numsamples; ++s)
	{
		if (osamples[s] > 127 && !audioT)
		{
			Global::error = S_FMT("Invalid PC Speaker counter value: %d > 127", osamples[s]);
			delete[] osamples;
			delete[] nsamples;
			return false;
		}
		if (osamples[s] > 0)
		{
			// First, convert counter value to frequency in Hz
			//double f = FREQ / (double)counters[osamples[s]];
			uint32_t tone = audioT ? osamples[s] * 60 : counters[osamples[s]];
			uint32_t phase_length = (tone * RATE) / (2 * FREQ);

			// Then write a bunch of samples.
			for (int i = 0; i < FACTOR; ++i)
			{
				// Finally, convert frequency into sample value
				int pos = (s*FACTOR) + i;
				nsamples[pos] = 128 + sign*PC_VOLUME;
				if (phase_tic++ >= phase_length)
				{
					sign = -sign;
					phase_tic = 0;
				}
			}
		}
		else
		{
			memset(nsamples + (s*FACTOR), 128, FACTOR);
			phase_tic = 0;
		}
	}

	// --- Write WAV ---

	wav_chunk_t whdr, wdhdr;
	wav_fmtchunk_t fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = numsamples * FACTOR;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag = 1;
	fmtchunk.channels = 1;
	fmtchunk.samplerate = RATE;
	fmtchunk.datarate = RATE;
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
	out.write(nsamples, numsamples * FACTOR);

	// Ensure data ends on even byte boundary
	if (header.samples % 2 != 0)
		out.write("\0", 1);

	delete[] osamples;
	delete[] nsamples;

	return true;
}

/* Conversions::auSndToWav
 * Converts Sun/NeXT sound data [in] to wav format, written to [out]
 *******************************************************************/
bool Conversions::auSndToWav(MemChunk& in, MemChunk& out)
{
	// --- Read Sun sound ---

	// Read doom sound header
	sunsnd_header_t header;
	in.seek(0, SEEK_SET);
	in.read(&header, 24);

	header.magic = wxUINT32_SWAP_ON_LE(header.magic);
	header.offset = wxUINT32_SWAP_ON_LE(header.offset);
	header.size = wxUINT32_SWAP_ON_LE(header.size);
	header.format = wxUINT32_SWAP_ON_LE(header.format);
	header.rate = wxUINT32_SWAP_ON_LE(header.rate);
	header.channels = wxUINT32_SWAP_ON_LE(header.channels);

	// Format checks
	if (header.magic != 0x2E736E64)  	// ASCII code for ".snd"
	{
		Global::error = "Invalid Sun Sound";
		return false;
	}
	// Only cover integer linear PCM for now
	if (header.format < 2 || header.format > 5)
	{
		Global::error = S_FMT("Unsupported Sun Sound format (%d)", header.format);
		return false;
	}
	uint8_t samplesize = header.format - 1;
	if (header.format == 1) samplesize = 1;
	else if (header.format >= 6) --samplesize;

	// Read samples
	uint8_t* samples = new uint8_t[header.size];
	in.read(samples, header.size);

	// Swap endianness around if needed
	if (samplesize > 1)
	{
		uint8_t swapval = 0;
		for (size_t i = 0; i < header.size / samplesize; ++i)
		{
			switch (samplesize)
			{
			case 2:
				swapval = samples[i*2];
				samples[i*2] = samples[i*2 + 1];
				samples[i*2 + 1] = swapval;
				break;
			case 3:
				swapval = samples[i*3];
				samples[i*3] = samples[i*3 + 2];
				samples[i*3 + 2] = swapval;
				break;
			case 4:
				swapval = samples[i*4];
				samples[i*4] = samples[i*4 + 3];
				samples[i*4 + 3] = swapval;
				swapval = samples[i*4 + 2];
				samples[i*4 + 2] = samples[i*4 + 1];
				samples[i*4 + 1] = swapval;
				break;
			}
		}
	}

	// --- Write WAV ---

	wav_chunk_t whdr, wdhdr;
	wav_fmtchunk_t fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = header.size;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag = 1;
	fmtchunk.channels = header.channels;
	fmtchunk.samplerate = header.rate;
	fmtchunk.datarate = header.rate * header.channels;
	fmtchunk.blocksize = samplesize;
	fmtchunk.bps = 8 * samplesize;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 8;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(wav_fmtchunk_t));
	out.write(&wdhdr, 8);
	out.write(samples, header.size);

	// Ensure data ends on even byte boundary
	if (header.size % 2 != 0)
		out.write("\0", 1);

	delete[] samples;

	return true;
}

/* Conversions::pcm16to8bits
 * Converts a 16-bit signed sample to an 8-bit unsigned one.
 *******************************************************************/
uint8_t Conversions::pcm16to8bits(int16_t val)
{
	// Value is in the [-32768, 32767] range.
	// Shift it eight bits to [-128, 127] range,
	// and add 128 to put it in [0, 255] range.
	uint8_t ret = 128 + (val >> 8);
	// Round value up or down depending on value
	// of shifted-off bits.
	if ((val & 0x80FF) > 127)  ret++;
	else if ((val & 0x80FF) < -128) ret--;
	// Send it.
	return ret;
}

/* Conversions::pcm24to8bits
 * Converts a 24-bit signed sample to an 8-bit unsigned one.
 *******************************************************************/
uint8_t Conversions::pcm24to8bits(int32_t val)
{
	int16_t ret = (val >> 8);
	if ((val & 0x8000FF) > 127)  ret++;
	else if ((val & 0x8000FF) < -128) ret--;
	return pcm16to8bits(ret);
}

/* Conversions::pcm32to8bits
 * Converts a 32-bit signed sample to an 8-bit unsigned one.
 *******************************************************************/
uint8_t Conversions::pcm32to8bits(int32_t val)
{
	int32_t ret = (val >> 8);
	if ((val & 0x800000FF) > 127)  ret++;
	else if ((val & 0x800000FF) < -128) ret--;
	return pcm24to8bits(ret);
}

/* Conversions::stereoToMono
 * Averages the values of two eight-bit unsigned samples into one.
 *******************************************************************/
uint8_t Conversions::stereoToMono(uint8_t left, uint8_t right)
{
	uint16_t val = left + right;
	val /= 2;
	if (val > 255)
		val = 255;
	return (uint8_t)val;
}

/* The following two functions are adapted from Sun Microsystem's g711.cpp code.
 * Unrestricted use and modifications are allowed.
 */

#define	SIGN_BIT	(0x80)	// Sign bit (values are otherwise treated as unsigned).
#define	QUANT_MASK	(0xf)	// Quantization field mask.
#define	SEG_SHIFT	(4) 	// Left shift for segment number.
#define	SEG_MASK	(0x70)	// Segment field mask.
#define	BIAS		(0x84)	// Bias for linear code.

/* Conversions::AlawToLinear
 * Converts a 8-bit A-law sample to 16-bit signed linear PCM
 *******************************************************************/
int16_t Conversions::AlawToLinear(uint8_t alaw)
{
	int16_t	t;
	int16_t	seg;

	alaw ^= 0x55;

	t = (alaw & QUANT_MASK) << 4;
	seg = (alaw & SEG_MASK) >> SEG_SHIFT;
	switch (seg)
	{
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return ((alaw & SIGN_BIT) ? t : -t);
}

 /* Conversions::mulawToLinear
  * Converts a 8-bit µ-law sample to 16-bit signed linear PCM
  *******************************************************************/
int16_t Conversions::mulawToLinear(uint8_t ulaw)
{
	int	t;

	/* Complement to obtain normal u-law value. */
	ulaw = ~ulaw;

	/*
	* Extract and bias the quantization bits. Then
	* shift up by the segment number and subtract out the bias.
	*/
	t = ((ulaw & QUANT_MASK) << 3) + BIAS;
	t <<= (ulaw & SEG_MASK) >> SEG_SHIFT;

	return ((ulaw & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}
