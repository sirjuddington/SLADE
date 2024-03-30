//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
// Copyright(C) 2006 Ben Ryves 2006
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// mus2mid.c - Ben Ryves 2006 - http://benryves.com - benryves@benryves.com
// Use to convert a MUS file into a single track, type 0 MIDI file.
//
// mus2mid.cpp adapted to C++ and SLADE 3 classes, work with MemChunks
// instead of files. Most boolean functions had their "polarity" reversed:
// the original code returned true on failure, false on success. Now this
// consistently return true on success and false if there were a problem.

#include "Main.h"
#include "Mus2Mid.h"

using namespace slade;

// MUS event codes
enum musevent
{
	mus_releasekey       = 0x00,
	mus_presskey         = 0x10,
	mus_pitchwheel       = 0x20,
	mus_systemevent      = 0x30,
	mus_changecontroller = 0x40,
	mus_scoreend         = 0x60
};

// MIDI event codes
enum midievent
{
	midi_releasekey        = 0x80,
	midi_presskey          = 0x90,
	midi_aftertouchkey     = 0xA0,
	midi_changecontroller  = 0xB0,
	midi_changepatch       = 0xC0,
	midi_aftertouchchannel = 0xD0,
	midi_pitchwheel        = 0xE0
};

// Structure to hold MUS file header
#pragma pack(1)
struct musheader
{
	uint8_t        id[4];
	unsigned short scorelength;
	unsigned short scorestart;
	unsigned short primarychannels;
	unsigned short secondarychannels;
	unsigned short instrumentcount;
};
#pragma pack()

// Standard MIDI type 0 header + track header
const uint8_t midiheader[] = {
	'M',  'T',  'h',  'd',  // Main header
	0x00, 0x00, 0x00, 0x06, // Header size
	0x00, 0x00,             // MIDI type (0)
	0x00, 0x01,             // Number of tracks
	0x00, 0x46,             // Resolution
	'M',  'T',  'r',  'k',  // Start of track
	0x00, 0x00, 0x00, 0x00  // Placeholder for track length
};

// Constants
#define NUM_CHANNELS 16
#define MUS_PERCUSSION_CHAN 15
#define MIDI_PERCUSSION_CHAN 9
#define MIDI_TRACKLENGTH_OFS 18

// Cached channel velocities
uint8_t channelvelocities[] = { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 };

// Timestamps between sequences of MUS events
static unsigned int queuedtime = 0;

// Counter for the length of the track
static unsigned int tracksize;

const uint8_t controller_map[] = { 0x00, 0x20, 0x01, 0x07, 0x0A, 0x0B, 0x5B, 0x5D,
								   0x40, 0x43, 0x78, 0x7B, 0x7E, 0x7F, 0x79 };

int channel_map[NUM_CHANNELS];

// Write timestamp to a MIDI file.
static bool WriteTime(uint32_t time, MemChunk& midioutput)
{
	uint32_t buffer = time & 0x7F;
	uint8_t  writeval;

	while ((time >>= 7) != 0)
	{
		buffer <<= 8;
		buffer |= ((time & 0x7F) | 0x80);
	}

	for (;;)
	{
		writeval = (uint8_t)(buffer & 0xFF);

		if (!midioutput.write(&writeval, 1))
			return false;

		++tracksize;

		if ((buffer & 0x80) != 0)
			buffer >>= 8;
		else
		{
			queuedtime = 0;
			return true;
		}
	}
}

// Write the end of track marker
static bool WriteEndTrack(MemChunk& midioutput)
{
	uint8_t endtrack[] = { 0xFF, 0x2F, 0x00 };

	if (!WriteTime(queuedtime, midioutput))
		return false;

	if (!midioutput.write(endtrack, 3))
		return false;

	tracksize += 3;
	return true;
}

// Write a key press event
static bool WritePressKey(uint8_t channel, uint8_t key, uint8_t velocity, MemChunk& midioutput)
{
	// Write queued time
	if (!WriteTime(queuedtime, midioutput))
		return false;

	// Write pressed key and channel
	uint8_t working = midi_presskey | channel;
	if (!midioutput.write(&working, 1))
		return false;

	// Write key
	working = key & 0x7F;
	if (!midioutput.write(&working, 1))
		return false;

	// Wite velocity
	working = velocity & 0x7F;
	if (!midioutput.write(&working, 1))
		return false;

	tracksize += 3;

	return true;
}

// Write a key release event
static bool WriteReleaseKey(uint8_t channel, uint8_t key, MemChunk& midioutput)
{
	// Write queued time
	if (!WriteTime(queuedtime, midioutput))
		return false;

	// Write released key
	uint8_t working = midi_releasekey | channel;
	if (!midioutput.write(&working, 1))
		return false;

	// Write key
	working = key & 0x7F;
	if (!midioutput.write(&working, 1))
		return false;

	// Dummy
	working = 0;
	if (!midioutput.write(&working, 1))
		return false;

	tracksize += 3;

	return true;
}

// Write a pitch wheel/bend event
static bool WritePitchWheel(uint8_t channel, int16_t wheel, MemChunk& midioutput)
{
	// Write queued time
	if (!WriteTime(queuedtime, midioutput))
		return false;

	uint8_t working = midi_pitchwheel | channel;
	if (!midioutput.write(&working, 1))
		return false;

	working = wheel & 0x7F;
	if (!midioutput.write(&working, 1))
		return false;

	working = (wheel >> 7) & 0x7F;
	if (!midioutput.write(&working, 1))
		return false;

	tracksize += 3;
	return true;
}

// Write a patch change event
static bool WriteChangePatch(uint8_t channel, uint8_t patch, MemChunk& midioutput)
{
	// Write queued time
	if (!WriteTime(queuedtime, midioutput))
		return false;

	uint8_t working = midi_changepatch | channel;
	if (!midioutput.write(&working, 1))
		return false;

	working = patch & 0x7F;
	if (!midioutput.write(&working, 1))
		return false;

	tracksize += 2;

	return true;
}

// Write a valued controller change event

static bool WriteChangeController_Valued(uint8_t channel, uint8_t control, uint8_t value, MemChunk& midioutput)
{
	// Write queued time
	if (!WriteTime(queuedtime, midioutput))
		return false;

	uint8_t working = midi_changecontroller | channel;
	if (!midioutput.write(&working, 1))
		return false;

	working = control & 0x7F;
	if (!midioutput.write(&working, 1))
		return false;

	// Quirk in vanilla DOOM? MUS controller values should be 7-bit, not 8-bit.
	working = value & 0x80 ? 0x7F : value;
	if (!midioutput.write(&working, 1))
		return false;

	tracksize += 3;

	return true;
}

// Write a valueless controller change event
static bool WriteChangeController_Valueless(uint8_t channel, uint8_t control, MemChunk& midioutput)
{
	return WriteChangeController_Valued(channel, control, 0, midioutput);
}

// Allocate a free MIDI channel.
static int AllocateMIDIChannel()
{
	int result;
	int max;
	int i;

	// Find the current highest-allocated channel.

	max = -1;

	for (i = 0; i < NUM_CHANNELS; ++i)
	{
		if (channel_map[i] > max)
		{
			max = channel_map[i];
		}
	}

	// max is now equal to the highest-allocated MIDI channel.  We can
	// now allocate the next available channel.  This also works if
	// no channels are currently allocated (max=-1)

	result = max + 1;

	// Don't allocate the MIDI percussion channel!

	if (result == MIDI_PERCUSSION_CHAN)
	{
		++result;
	}

	return result;
}

// Given a MUS channel number, get the MIDI channel number to use in the outputted file.
static int GetMIDIChannel(int mus_channel)
{
	// Find the MIDI channel to use for this MUS channel.
	// MUS channel 15 is the percusssion channel.

	if (mus_channel == MUS_PERCUSSION_CHAN)
	{
		return MIDI_PERCUSSION_CHAN;
	}
	else
	{
		// If a MIDI channel hasn't been allocated for this MUS channel
		// yet, allocate the next free MIDI channel.

		if (channel_map[mus_channel] == -1)
		{
			channel_map[mus_channel] = AllocateMIDIChannel();
		}

		return channel_map[mus_channel];
	}
}

static bool ReadMusHeader(const MemChunk& file, musheader* header)
{
	if (file.size() >= sizeof(musheader))
	{
		const uint8_t* data = file.data();
		memcpy(header, data, sizeof(musheader));
		return true;
	}
	return false;
}


// Read a MUS file from a MemChunk (musinput) and output a MIDI file to
// a MemChunk (midioutput).
//
// Returns true if successful, false otherwise

bool slade::mus2mid(const MemChunk& musinput, MemChunk& midioutput)
{
	// Header for the MUS file
	musheader musfileheader;

	// Descriptor for the current MUS event
	uint8_t eventdescriptor;
	int     channel; // Channel number
	int     mus_event;


	// Bunch of vars read from MUS lump
	uint8_t key;
	uint8_t controllernumber;
	uint8_t controllervalue;

	// Buffer used for MIDI track size record
	uint8_t tracksizebuffer[4];

	// Flag for when the score end marker is hit.
	int hitscoreend = 0;

	// Temp working byte
	uint8_t working;
	// Used in building up time delays
	unsigned int timedelay;

	// Initialise channel map to mark all channels as unused.
	for (channel = 0; channel < NUM_CHANNELS; ++channel)
	{
		channel_map[channel] = -1;
	}

	// Grab the header
	if (!ReadMusHeader(musinput, &musfileheader))
		return false;

	// Check MUS header
	if (musfileheader.id[0] != 'M' || musfileheader.id[1] != 'U' || musfileheader.id[2] != 'S'
		|| musfileheader.id[3] != 0x1A)
	{
		return false;
	}

	// Seek to where the data is held
	if (!musinput.seek((long)musfileheader.scorestart, SEEK_SET))
		return false;

	// So, we can assume the MUS file is faintly legit. Let's start writing MIDI data...

	midioutput.clear();
	midioutput.write(midiheader, sizeof(midiheader));
	tracksize = 0;

	// Now, process the MUS file:
	while (!hitscoreend)
	{
		// Handle a block of events:

		while (!hitscoreend)
		{
			// Fetch channel number and event code:

			if (!musinput.read(&eventdescriptor, 1))
				return false;

			channel   = GetMIDIChannel(eventdescriptor & 0x0F);
			mus_event = eventdescriptor & 0x70;

			switch (mus_event)
			{
			case mus_releasekey:
				if (!musinput.read(&key, 1))
					return false;

				if (!WriteReleaseKey(channel, key, midioutput))
					return false;

				break;

			case mus_presskey:
				if (!musinput.read(&key, 1))
					return false;

				if (key & 0x80)
				{
					if (!musinput.read(&channelvelocities[channel], 1))
						return false;

					channelvelocities[channel] &= 0x7F;
				}

				if (!WritePressKey(channel, key, channelvelocities[channel], midioutput))
					return false;

				break;

			case mus_pitchwheel:
				if (!musinput.read(&key, 1))
					break;

				if (!WritePitchWheel(channel, (short)(key * 64), midioutput))
					return false;

				break;

			case mus_systemevent:
				if (!musinput.read(&controllernumber, 1))
					return false;

				if (controllernumber < 10 || controllernumber > 14)
					return false;

				if (!WriteChangeController_Valueless(channel, controller_map[controllernumber], midioutput))
					return false;

				break;

			case mus_changecontroller:
				if (!musinput.read(&controllernumber, 1))
					return false;

				if (!musinput.read(&controllervalue, 1))
					return false;

				if (controllernumber == 0)
				{
					if (!WriteChangePatch(channel, controllervalue, midioutput))
						return false;
				}
				else
				{
					if (controllernumber < 1 || controllernumber > 9)
						return false;

					if (!WriteChangeController_Valued(
							channel, controller_map[controllernumber], controllervalue, midioutput))
						return false;
				}

				break;

			case mus_scoreend: hitscoreend = 1; break;

			default:           return false; break;
			}

			if (eventdescriptor & 0x80)
				break;
		}
		// Now we need to read the time code:
		if (!hitscoreend)
		{
			timedelay = 0;
			for (;;)
			{
				if (!musinput.read(&working, 1))
					return false;

				timedelay = timedelay * 128 + (working & 0x7F);
				if ((working & 0x80) == 0)
					break;
			}
			queuedtime += timedelay;
		}
	}

	// End of track
	if (!WriteEndTrack(midioutput))
	{
		return false;
	}

	// Write the track size into the stream
	if (!midioutput.seek(MIDI_TRACKLENGTH_OFS, SEEK_SET))
		return false;

	tracksizebuffer[0] = (tracksize >> 24) & 0xff;
	tracksizebuffer[1] = (tracksize >> 16) & 0xff;
	tracksizebuffer[2] = (tracksize >> 8) & 0xff;
	tracksizebuffer[3] = tracksize & 0xff;

	if (!midioutput.write(tracksizebuffer, 4))
		return false;

	return true;
}
