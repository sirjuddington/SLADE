/*
** music_mus_midiout.cpp
** Code to let ZDoom play MUS music through the MIDI streaming API.
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
*/

// HEADER FILES ------------------------------------------------------------

#include "i_musicinterns.h"
#include "templates.h"
#include "m_swap.h"

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

int MUSHeaderSearch(const uint8_t *head, int len);

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static const uint8_t CtrlTranslate[15] =
{
	0,	// program change
	0,	// bank select
	1,	// modulation pot
	7,	// volume
	10, // pan pot
	11, // expression pot
	91, // reverb depth
	93, // chorus depth
	64, // sustain pedal
	67, // soft pedal
	120, // all sounds off
	123, // all notes off
	126, // mono
	127, // poly
	121, // reset all controllers
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
// MUSSong Constructor
//
// Performs some validity checks on the MUS file, buffers it, and creates
// the playback thread control events.
//
//==========================================================================

MUSSong::MUSSong (FILE *file, const uint8_t *musiccache, int len)
: MIDIStreamer(), MusHeader(0), MusBuffer(0)
{
	uint8_t front[32];
	int start;

	if (file == NULL)
	{
		memcpy(front, musiccache, sizeof(front));
	}
	else if (fread(front, 1, sizeof(front), file) != sizeof(front))
	{
		return;
	}

	// To tolerate sloppy wads (diescum.wad, I'm looking at you), we search
	// the first 32 uint8_ts of the file for a signature. My guess is that DMX
	// does no validation whatsoever and just assumes it was passed a valid
	// MUS file, since where the header is offset affects how it plays.
	start = MUSHeaderSearch(front, sizeof(front));
	if (start < 0)
	{
		return;
	}

	// Read the remainder of the song.
	len = int(len - start);
	if (len < (int)sizeof(MusHeader))
	{ // It's too short.
		return;
	}
	MusHeader = (MUSHeader *)new uint8_t[len];
	if (file == NULL)
	{
		memcpy(MusHeader, musiccache + start, len);
	}
	else
	{
		memcpy(MusHeader, front + start, sizeof(front) - start);
		if (fread((uint8_t *)MusHeader + sizeof(front) - start, 1, len - (sizeof(front) - start), file) != (size_t)(len - (32 - start)))
		{
			return;
		}
	}

	// Do some validation of the MUS file.
	if (LittleShort(MusHeader->NumChans) > 15)
	{
		return;
	}

	MusBuffer = (uint8_t *)MusHeader + LittleShort(MusHeader->SongStart);
	MaxMusP = MIN<int>(LittleShort(MusHeader->SongLen), len - LittleShort(MusHeader->SongStart));
	Division = 140;
	InitialTempo = 1000000;
}

//==========================================================================
//
// MUSSong Destructor
//
//==========================================================================

MUSSong::~MUSSong ()
{
	if (MusHeader != NULL)
	{
		delete[] (uint8_t *)MusHeader;
	}
}

//==========================================================================
//
// MUSSong :: DoRestart
//
// Rewinds the song.
//
//==========================================================================

void MUSSong::DoRestart()
{
	MusP = 0;
}

//==========================================================================
//
// MUSSong :: CheckDone
//
//==========================================================================

bool MUSSong::CheckDone()
{
	return MusP >= MaxMusP;
}

//==========================================================================
//
// MUSSong :: MakeEvents
//
// Translates MUS events into MIDI events and puts them into a MIDI stream
// buffer. Returns the new position in the buffer.
//
//==========================================================================

uint32_t *MUSSong::MakeEvents(uint32_t *events, uint32_t *max_event_p, uint32_t max_time)
{
	uint32_t tot_time = 0;
	uint32_t time = 0;

	max_time = max_time * Division / Tempo;

	while (events < max_event_p && tot_time <= max_time)
	{
		uint8_t mid1, mid2;
		uint8_t channel;
		uint8_t t = 0, status;
		uint8_t mevent = MusBuffer[MusP++];
		
		if ((mevent & 0x70) != MUS_SCOREEND)
		{
			t = MusBuffer[MusP++];
		}
		channel = mevent & 15;

		// Map MUS channels to MIDI channels
		if (channel == 15)
		{
			channel = 9;
		}
		else if (channel >= 9)
		{
			channel = channel + 1;
		}

		status = channel;

		switch (mevent & 0x70)
		{
		case MUS_NOTEOFF:
			status |= MIDI_NOTEON;
			mid1 = t;
			mid2 = 0;
			break;
			
		case MUS_NOTEON:
			status |= MIDI_NOTEON;
			mid1 = t & 127;
			if (t & 128)
			{
				LastVelocity[channel] = MusBuffer[MusP++];
			}
			mid2 = LastVelocity[channel];
			break;
			
		case MUS_PITCHBEND:
			status |= MIDI_PITCHBEND;
			mid1 = (t & 1) << 6;
			mid2 = (t >> 1) & 127;
			break;
			
		case MUS_SYSEVENT:
			status |= MIDI_CTRLCHANGE;
			mid1 = CtrlTranslate[t];
			mid2 = t == 12 ? LittleShort(MusHeader->NumChans) : 0;
			break;
			
		case MUS_CTRLCHANGE:
			if (t == 0)
			{ // program change
				status |= MIDI_PRGMCHANGE;
				mid1 = MusBuffer[MusP++];
				mid2 = 0;
			}
			else
			{
				status |= MIDI_CTRLCHANGE;
				mid1 = CtrlTranslate[t];
				mid2 = MusBuffer[MusP++];
				if (mid1 == 7)
				{ // Clamp volume to 127, since DMX apparently allows 8-bit volumes.
				  // Fix courtesy of Gez, courtesy of Ben Ryves.
					mid2 = VolumeControllerChange(channel, MIN<int>(mid2, 0x7F));
				}
			}
			break;
			
		case MUS_SCOREEND:
		default:
			MusP = MaxMusP;
			goto end;
		}

		events[0] = time;			// dwDeltaTime
		events[1] = 0;				// dwStreamID
		events[2] = status | (mid1 << 8) | (mid2 << 16);
		events += 3;

		time = 0;
		if (mevent & 128)
		{
			do
			{
				t = MusBuffer[MusP++];
				time = (time << 7) | (t & 127);
			}
			while (t & 128);
		}
		tot_time += time;
	}
end:
	if (time != 0)
	{
		events[0] = time;			// dwDeltaTime
		events[1] = 0;				// dwStreamID
		events[2] = MEVT_NOP << 24;	// dwEvent
		events += 3;
	}
	return events;
}

//==========================================================================
//
// MUSHeaderSearch
//
// Searches for the MUS header within the given memory block, returning
// the offset it was found at, or -1 if not present.
//
//==========================================================================

int MUSHeaderSearch(const uint8_t *head, int len)
{
	len -= 4;
	for (int i = 0; i <= len; ++i)
	{
		if (head[i+0] == 'M' && head[i+1] == 'U' && head[i+2] == 'S' && head[i+3] == 0x1A)
		{
			return i;
		}
	}
	return -1;
}
