/*
** music_midistream.cpp
** Implements base class for MIDI and MUS streaming.
**
**---------------------------------------------------------------------------
** Copyright 2008 Randy Heit
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
**
*/

// HEADER FILES ------------------------------------------------------------

#include "i_musicinterns.h"
#include "templates.h"
#include "m_swap.h"

// MACROS ------------------------------------------------------------------

#define MAX_TIME	(1000000/10)	// Send out 1/10 of a sec of events at a time.

#define EXPORT_LOOP_LIMIT	30		// Maximum number of times to loop when exporting a MIDI file.
									// (for songs with loop controller events)

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void WriteVarLen (TArray<uint8_t> &file, uint32_t value);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern char MIDI_EventLengths[7];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static const uint8_t StaticMIDIhead[] =
{
	'M','T','h','d', 0, 0, 0, 6,
	0, 0, // format 0: only one track
	0, 1, // yes, there is really only one track
	0, 0, // divisions (filled in)
	'M','T','r','k', 0, 0, 0, 0,
	// The first event sets the tempo (filled in)
	0, 255, 81, 3, 0, 0, 0
};

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// MIDIStreamer Constructor
//
//==========================================================================

MIDIStreamer::MIDIStreamer()
: Division(0), InitialTempo(500000)
{
}

//==========================================================================
//
// MIDIStreamer Destructor
//
//==========================================================================

MIDIStreamer::~MIDIStreamer()
{
}

//==========================================================================
//
// MIDIStreamer :: CheckCaps
//
// Called immediately after the device is opened in case a subclass should
// want to alter its behavior depending on which device it got.
//
//==========================================================================

void MIDIStreamer::CheckCaps(int tech)
{
}

//==========================================================================
//
// MIDIStreamer :: VolumeControllerChange
//
// Some devices don't support master volume
// (e.g. the Audigy's software MIDI synth--but not its two hardware ones),
// so assume none of them do and scale channel volumes manually.
//
//==========================================================================

int MIDIStreamer::VolumeControllerChange(int channel, int volume)
{
	// If loops are limited, we can assume we're exporting this MIDI file,
	// so we should not adjust the volume level.
	return LoopLimit != 0 ? volume : ((volume + 1) * Volume) >> 16;
}

//==========================================================================
//
// MIDIStreamer :: CreateSMF
//
// Simulates playback to create a Standard MIDI File.
//
//==========================================================================

void MIDIStreamer::CreateSMF(TArray<uint8_t> &file, int looplimit)
{
	uint32_t delay = 0;
	uint8_t running_status = 255;

	// Always create songs aimed at GM devices.
	CheckCaps(MOD_MIDIPORT);
	LoopLimit = looplimit <= 0 ? EXPORT_LOOP_LIMIT : looplimit;
	DoRestart();
	Tempo = InitialTempo;

	file.Reserve(sizeof(StaticMIDIhead));
	memcpy(&file[0], StaticMIDIhead, sizeof(StaticMIDIhead));
	file[12] = Division >> 8;
	file[13] = Division & 0xFF;
	file[26] = InitialTempo >> 16;
	file[27] = InitialTempo >> 8;
	file[28] = InitialTempo;

	while (!CheckDone())
	{
		uint32_t *event_end = MakeEvents(Events[0], &Events[0][MAX_EVENTS*3], 1000000*600);
		for (uint32_t *event = Events[0]; event < event_end; )
		{
			delay += event[0];
			if (MEVT_EVENTTYPE(event[2]) == MEVT_TEMPO)
			{
				WriteVarLen(file, delay);
				delay = 0;
				uint32_t tempo = MEVT_EVENTPARM(event[2]);
				file.Push(MIDI_META);
				file.Push(MIDI_META_TEMPO);
				file.Push(3);
				file.Push(uint8_t(tempo >> 16));
				file.Push(uint8_t(tempo >> 8));
				file.Push(uint8_t(tempo));
				running_status = 255;
			}
			else if (MEVT_EVENTTYPE(event[2]) == MEVT_LONGMSG)
			{
				WriteVarLen(file, delay);
				delay = 0;
				uint32_t len = MEVT_EVENTPARM(event[2]);
				uint8_t *uint8_ts = (uint8_t *)&event[3];
				if (uint8_ts[0] == MIDI_SYSEX)
				{
					len--;
					file.Push(MIDI_SYSEX);
					WriteVarLen(file, len);
					memcpy(&file[file.Reserve(len - 1)], uint8_ts, len);
					running_status = 255;
				}
			}
			else if (MEVT_EVENTTYPE(event[2]) == 0)
			{
				WriteVarLen(file, delay);
				delay = 0;
				uint8_t status = uint8_t(event[2]);
				if (status != running_status)
				{
					running_status = status;
					file.Push(status);
				}
				file.Push(uint8_t((event[2] >> 8) & 0x7F));
				if (MIDI_EventLengths[(status >> 4) & 7] == 2)
				{
					file.Push(uint8_t((event[2] >> 16) & 0x7F));
				}
			}
			// Advance to next event
			if (event[2] < 0x80000000)
			{ // short message
				event += 3;
			}
			else
			{ // long message
				event += 3 + ((MEVT_EVENTPARM(event[2]) + 3) >> 2);
			}
		}
	}

	// End track
	WriteVarLen(file, delay);
	file.Push(MIDI_META);
	file.Push(MIDI_META_EOT);
	file.Push(0);

	// Fill in track length
	uint32_t len = file.Size() - 22;
	file[18] = uint8_t(len >> 24);
	file[19] = uint8_t(len >> 16);
	file[20] = uint8_t(len >> 8);
	file[21] = uint8_t(len & 255);

	LoopLimit = 0;
}

//==========================================================================
//
// WriteVarLen
//
//==========================================================================

static void WriteVarLen (TArray<uint8_t> &file, uint32_t value)
{
   uint32_t buffer = value & 0x7F;

   while ( (value >>= 7) )
   {
     buffer <<= 8;
     buffer |= (value & 0x7F) | 0x80;
   }

   for (;;)
   {
	   file.Push(uint8_t(buffer));
	   if (buffer & 0x80)
	   {
		   buffer >>= 8;
	   }
	   else
	   {
		   break;
	   }
   }
}

//==========================================================================
//
// MIDIStreamer :: SetTempo
//
// Sets the tempo from a track's initial meta events. Later tempo changes
// create MEVT_TEMPO events instead.
//
//==========================================================================

void MIDIStreamer::SetTempo(int new_tempo)
{
	InitialTempo = Tempo = new_tempo;
}


//==========================================================================
//
// MIDIStreamer :: ClampLoopCount
//
// We use the XMIDI interpretation of loop count here, where 1 means it
// plays that section once (in other uint16_ts, no loop) rather than the EMIDI
// interpretation where 1 means to loop it once.
//
// If LoopLimit is 1, we limit all loops, since this pass over the song is
// used to determine instruments for precaching.
//
// If LoopLimit is higher, we only limit infinite loops, since this song is
// being exported.
//
//==========================================================================

int MIDIStreamer::ClampLoopCount(int loopcount)
{
	if (LoopLimit == 0)
	{
		return loopcount;
	}
	if (LoopLimit == 1)
	{
		return 1;
	}
	if (loopcount == 0)
	{
		return LoopLimit;
	}
	return loopcount;
}

//==========================================================================
//
// MIDIStreamer :: SetSubsong
//
// Selects which subsong to play in an already-playing file. This is public.
//
//==========================================================================

bool MIDIStreamer::SetSubsong(int subsong)
{
	return SetMIDISubsong(subsong);
}

//==========================================================================
//
// MIDIStreamer :: SetMIDISubsong
//
// Selects which subsong to play. This is private.
//
//==========================================================================

bool MIDIStreamer::SetMIDISubsong(int subsong)
{
	return subsong == 0;
}
