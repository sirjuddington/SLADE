/*
** music_xmi_midiout.cpp
** Code to let ZDoom play XMIDI music through the MIDI streaming API.
**
**---------------------------------------------------------------------------
** Copyright 2010 Randy Heit
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
#include "portable_endian.h"
#include "Utility/Memory.h"

using namespace slade;

// MACROS ------------------------------------------------------------------

#define MAX_FOR_DEPTH		4

#define GET_DELAY			(EventDue == EVENT_Real ? CurrSong->Delay : NoteOffs[0].Delay)

// Used by SendCommand to check for unexpected end-of-track conditions.
#define CHECK_FINISHED \
	if (track->EventP >= track->EventLen) \
	{ \
		track->Finished = true; \
		return events; \
	}

// TYPES -------------------------------------------------------------------

struct LoopInfo
{
	size_t LoopBegin;
	int LoopCount;
	bool LoopFinished;
};

struct XMISong::TrackInfo
{
	const uint8_t *EventChunk;
	size_t EventLen;
	size_t EventP;

	const uint8_t *TimbreChunk;
	size_t TimbreLen;

	uint32_t Delay;
	uint32_t PlayedTime;
	bool Finished;

	LoopInfo ForLoops[MAX_FOR_DEPTH];
	int ForDepth;
    
	uint32_t ReadVarLen();
	uint32_t ReadDelay();
};

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern char MIDI_EventLengths[7];
extern char MIDI_CommonLengths[15];

//==========================================================================
//
// XMISong Constructor
//
// Buffers the file and does some validation of the SMF header.
//
//==========================================================================

XMISong::XMISong (FILE *file, const uint8_t *musiccache, int len)
: MIDIStreamer(), MusHeader(0), Songs(0)
{
	MusHeader = new uint8_t[len];
	SongLen = len;
	if (file != NULL)
	{
		if (fread(MusHeader, 1, len, file) != (size_t)len)
			return;
	}
	else
	{
		memcpy(MusHeader, musiccache, len);
	}

	// Find all the songs in this file.
	NumSongs = FindXMIDforms(MusHeader, len, NULL);
	if (NumSongs == 0)
	{
		return;
	}

	// XMIDI files are played with a constant 120 Hz clock rate. While the
	// song may contain tempo events, these are vestigial remnants from the
	// original MIDI file that were not removed by the converter and should
	// be ignored.
	//
	// We can use any combination of Division and Tempo values that work out
	// to be 120 Hz.
	Division = 60;
	InitialTempo = 500000;

	Songs = new TrackInfo[NumSongs];
	memset(Songs, 0, sizeof(*Songs) * NumSongs);
	FindXMIDforms(MusHeader, len, Songs);
	CurrSong = Songs;
}

//==========================================================================
//
// XMISong Destructor
//
//==========================================================================

XMISong::~XMISong ()
{
	if (Songs != NULL)
	{
		delete[] Songs;
	}
	if (MusHeader != NULL)
	{
		delete[] MusHeader;
	}
}

//==========================================================================
//
// XMISong :: FindXMIDforms
//
// Find all FORM XMID chunks in this chunk.
//
//==========================================================================

int XMISong::FindXMIDforms(const uint8_t *chunk, int len, TrackInfo *songs) const
{
	int count = 0;

	for (int p = 0; p <= len - 12; )
	{
		uint32_t chunktype = memory::readL32(chunk, p);
		int chunklen = memory::readB32(chunk, p + 4);

		if (chunktype == MAKE_ID('F','O','R','M'))
		{
			uint32_t chunktype2 = memory::readL32(chunk, p + 8);
			if (chunktype2 == MAKE_ID('X','M','I','D'))
			{
				if (songs != NULL)
				{
					FoundXMID(chunk + p + 12, chunklen - 4, songs + count);
				}
				count++;
			}
		}
		else if (chunktype == MAKE_ID('C','A','T',' '))
		{
			// Recurse to handle CAT chunks.
			count += FindXMIDforms(chunk + p + 12, chunklen - 4, songs + count);
		}
		// IFF chunks are padded to even uint8_t boundaries to avoid
		// unaligned reads on 68k processors.
		p += 8 + chunklen + (chunklen & 1);
		// Avoid crashes from corrupt chunks which indicate a negative size.
		if (chunklen < 0) p = len;
	}
	return count;
}

//==========================================================================
//
// XMISong :: FoundXMID
//
// Records information about this XMID song.
//
//==========================================================================

void XMISong::FoundXMID(const uint8_t *chunk, int len, TrackInfo *song) const
{
	for (int p = 0; p <= len - 8; )
	{
		uint32_t chunktype = *(uint32_t*)(chunk + p);
		int chunklen = memory::readB32(chunk, p + 4);

		if (chunktype == MAKE_ID('T','I','M','B'))
		{
			song->TimbreChunk = chunk + p + 8;
			song->TimbreLen = chunklen;
		}
		else if (chunktype == MAKE_ID('E','V','N','T'))
		{
			song->EventChunk = chunk + p + 8;
			song->EventLen = chunklen;
			// EVNT must be the final chunk in the FORM.
			break;
		}
		p += 8 + chunklen + (chunklen & 1);
	}
}

//==========================================================================
//
// XMISong :: GetMIDISubsongs
//
// Returns how many songs are available.
//
//==========================================================================

int XMISong::GetMIDISubsongs()
{
	return NumSongs;
}

//==========================================================================
//
// XMISong :: SetMIDISubsong
//
// Selects which song in this file to play.
//
//==========================================================================

bool XMISong::SetMIDISubsong(int subsong)
{
	if ((unsigned)subsong >= (unsigned)NumSongs)
	{
		return false;
	}
	CurrSong = &Songs[subsong];
	return true;
}

//==========================================================================
//
// XMISong :: DoRestart
//
// Rewinds the current song.
//
//==========================================================================

void XMISong::DoRestart()
{
	CurrSong->EventP = 0;
	CurrSong->Finished = false;
	CurrSong->PlayedTime = 0;
	CurrSong->ForDepth = 0;
	NoteOffs.Clear();

	ProcessInitialMetaEvents ();

	CurrSong->Delay = CurrSong->ReadDelay();
	EventDue = FindNextDue();
}

//==========================================================================
//
// XMISong :: CheckDone
//
//==========================================================================

bool XMISong::CheckDone()
{
	return EventDue == EVENT_None;
}

//==========================================================================
//
// XMISong :: MakeEvents
//
// Copies MIDI events from the XMI and puts them into a MIDI stream
// buffer. Returns the new position in the buffer.
//
//==========================================================================

uint32_t *XMISong::MakeEvents(uint32_t *events, uint32_t *max_event_p, uint32_t max_time)
{
	uint32_t *start_events;
	uint32_t tot_time = 0;
	uint32_t time = 0;
	uint32_t delay;

	start_events = events;
	while (EventDue != EVENT_None && events < max_event_p && tot_time <= max_time)
	{
		// It's possible that this tick may be nothing but meta-events and
		// not generate any real events. Repeat this until we actually
		// get some output so we don't send an empty buffer to the MIDI
		// device.
		do
		{
			delay = GET_DELAY;
			time += delay;
			// Advance time for all tracks by the amount needed for the one up next.
			tot_time += delay * Tempo / Division;
			AdvanceSong(delay);
			// Play all events for this tick.
			do
			{
				uint32_t *new_events = SendCommand(events, EventDue, time);
				EventDue = FindNextDue();
				if (new_events != events)
				{
					time = 0;
				}
				events = new_events;
			}
			while (EventDue != EVENT_None && GET_DELAY == 0 && events < max_event_p);
		}
		while (start_events == events && EventDue != EVENT_None);
		time = 0;
	}
	return events;
}

//==========================================================================
//
// XMISong :: AdvanceSong
//
// Advances time for the current song by the specified amount.
//
//==========================================================================

void XMISong::AdvanceSong(uint32_t time)
{
	if (time != 0)
	{
		if (!CurrSong->Finished)
		{
			CurrSong->Delay -= time;
			CurrSong->PlayedTime += time;
		}
		NoteOffs.AdvanceTime(time);
	}
}

//==========================================================================
//
// XMISong :: SendCommand
//
// Places a single MIDIEVENT in the event buffer.
//
//==========================================================================

uint32_t *XMISong::SendCommand (uint32_t *events, EventSource due, uint32_t delay)
{
	uint32_t len;
	uint8_t event, data1 = 0, data2 = 0;

	if (due == EVENT_Fake)
	{
		AutoNoteOff off;
		NoteOffs.Pop(off);
		events[0] = delay;
		events[1] = 0;
		events[2] = MIDI_NOTEON | off.Channel | (off.Key << 8);
		return events + 3;
	}

	TrackInfo *track = CurrSong;

	CHECK_FINISHED
	event = track->EventChunk[track->EventP++];
	CHECK_FINISHED

	if (event != MIDI_SYSEX && event != MIDI_META && event != MIDI_SYSEXEND)
	{
		// Normal short message
		if ((event & 0xF0) == 0xF0)
		{
			if (MIDI_CommonLengths[event & 15] > 0)
			{
				data1 = track->EventChunk[track->EventP++];
				if (MIDI_CommonLengths[event & 15] > 1)
				{
					data2 = track->EventChunk[track->EventP++];
				}
			}
		}
		else
		{
			data1 = track->EventChunk[track->EventP++];
		}

		CHECK_FINISHED

		if (MIDI_EventLengths[(event&0x70)>>4] == 2)
		{
			data2 = track->EventChunk[track->EventP++];
		}

		if ((event & 0x70) == (MIDI_CTRLCHANGE & 0x70))
		{
			switch (data1)
			{
			case 7:		// Channel volume
				data2 = VolumeControllerChange(event & 15, data2);
				break;

			case 110:	// XMI channel lock
			case 111:	// XMI channel lock protect
			case 112:	// XMI voice protect
			case 113:	// XMI timbre protect
			case 115:	// XMI indirect controller prefix
			case 118:	// XMI clear beat/bar count
			case 119:	// XMI callback trigger
			case 120:
				event = MIDI_META;		// none of these are relevant to us.
				break;

			case 114:	// XMI patch bank select
				data1 = 0;				// Turn this into a standard MIDI bank select controller.
				break;

			case 116:	// XMI for loop controller
				if (track->ForDepth < MAX_FOR_DEPTH)
				{
					track->ForLoops[track->ForDepth].LoopBegin = track->EventP;
					track->ForLoops[track->ForDepth].LoopCount = ClampLoopCount(data2);
					track->ForLoops[track->ForDepth].LoopFinished = track->Finished;
				}
				track->ForDepth++;
				event = MIDI_META;
				break;

			case 117:	// XMI next loop controller
				if (track->ForDepth > 0)
				{
					int depth = track->ForDepth - 1;
					if (depth < MAX_FOR_DEPTH)
					{
						if (data2 < 64 || (track->ForLoops[depth].LoopCount == 0))
						{ // throw away this loop.
							track->ForLoops[depth].LoopCount = 1;
						}
						// A loop count of 0 loops forever.
						if (track->ForLoops[depth].LoopCount == 0 || --track->ForLoops[depth].LoopCount > 0)
						{
							track->EventP = track->ForLoops[depth].LoopBegin;
							track->Finished = track->ForLoops[depth].LoopFinished;
						}
						else
						{ // done with this loop
							track->ForDepth = depth;
						}
					}
					else
					{ // ignore any loops deeper than the max depth
						track->ForDepth = depth;
					}
				}
				event = MIDI_META;
				break;
			}
		}
		events[0] = delay;
		events[1] = 0;
		if (event != MIDI_META)
		{
			events[2] = event | (data1<<8) | (data2<<16);
		}
		else
		{
			events[2] = MEVT_NOP << 24;
		}
		events += 3;


		if ((event & 0x70) == (MIDI_NOTEON & 0x70))
		{ // XMI note on events include the time until an implied note off event.
			NoteOffs.AddNoteOff(track->ReadVarLen(), event & 0x0F, data1);
		}
	}
	else
	{
		// Skip SysEx events just because I don't want to bother with them.
		// The old MIDI player ignored them too, so this won't break
		// anything that played before.
		if (event == MIDI_SYSEX || event == MIDI_SYSEXEND)
		{
			len = track->ReadVarLen ();
			track->EventP += len;
		}
		else if (event == MIDI_META)
		{
			// It's a meta-event
			event = track->EventChunk[track->EventP++];
			CHECK_FINISHED
			len = track->ReadVarLen ();
			CHECK_FINISHED

			if (track->EventP + len <= track->EventLen)
			{
				if (event == MIDI_META_EOT)
				{
					track->Finished = true;
				}
				track->EventP += len;
				if (track->EventP == track->EventLen)
				{
					track->Finished = true;
				}
			}
			else
			{
				track->Finished = true;
			}
		}
	}
	if (!track->Finished)
	{
		track->Delay = track->ReadDelay();
	}
	return events;
}

//==========================================================================
//
// XMISong :: ProcessInitialMetaEvents
//
// Handle all the meta events at the start of the current song.
//
//==========================================================================

void XMISong::ProcessInitialMetaEvents ()
{
	TrackInfo *track = CurrSong;
	uint8_t event;
	uint32_t len;

	while (!track->Finished &&
			track->EventP < track->EventLen - 3 &&
			track->EventChunk[track->EventP] == MIDI_META)
	{
		event = track->EventChunk[track->EventP+1];
		track->EventP += 2;
		len = track->ReadVarLen();
		if (track->EventP + len <= track->EventLen && event == MIDI_META_EOT)
		{
			track->Finished = true;
		}
		track->EventP += len;
	}
	if (track->EventP >= track->EventLen - 1)
	{
		track->Finished = true;
	}
}

//==========================================================================
//
// XMISong :: TrackInfo :: ReadVarLen
//
// Reads a variable length SMF number.
//
//==========================================================================

uint32_t XMISong::TrackInfo::ReadVarLen()
{
	uint32_t time = 0, t = 0x80;

	while ((t & 0x80) && EventP < EventLen)
	{
		t = EventChunk[EventP++];
		time = (time << 7) | (t & 127);
	}
	return time;
}

//==========================================================================
//
// XMISong :: TrackInfo :: ReadDelay
//
// XMI does not use variable length numbers for delays. Instead, it uses
// runs of uint8_ts with the high bit clear.
//
//==========================================================================

uint32_t XMISong::TrackInfo::ReadDelay()
{
	uint32_t time = 0, t;

	while (EventP < EventLen && !((t = EventChunk[EventP]) & 0x80))
	{
		time += t;
		EventP++;
	}
	return time;
}

//==========================================================================
//
// XMISong :: FindNextDue
//
// Decides whether the next event should come from the actual stong or
// from the auto note offs.
//
//==========================================================================

XMISong::EventSource XMISong::FindNextDue()
{
	// Are there still events available?
	if (CurrSong->Finished && NoteOffs.Size() == 0)
	{
		return EVENT_None;
	}

	// Which is due sooner? The current song or the note-offs?
	uint32_t real_delay = CurrSong->Finished ? 0xFFFFFFFF : CurrSong->Delay;
	uint32_t fake_delay = NoteOffs.Size() == 0 ? 0xFFFFFFFF : NoteOffs[0].Delay;

	return (fake_delay <= real_delay) ? EVENT_Fake : EVENT_Real;
}
