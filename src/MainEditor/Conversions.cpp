
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Conversions.cpp
// Description: Functions to perform various data type conversions
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
#include "Conversions.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Mus2Mid.h"
#include "ZReaders/i_music.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::conversion
{
constexpr int16_t SIGN_BIT   = 0x80; // Sign bit = values are otherwise treated as unsigned).
constexpr int16_t QUANT_MASK = 0xf;  // Quantization field mask.
constexpr int16_t SEG_SHIFT  = 4;    // Left shift for segment number.
constexpr int16_t SEG_MASK   = 0x70; // Segment field mask.
constexpr int16_t BIAS       = 0x84; // Bias for linear code.
constexpr uint8_t WAV_PCM    = 1;
constexpr uint8_t WAV_ALAW   = 6;
constexpr uint8_t WAV_ULAW   = 7;
} // namespace slade::conversion
CVAR(Bool, dmx_padding, true, CVar::Flag::Save)
CVAR(Int, wolfsnd_rate, 7042, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Structs
//
// -----------------------------------------------------------------------------
namespace slade::conversion
{
// Some structs for wav conversion
struct WavChunk
{
	char     id[4];
	uint32_t size;
};

struct WavFmtChunk
{
	WavChunk header;
	uint16_t tag;
	uint16_t channels;
	uint32_t samplerate;
	uint32_t datarate;
	uint16_t blocksize;
	uint16_t bps;
};

// For doom sound conversion
struct DSndHeader
{
	uint16_t three;
	uint16_t samplerate;
	uint32_t samples;
};

// For Jaguar doom sound conversion
struct JSndHeader
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
struct SpkSndHeader
{
	uint16_t zero;
	uint16_t samples;
};
// Sun sound header
struct SunSndHeader
{
	uint32_t magic;
	uint32_t offset;
	uint32_t size;
	uint32_t format;
	uint32_t rate;
	uint32_t channels;
};

struct VoxHeader
{
	uint32_t width;
	uint32_t length;
	uint32_t height;
};

struct KvxHeader
{
	uint32_t total_bytes;

	uint32_t width;
	uint32_t length;
	uint32_t height;

	uint32_t pivot_x;
	uint32_t pivot_y;
	uint32_t pivot_z;
};

struct KvxColumnPostHeader
{
	uint8_t topdelta;
	uint8_t size;
	uint8_t culling;
};

} // namespace slade::conversion


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------

// More conversion functions for internal use only
namespace slade::conversion
{
// -----------------------------------------------------------------------------
// Converts a 16-bit signed sample to an 8-bit unsigned one.
// -----------------------------------------------------------------------------
uint8_t pcm16to8bits(int16_t val)
{
	// Value is in the [-32768, 32767] range.
	// Shift it eight bits to [-128, 127] range,
	// and add 128 to put it in [0, 255] range.
	uint8_t ret = 128 + (val >> 8);
	// Round value up or down depending on value
	// of shifted-off bits.
	if ((val & 0x80FF) > 127 && ret < 255)
		ret++;
	else if ((val & 0x80FF) < -128 && ret > 0)
		ret--;
	// Send it.
	return ret;
}

// -----------------------------------------------------------------------------
// Converts a 24-bit signed sample to an 8-bit unsigned one.
// -----------------------------------------------------------------------------
uint8_t pcm24to8bits(int32_t val)
{
	int16_t ret = (val >> 8);
	if ((val & 0x8000FF) > 127 && ret < 255)
		ret++;
	else if ((val & 0x8000FF) < -128 && ret > 0)
		ret--;
	return pcm16to8bits(ret);
}

// -----------------------------------------------------------------------------
// Converts a 32-bit signed sample to an 8-bit unsigned one.
// -----------------------------------------------------------------------------
uint8_t pcm32to8bits(int32_t val)
{
	static constexpr int32_t mod = 0x800000FF;
	int32_t                  ret = (val >> 8);
	if ((val & mod) > 127 && ret < 255)
		ret++;
	else if ((val & mod) < -128 && ret > 0)
		ret--;
	return pcm24to8bits(ret);
}

// -----------------------------------------------------------------------------
// Averages the values of two eight-bit unsigned samples into one.
// -----------------------------------------------------------------------------
uint8_t stereoToMono(uint8_t left, uint8_t right)
{
	uint16_t val = left + right;
	val /= 2;
	if (val > 255)
		val = 255;
	return static_cast<uint8_t>(val);
}

// The following two functions are adapted from Sun Microsystem's g711.cpp code.
// Unrestricted use and modifications are allowed.

// -----------------------------------------------------------------------------
// Converts a 8-bit A-law sample to 16-bit signed linear PCM
// -----------------------------------------------------------------------------
int16_t alawToLinear(uint8_t alaw)
{
	int16_t t;
	int16_t seg;

	alaw ^= 0x55;

	t   = (alaw & QUANT_MASK) << 4;
	seg = (alaw & SEG_MASK) >> SEG_SHIFT;
	switch (seg)
	{
	case 0:  t += 8; break;
	case 1:  t += 0x108; break;
	default: t += 0x108; t <<= seg - 1;
	}
	return ((alaw & SIGN_BIT) ? t : -t);
}

// -----------------------------------------------------------------------------
// Converts a 8-bit µ-law sample to 16-bit signed linear PCM
// -----------------------------------------------------------------------------
int16_t mulawToLinear(uint8_t ulaw)
{
	int t;

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
} // namespace slade::conversion

// -----------------------------------------------------------------------------
// Converts doom sound data [in] to wav format, written to [out]
// -----------------------------------------------------------------------------
bool conversion::doomSndToWav(const MemChunk& in, MemChunk& out)
{
	// --- Read Doom sound ---

	// Read doom sound header
	DSndHeader header;
	in.seek(0, SEEK_SET);
	in.read(&header, 8);

	// Some sounds created on Mac platforms have their identifier and samplerate in BE format.
	// Curiously, the number of samples is still in LE format.
	header.three      = wxUINT16_SWAP_ON_BE(header.three);
	header.samplerate = wxUINT16_SWAP_ON_BE(header.samplerate);
	header.samples    = wxUINT32_SWAP_ON_BE(header.samples);
	if (header.three == 0x300)
		header.samplerate = wxUINT16_SWAP_ALWAYS(header.samplerate);

	// Format checks
	if (header.three != 3 && header.three != 0x300) // Check for magic number
	{
		global::error = "Invalid Doom Sound";
		return false;
	}
	if (header.samples > (in.size() - 8) || header.samples <= 4) // Check for sane values
	{
		global::error = "Invalid Doom Sound";
		return false;
	}

	// Read samples
	vector<uint8_t> samples(header.samples);
	in.read(samples.data(), header.samples);

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
			if (samples[i] != samples[16] || samples[e + i] != samples[e - 1])
			{
				samples_offset = 0;
				break;
			}
		}
	}
	else
		samples_offset = 0;

	if (samples_offset)
		header.samples -= 32;


	// --- Write WAV ---

	WavChunk    whdr, wdhdr;
	WavFmtChunk fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = header.samples;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag         = 1;
	fmtchunk.channels    = 1;
	fmtchunk.samplerate  = header.samplerate;
	fmtchunk.datarate    = header.samplerate;
	fmtchunk.blocksize   = 1;
	fmtchunk.bps         = 8;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 20;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(WavFmtChunk));
	out.write(&wdhdr, 8);
	out.write(samples.data() + samples_offset, header.samples);

	// Ensure data ends on even byte boundary
	if (header.samples % 2 != 0)
		out.write("\0", 1);

	return true;
}

// -----------------------------------------------------------------------------
// Converts wav data [in] to doom sound format, written to [out]
// -----------------------------------------------------------------------------
bool conversion::wavToDoomSnd(const MemChunk& in, MemChunk& out)
{
	// --- Read WAV ---
	WavChunk chunk;

	// Read header
	in.seek(0, SEEK_SET);
	in.read(&chunk, 8);

	// Check header
	if (chunk.id[0] != 'R' || chunk.id[1] != 'I' || chunk.id[2] != 'F' || chunk.id[3] != 'F')
	{
		global::error = "Invalid WAV";
		return false;
	}

	// Read format
	char format[4];
	in.read(format, 4);

	// Check format
	if (format[0] != 'W' || format[1] != 'A' || format[2] != 'V' || format[3] != 'E')
	{
		global::error = "Invalid WAV format";
		return false;
	}

	// Find fmt chunk
	size_t ofs = 12;
	while (ofs < in.size())
	{
		if (in[ofs] == 'f' && in[ofs + 1] == 'm' && in[ofs + 2] == 't' && in[ofs + 3] == ' ')
			break;
		ofs += 8 + in.readL32((ofs + 4));
	}

	// Read fmt chunk
	if (ofs + sizeof(WavFmtChunk) > in.size())
	{
		global::error = "Invalid WAV: no 'fmt ' chunk";
		return false;
	}
	in.seek(ofs, SEEK_SET);
	WavFmtChunk fmtchunk;
	in.read(&fmtchunk, sizeof(WavFmtChunk));

	// Get format
	uint8_t wavfmt = fmtchunk.tag == 0xFFFE ? in.readL32(ofs + 32) : fmtchunk.tag;
	// Get byte per samples (from bits per sample)
	uint8_t wavbps = fmtchunk.bps / 8;

	// Check fmt chunk values
	if (fmtchunk.channels > 2 || fmtchunk.bps % 8 || wavbps > 4
		|| (wavfmt != WAV_PCM && wavfmt != WAV_ALAW && wavfmt != WAV_ULAW))
	{
		global::error = "Cannot convert WAV file, only stereo or monophonic sounds in PCM format can be converted";
		return false;
	}

	// Warn
	if (wavbps > 1 || wavfmt != WAV_PCM || fmtchunk.channels == 2)
	{
		if (wxMessageBox(
				wxS("Warning: conversion will result in loss of metadata and audio quality. Do you wish to proceed?"),
				wxS("Conversion warning"),
				wxOK | wxCANCEL)
			!= wxOK)
		{
			global::error = "Conversion aborted by user";
			return false;
		}
	}

	// Find data chunk
	ofs += 8 + wxUINT32_SWAP_ON_BE(fmtchunk.header.size);
	while (ofs < in.size())
	{
		if (in[ofs] == 'd' && in[ofs + 1] == 'a' && in[ofs + 2] == 't' && in[ofs + 3] == 'a')
			break;
		ofs += 8 + in.readL32((ofs + 4));
	}

	// Read data
	if (ofs + sizeof(WavFmtChunk) > in.size())
	{
		global::error = "Invalid WAV: no 'data' chunk";
		return false;
	}
	in.seek(ofs, SEEK_SET);
	in.read(&chunk, 8);

	if (wavbps > 1)
		chunk.size /= wavbps;

	vector<uint8_t> data(chunk.size);
	uint8_t         padding[16];

	// Store sample data. A simple read for 8 bits per sample.
	if (fmtchunk.bps == 8)
		in.read(data.data(), chunk.size);
	// For 16, 24, or 32 bits per sample, downsample to 8.
	else
	{
		int32_t val = 0;
		for (size_t i = 0; i < chunk.size; ++i)
		{
			in.read(&val, wavbps);
			if (wavbps == 4)
				data[i] = pcm32to8bits(val);
			else if (wavbps == 3)
				data[i] = pcm24to8bits(val);
			else if (wavbps == 2)
				data[i] = pcm16to8bits(static_cast<int16_t>(val));
		}
	}

	// Convert A-law or µ-law to 8-bit linear
	if (wavfmt == WAV_ALAW || wavfmt == WAV_ULAW)
	{
		for (size_t i = 0; i < chunk.size; ++i)
		{
			int16_t val = (wavfmt == WAV_ALAW) ? alawToLinear(data[i]) : mulawToLinear(data[i]);
			data[i]     = pcm16to8bits(val);
		}
	}

	// Merge stereo channels into a single mono one
	if (fmtchunk.channels == 2)
	{
		chunk.size /= 2;
		for (size_t i = 0; i < chunk.size; ++i)
		{
			size_t j = 2 * i;
			data[i]  = stereoToMono(data[j], data[j + 1]);
		}
	}

	// --- Write Doom Sound ---

	// Write header
	DSndHeader ds_hdr;
	ds_hdr.three      = 3;
	ds_hdr.samplerate = fmtchunk.samplerate;
	ds_hdr.samples    = chunk.size;
	if (dmx_padding)
		ds_hdr.samples += 32;
	out.write(&ds_hdr, 8);

	// Write data
	if (dmx_padding)
	{
		memset(padding, data[0], 16);
		out.write(padding, 16);
	}
	out.write(data.data(), chunk.size);
	if (dmx_padding)
	{
		memset(padding, data[chunk.size - 1], 16);
		out.write(padding, 16);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Converts mus data [in] to midi, written to [out]
// -----------------------------------------------------------------------------
bool conversion::musToMidi(const MemChunk& in, MemChunk& out)
{
	return mus2mid(in, out);
}

// -----------------------------------------------------------------------------
// Converts MIDI-like music data [in] to midi, written to [out], using ZDoom
// MIDI system
// -----------------------------------------------------------------------------
bool conversion::zmusToMidi(const MemChunk& in, MemChunk& out, int subsong, int* num_tracks)
{
	return zmus2mid(in, out, subsong, num_tracks);
}

// -----------------------------------------------------------------------------
// Creative Voice files to wav format
// -----------------------------------------------------------------------------
bool conversion::vocToWav(const MemChunk& in, MemChunk& out)
{
	if (in.size() < 26 || in[19] != 26 || in[20] != 26 || in[21] != 0
		|| (0x1234 + ~(in.readL16(22)) != (in.readL16(24))))
	{
		global::error = "Invalid VOC";
		return false;
	}

	// --- Prepare WAV ---
	WavChunk    whdr, wdhdr;
	WavFmtChunk fmtchunk;

	// --- Pre-process the file to make sure we can convert it ---
	int    codec      = -1;
	int    blockcount = 0;
	size_t datasize   = 0;
	size_t i = 26, e = in.size();
	bool   gotextra = false;
	while (i < e)
	{
		// Parses through blocks
		uint8_t blocktype = in[i];
		size_t  blocksize = (i + 4 < e) ? in.readL24(i + 1) : 0x1000000;
		i += 4;
		if (i + blocksize > e && i < e && blocktype != 0)
		{
			global::error = fmt::format("VOC file cut abruptly in block {}", blockcount);
			return false;
		}
		blockcount++;
		switch (blocktype)
		{
		case 0: // Terminator, the rest should be ignored
			i = e;
			break;
		case 1: // Sound data
			if (!gotextra && codec >= 0 && codec != in[i + 1])
			{
				global::error = "VOC files with different codecs are not supported";
				return false;
			}
			else if (codec == -1)
			{
				fmtchunk.samplerate = 1000000 / (256 - in[i]);
				fmtchunk.channels   = 1;
				fmtchunk.tag        = 1;
				codec               = in[i + 1];
			}
			datasize += blocksize - 2;
			break;
		case 2: // Sound data continuation
			if (codec == -1)
			{
				global::error = "Sound data without codec in VOC file";
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
				global::error = "Extra info block must precede sound data info block in VOC file";
				return false;
			}
			else
			{
				fmtchunk.samplerate = 256000000 / ((in[i + 3] + 1) * (65536 - in.readL16(i)));
				fmtchunk.channels   = in[i + 3] + 1;
				fmtchunk.tag        = 1;
				codec               = in[i + 2];
			}
			break;
		case 9: // Sound data in new format
			if (codec >= 0 && codec != in.readL16(i + 6))
			{
				global::error = "VOC files with different codecs are not supported";
				return false;
			}
			else if (codec == -1)
			{
				fmtchunk.samplerate = in.readL32(i);
				fmtchunk.bps        = in[i + 4];
				fmtchunk.channels   = in[i + 5];
				fmtchunk.tag        = 1;
				codec               = in.readL16(i + 6);
			}
			datasize += blocksize - 12;
			break;
		default: break;
		}
		i += blocksize;
	}
	wdhdr.size = datasize;
	switch (codec)
	{
	case 0: // 8 bits unsigned PCM
		fmtchunk.bps       = 8;
		fmtchunk.datarate  = fmtchunk.samplerate;
		fmtchunk.blocksize = 1;
		break;
	case 4: // 16 bits signed PCM
		fmtchunk.bps       = 16;
		fmtchunk.datarate  = fmtchunk.samplerate << 1;
		fmtchunk.blocksize = 2;
		break;
	case 1:     // 4 bits to 8 bits Creative ADPCM
	case 2:     // 3 bits to 8 bits Creative ADPCM (AKA 2.6 bits)
	case 3:     // 2 bits to 8 bits Creative ADPCM
	case 6:     // alaw
	case 7:     // ulaw
	case 0x200: // 4 bits to 16 bits Creative ADPCM (only valid in block type 0x09)
		global::error = fmt::format("Unsupported codec {} in VOC file", codec);
		return false;
	default: global::error = fmt::format("Unknown codec {} in VOC file", codec); return false;
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
	whdr.size = wdhdr.size + fmtchunk.header.size + 20;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(WavFmtChunk));
	out.write(&wdhdr, 8);

	// Now go and copy sound data
	const uint8_t* src = in.data();
	i                  = 26;
	while (i < e)
	{
		// Parses through blocks again
		uint8_t blocktype = in[i];
		size_t  blocksize = in.readL24(i + 1);
		i += 4;
		switch (blocktype)
		{
		case 1: // Sound data
			out.write(src + i + 2, blocksize - 2);
			break;
		case 2: // Sound data continuation
			out.write(src + i, blocksize);
			break;
		case 3: // Silence
			// Not supported yet
			break;
		case 9: // Sound data in new format
			out.write(src + i + 12, blocksize - 12);
		default: break;
		}
		i += blocksize;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Blood SFX files to wav format
// -----------------------------------------------------------------------------
bool conversion::bloodToWav(const ArchiveEntry* in, MemChunk& out)
{
	auto& mc = in->data();
	if (mc.size() < 22 || mc.size() > 29 || ((mc[12] != 1 && mc[12] != 5) || mc[mc.size() - 1] != 0))
	{
		global::error = "Invalid SFX";
		return false;
	}
	string name;
	for (size_t i = 20; i < mc.size() - 1; ++i)
	{
		// Check that the entry does give a purely alphanumeric ASCII name
		if ((mc[i] < '0' || (mc[i] > '9' && mc[i] < 'A') || (mc[i] > 'Z' && mc[i] < 'a') || mc[i] > 'z')
			&& mc[i] != '_')
		{
			global::error = "Invalid SFX";
			return false;
		}
		else
			name += mc[i];
	}

	// Find raw data
	name += ".raw";
	auto raw = in->parent()->entry(name);
	if (!raw || raw->size() == 0)
	{
		global::error = "No RAW data for SFX";
		return false;
	}

	size_t rawsize = raw->size();

	// --- Write WAV ---
	WavChunk    whdr, wdhdr;
	WavFmtChunk fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = rawsize;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag         = 1;
	fmtchunk.channels    = 1;
	fmtchunk.samplerate  = mc[12] == 5 ? 22050 : 11025;
	fmtchunk.datarate    = fmtchunk.samplerate;
	fmtchunk.blocksize   = 1;
	fmtchunk.bps         = 8;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 20;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(WavFmtChunk));
	out.write(&wdhdr, 8);
	out.write(raw->rawData(), raw->size());

	return true;
}

// -----------------------------------------------------------------------------
// Converts Wolf3D sound data [in] to wav format, written to [out]
// -----------------------------------------------------------------------------
bool conversion::wolfSndToWav(const MemChunk& in, MemChunk& out)
{
	// --- Read Wolf sound ---

	// Read samples
	size_t         numsamples = in.size();
	const uint8_t* samples    = in.data();

	// --- Write WAV ---

	WavChunk    whdr, wdhdr;
	WavFmtChunk fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = numsamples;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag         = 1;
	fmtchunk.channels    = 1;
	fmtchunk.samplerate  = wolfsnd_rate;
	fmtchunk.datarate    = wolfsnd_rate;
	fmtchunk.blocksize   = 1;
	fmtchunk.bps         = 8;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 20;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(WavFmtChunk));
	out.write(&wdhdr, 8);
	out.write(samples, numsamples);

	// Ensure data ends on even byte boundary
	if (numsamples % 2 != 0)
		out.write("\0", 1);

	return true;
}

// -----------------------------------------------------------------------------
// Converts Jaguar Doom sound data [in] to wav format, written to [out]
// -----------------------------------------------------------------------------
bool conversion::jagSndToWav(const MemChunk& in, MemChunk& out)
{
	// --- Read Jaguar Doom sound ---

	// Read Jaguar doom sound header
	JSndHeader header;
	in.seek(0, SEEK_SET);
	in.read(&header, 28);

	// Correct endianness for the one value we actually use
	// (The rest of the header is in big endian format too, but we
	//  don't use these values so we don't need to correct them.)
	header.samples = wxINT32_SWAP_ON_LE(header.samples);

	// Format checks
	if (header.samples > (in.size() - 28) || header.samples <= 4) // Check for sane values
	{
		global::error = "Invalid Jaguar Doom Sound";
		return false;
	}

	// Read samples
	vector<uint8_t> samples(header.samples);
	in.read(samples.data(), header.samples);


	// --- Write WAV ---

	WavChunk    whdr, wdhdr;
	WavFmtChunk fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = header.samples;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag         = 1;
	fmtchunk.channels    = 1;
	fmtchunk.samplerate  = 11025;
	fmtchunk.datarate    = 11025;
	fmtchunk.blocksize   = 1;
	fmtchunk.bps         = 8;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 20;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(WavFmtChunk));
	out.write(&wdhdr, 8);
	out.write(samples.data(), header.samples);

	// Ensure data ends on even byte boundary
	if (header.samples % 2 != 0)
		out.write("\0", 1);

	return true;
}

// -----------------------------------------------------------------------------
// Dark Forces GMID file to Standard MIDI File
// -----------------------------------------------------------------------------
bool conversion::gmidToMidi(const MemChunk& in, MemChunk& out)
{
	// Skip beginning of file and look for MThd chunk
	// (the standard MIDI header)
	size_t size = in.size();
	if (size < 16)
		return false;
	if (in[0] != 'M' && in[1] != 'I' && in[2] != 'D' && in[3] != 'I' && ((in.readB32(4) + 8) != size))
		return false;

	size_t offset   = 8;
	bool   notfound = true;
	while (notfound)
	{
		if (offset + 8 > size)
			return false;
		// Look for header
		if (in[offset] == 'M' && in[offset + 1] == 'T' && in[offset + 2] == 'h' && in[offset + 3] == 'd')
			notfound = false;
		else
			offset += (in.readB32(offset + 4) + 8);
	}

	// Write the rest of the file
	out.write(in.data() + offset, size - offset);

	return true;
}

// -----------------------------------------------------------------------------
// RMID file to Standard MIDI File
// -----------------------------------------------------------------------------
bool conversion::rmidToMidi(const MemChunk& in, MemChunk& out)
{
	// Skip beginning of file and look for MThd chunk
	// (the standard MIDI header)
	size_t size = in.size();
	if (size < 36)
		return false;
	if (in[0] != 'R' && in[1] != 'I' && in[2] != 'F' && in[3] != 'F' && ((in.readL32(4) + 8) != size))
		return false;

	size_t offset   = 12;
	size_t datasize = 0;
	bool   notfound = true;
	while (notfound)
	{
		if (offset + 20 > size)
			return false;
		// Look for header
		if (in[offset] == 'd' && in[offset + 1] == 'a' && in[offset + 2] == 't' && in[offset + 3] == 'a'
			&& in[offset + 8] == 'M' && in[offset + 9] == 'T' && in[offset + 10] == 'h' && in[offset + 11] == 'd')
		{
			notfound = false;
			datasize = in.readL32(offset + 4);
			offset += 8;
		}
		else
			offset += (in.readL32(offset + 4) + 8);
	}

	// Write the rest of the file
	if (offset + datasize <= size)
	{
		out.write(in.data() + offset, datasize);
		return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
// Automatizes this: http://zdoom.org/wiki/Using_OPL_music_in_ZDoom
// -----------------------------------------------------------------------------
bool conversion::addImfHeader(const MemChunk& in, MemChunk& out)
{
	if (in.size() == 0)
		return false;

	uint32_t newsize = in.size() + 9;
	uint8_t  start   = 0;
	if (in[0] | in[1])
	{
		// non-zero start
		newsize += 2;
		start = 2;
	}
	else
		newsize += 4;

	out.reSize(newsize, false);
	out[0] = 'A';
	out[1] = 'D';
	out[2] = 'L';
	out[3] = 'I';
	out[4] = 'B';
	out[5] = 1;
	out[6] = 0;
	out[7] = 0;
	out[8] = 1;
	if (in[0] | in[1])
	{
		out[9]  = in[0];
		out[10] = in[1];
		out[11] = 0;
		out[12] = 0;
	}
	else
	{
		out[9]  = 0;
		out[10] = 0;
		out[11] = 0;
		out[12] = 0;
	}
	out.seek(13, SEEK_SET);
	in.seek(start, SEEK_SET);
	// size_t size = MIN(in.getSize() - start, newsize - 13);
	// return in.readMC(out, size);
	for (size_t i = 0; ((i + start < in.size()) && (13 + i < newsize)); ++i)
	{
		out[13 + i] = in[i + start];
	}
	return true;
}

// -----------------------------------------------------------------------------
// Converts Doom PC speaker sound data [in] to wav format, written to [out].
//
// This code is partly adapted from info found on:
// http://www.shikadi.net/moddingwiki/AudioT_Format and
// http://www.shikadi.net/moddingwiki/Inverse_Frequency_Sound_format
// -----------------------------------------------------------------------------
bool conversion::spkSndToWav(const MemChunk& in, MemChunk& out, bool audioT)
{
	constexpr double   ORIG_RATE     = 140.0;
	constexpr int      FACTOR        = 315; // 315*140 = 44100
	constexpr double   FREQ          = 1193181.0;
	constexpr double   RATE          = (ORIG_RATE * FACTOR);
	constexpr int      PC_VOLUME     = 20;
	constexpr uint16_t counters[128] = { 0,    6818, 6628, 6449, 6279, 6087, 5906, 5736, 5575, 5423, 5279, 5120, 4971,
										 4830, 4697, 4554, 4435, 4307, 4186, 4058, 3950, 3836, 3728, 3615, 3519, 3418,
										 3323, 3224, 3131, 3043, 2960, 2875, 2794, 2711, 2633, 2560, 2485, 2415, 2348,
										 2281, 2213, 2153, 2089, 2032, 1975, 1918, 1864, 1810, 1757, 1709, 1659, 1612,
										 1565, 1521, 1478, 1435, 1395, 1355, 1316, 1280, 1242, 1207, 1173, 1140, 1107,
										 1075, 1045, 1015, 986,  959,  931,  905,  879,  854,  829,  806,  783,  760,
										 739,  718,  697,  677,  658,  640,  621,  604,  586,  570,  553,  538,  522,
										 507,  493,  479,  465,  452,  439,  427,  415,  403,  391,  380,  369,  359,
										 348,  339,  329,  319,  310,  302,  293,  285,  276,  269,  261,  253,  246,
										 239,  232,  226,  219,  213,  207,  201,  195,  190,  184,  179 };

	// --- Read Doom sound ---
	// -- Also AudioT sound --

	size_t minsize = 4 + (audioT ? 3 : 0);
	if (in.size() < minsize)
	{
		global::error = "Invalid PC Speaker Sound";
		return false;
	}

	// Read doom sound header
	SpkSndHeader header;
	in.seek(0, SEEK_SET);
	in.read(&header, 4);
	size_t numsamples;

	// Format checks
	if (audioT)
	{
		numsamples = in.readL32(0);
		uint16_t priority;
		in.read(&priority, 2);
		if (in.size() < 6 + numsamples)
		{
			global::error = "Invalid AudioT PC Speaker Sound";
			return false;
		}
	}
	else
	{
		if (header.zero != 0) // Check for magic number
		{
			global::error = "Invalid Doom PC Speaker Sound";
			return false;
		}
		if (header.samples > (in.size() - 4) || header.samples < 4) // Check for sane values
		{
			global::error = "Invalid Doom PC Speaker Sound";
			return false;
		}
		numsamples = header.samples;
	}

	// Read samples
	vector<uint8_t> osamples(numsamples);
	vector<uint8_t> nsamples(numsamples * FACTOR);
	in.read(osamples.data(), numsamples);

	int      sign      = -1;
	uint32_t phase_tic = 0;

	// Convert counter values to sample values
	for (size_t s = 0; s < numsamples; ++s)
	{
		if (osamples[s] > 127 && !audioT)
		{
			global::error = fmt::format("Invalid PC Speaker counter value: {} > 127", osamples[s]);
			return false;
		}
		if (osamples[s] > 0)
		{
			// First, convert counter value to frequency in Hz
			// double f = FREQ / (double)counters[osamples[s]];
			uint32_t tone         = audioT ? osamples[s] * 60 : counters[osamples[s]];
			uint32_t phase_length = (tone * RATE) / (2 * FREQ);

			// Then write a bunch of samples.
			for (int i = 0; i < FACTOR; ++i)
			{
				// Finally, convert frequency into sample value
				int pos       = (s * FACTOR) + i;
				nsamples[pos] = 128 + sign * PC_VOLUME;
				if (phase_tic++ >= phase_length)
				{
					sign      = -sign;
					phase_tic = 0;
				}
			}
		}
		else
		{
			memset(nsamples.data() + static_cast<size_t>(s * FACTOR), 128, FACTOR);
			phase_tic = 0;
		}
	}

	// --- Write WAV ---

	WavChunk    whdr, wdhdr;
	WavFmtChunk fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = numsamples * FACTOR;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag         = 1;
	fmtchunk.channels    = 1;
	fmtchunk.samplerate  = RATE;
	fmtchunk.datarate    = RATE;
	fmtchunk.blocksize   = 1;
	fmtchunk.bps         = 8;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 20;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(WavFmtChunk));
	out.write(&wdhdr, 8);
	out.write(nsamples.data(), numsamples * FACTOR);

	// Ensure data ends on even byte boundary
	if (header.samples % 2 != 0)
		out.write("\0", 1);

	return true;
}

// -----------------------------------------------------------------------------
// Converts Sun/NeXT sound data [in] to wav format, written to [out]
// -----------------------------------------------------------------------------
bool conversion::auSndToWav(const MemChunk& in, MemChunk& out)
{
	// --- Read Sun sound ---

	// Read doom sound header
	SunSndHeader header;
	in.seek(0, SEEK_SET);
	in.read(&header, 24);

	header.magic    = wxUINT32_SWAP_ON_LE(header.magic);
	header.offset   = wxUINT32_SWAP_ON_LE(header.offset);
	header.size     = wxUINT32_SWAP_ON_LE(header.size);
	header.format   = wxUINT32_SWAP_ON_LE(header.format);
	header.rate     = wxUINT32_SWAP_ON_LE(header.rate);
	header.channels = wxUINT32_SWAP_ON_LE(header.channels);

	// Format checks
	if (header.magic != 0x2E736E64) // ASCII code for ".snd"
	{
		global::error = "Invalid Sun Sound";
		return false;
	}
	// Only cover integer linear PCM for now
	if (header.format < 2 || header.format > 5)
	{
		global::error = fmt::format("Unsupported Sun Sound format ({})", header.format);
		return false;
	}
	uint8_t samplesize = header.format - 1;
	if (header.format == 1)
		samplesize = 1;
	else if (header.format >= 6)
		--samplesize;

	// Read samples
	vector<uint8_t> samples(header.size);
	in.read(samples.data(), header.size);

	// Swap endianness around if needed
	if (samplesize > 1)
	{
		uint8_t swapval = 0;
		for (size_t i = 0; i < header.size / samplesize; ++i)
		{
			switch (samplesize)
			{
			case 2:
				swapval            = samples[i * 2];
				samples[i * 2]     = samples[i * 2 + 1];
				samples[i * 2 + 1] = swapval;
				break;
			case 3:
				swapval            = samples[i * 3];
				samples[i * 3]     = samples[i * 3 + 2];
				samples[i * 3 + 2] = swapval;
				break;
			case 4:
				swapval            = samples[i * 4];
				samples[i * 4]     = samples[i * 4 + 3];
				samples[i * 4 + 3] = swapval;
				swapval            = samples[i * 4 + 2];
				samples[i * 4 + 2] = samples[i * 4 + 1];
				samples[i * 4 + 1] = swapval;
				break;
			default: break;
			}
		}
	}

	// --- Write WAV ---

	WavChunk    whdr, wdhdr;
	WavFmtChunk fmtchunk;

	// Setup data header
	char did[4] = { 'd', 'a', 't', 'a' };
	memcpy(&wdhdr.id, &did, 4);
	wdhdr.size = header.size;

	// Setup fmt chunk
	char fid[4] = { 'f', 'm', 't', ' ' };
	memcpy(&fmtchunk.header.id, &fid, 4);
	fmtchunk.header.size = 16;
	fmtchunk.tag         = 1;
	fmtchunk.channels    = header.channels;
	fmtchunk.samplerate  = header.rate;
	fmtchunk.datarate    = header.rate * header.channels;
	fmtchunk.blocksize   = samplesize;
	fmtchunk.bps         = 8 * samplesize;

	// Setup main header
	char wid[4] = { 'R', 'I', 'F', 'F' };
	memcpy(&whdr.id, &wid, 4);
	whdr.size = wdhdr.size + fmtchunk.header.size + 20;

	// Write chunks
	out.write(&whdr, 8);
	out.write("WAVE", 4);
	out.write(&fmtchunk, sizeof(WavFmtChunk));
	out.write(&wdhdr, 8);
	out.write(samples.data(), header.size);

	// Ensure data ends on even byte boundary
	if (header.size % 2 != 0)
		out.write("\0", 1);

	return true;
}

bool conversion::voxToKvx(const MemChunk& in, MemChunk& out)
{
#define AT(x, y, z) (((x) * length + (y)) * height + (z))

	constexpr uint8_t LEFT   = 1;
	constexpr uint8_t RIGHT  = 2;
	constexpr uint8_t FRONT  = 4;
	constexpr uint8_t BACK   = 8;
	constexpr uint8_t TOP    = 16;
	constexpr uint8_t BOTTOM = 32;

	VoxHeader voxHeader;
	in.seek(0, SEEK_SET);
	in.read(&voxHeader, sizeof(voxHeader));


	voxHeader.width  = wxUINT32_SWAP_ON_BE(voxHeader.width);
	voxHeader.length = wxUINT32_SWAP_ON_BE(voxHeader.length);
	voxHeader.height = wxUINT32_SWAP_ON_BE(voxHeader.height);

	uint32_t width  = voxHeader.width;
	uint32_t length = voxHeader.length;
	uint32_t height = voxHeader.height;


	uint8_t* voxels = new uint8_t[width * length * height];
	in.read(voxels, width * length * height);

	uint8_t* visibilities = new uint8_t[width * length * height]{ 0 };

	for (uint32_t x = 0; x < width; x++)
	{
		for (uint32_t y = 0; y < length; y++)
		{
			for (uint32_t z = 0; z < height; z++)
			{
				if (voxels[AT(x, y, z)] == 255)
					continue;

				if (x == 0 || voxels[AT(x - 1, y, z)] == 255)
					visibilities[AT(x, y, z)] |= LEFT;
				if (x == width - 1 || voxels[AT(x + 1, y, z)] == 255)
					visibilities[AT(x, y, z)] |= RIGHT;
				if (y == 0 || voxels[AT(x, y - 1, z)] == 255)
					visibilities[AT(x, y, z)] |= FRONT;
				if (y == length - 1 || voxels[AT(x, y + 1, z)] == 255)
					visibilities[AT(x, y, z)] |= BACK;
				if (z == 0 || voxels[AT(x, y, z - 1)] == 255)
					visibilities[AT(x, y, z)] |= TOP;
				if (z == height - 1 || voxels[AT(x, y, z + 1)] == 255)
					visibilities[AT(x, y, z)] |= BOTTOM;
			}
		}
	}

	for (uint32_t x = 0; x < width; x++)
	{
		for (uint32_t y = 0; y < length; y++)
		{
			for (uint32_t z = 0; z < height; z++)
			{
				if (visibilities[AT(x, y, z)] == 0)
				{
					voxels[AT(x, y, z)] = 255;
				}
			}
		}
	}
	uint8_t* palette = new uint8_t[768];
	in.read(palette, 768);

	uint32_t* xoffsets  = new uint32_t[width + 1];
	uint16_t* xyoffsets = new uint16_t[width * (length + 1)];

	out.reSize((width + 1) * sizeof(uint32_t) + width * (length + 1) * sizeof(uint16_t) + sizeof(KvxHeader) + 1);
	out.seek((width + 1) * sizeof(uint32_t) + width * (length + 1) * sizeof(uint16_t) + sizeof(KvxHeader), SEEK_SET);
	xoffsets[0] = out.currentPos() - sizeof(KvxHeader);

	log::console(fmt::format("KVX: {} {}", xoffsets[0], out.currentPos()));

	vector<uint8_t> post_colors;
	post_colors.reserve(256);

	for (uint32_t x = 0; x < width; x++)
	{
		xyoffsets[x * (length + 1)] = out.currentPos() - xoffsets[x] - sizeof(KvxHeader);
		for (uint32_t y = 0; y < length; y++)
		{
			for (uint32_t z = 0; z < height; z++)
			{
				uint8_t color      = voxels[AT(x, y, z)];
				uint8_t visibility = visibilities[AT(x, y, z)];
				bool    at_end     = (z == height - 1);

				if (color != 255)
				{
					post_colors.push_back(color);
				}


				bool must_write_post =
					(!post_colors.empty()
					 && (at_end || voxels[AT(x, y, z + 1)] == 255 || visibilities[AT(x, y, z + 1)] != visibility
						 || post_colors.size() == 255));


				if (must_write_post)
				{
					KvxColumnPostHeader postHeader{ static_cast<uint8_t>(z - (post_colors.size() - 1)),
													static_cast<uint8_t>(post_colors.size()),
													visibility };
					out.write(&postHeader, sizeof(KvxColumnPostHeader));
					out.write(post_colors.data(), post_colors.size());
					post_colors.clear();
				}
			}
			xyoffsets[x * (length + 1) + y + 1] = out.currentPos() - xoffsets[x] - sizeof(KvxHeader);
		}
		xoffsets[x + 1] = out.currentPos() - sizeof(KvxHeader);
	}

	uint32_t total_bytes = out.currentPos() - sizeof(uint32_t);
	log::console(fmt::format("Total size: {}", total_bytes));

	out.write(palette, 768);

	log::console("XOFFSETS");
	for (int x = 0; x < width + 1; x++)
	{
		log::console(fmt::format("xoffsets[{}]: {}", x, xoffsets[x]));
	}

	log::console("XYOFFSETS");
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < length + 1; y++)
		{
			log::console(
				fmt::format(
					"xyoffsets[{}][{}]: {}, total: {}",
					x,
					y,
					xyoffsets[x * (length + 1) + y],
					xoffsets[x] + xyoffsets[x * (length - 1) + y]));
		}
	}

	KvxHeader kvxHeader{ total_bytes, width, length, height, width << 7, length << 7, height << 8 };

	out.seek(0, SEEK_SET);
	out.write(&kvxHeader, sizeof(KvxHeader));
	out.write(xoffsets, (width + 1) * sizeof(uint32_t));
	out.write(xyoffsets, width * (length + 1) * sizeof(uint16_t));

	delete[] xoffsets;
	delete[] xyoffsets;
	delete[] voxels;
	delete[] visibilities;

	return true;

#undef AT
}
