/*
** music_midi_midiout.cpp
** Code to let ZDoom play SMF MIDI music through the MIDI streaming API.
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
**
** This file also supports the Apogee Sound System's EMIDI files. That
** basically means you can play the Duke3D songs without any editing and
** have them sound right.
*/

// HEADER FILES ------------------------------------------------------------

#include "i_musicinterns.h"
#include "templates.h"
#include "m_swap.h"

// MACROS ------------------------------------------------------------------

// Used by SendCommand to check for unexpected end-of-track conditions.
#define CHECK_FINISHED \
	if (track->TrackP >= track->MaxTrackP) \
	{ \
		track->Finished = true; \
		return events; \
	}

// TYPES -------------------------------------------------------------------

struct MIDISong::TrackInfo
{
	const uint8_t *TrackBegin;
	size_t TrackP;
	size_t MaxTrackP;
	uint32_t Delay;
	uint32_t PlayedTime;
	bool Finished;
	uint8_t RunningStatus;
	bool Designated;
	bool EProgramChange;
	bool EVolume;
	uint16_t Designation;

	size_t LoopBegin;
	uint32_t LoopDelay;
	int LoopCount;
	bool LoopFinished;
    
	uint32_t ReadVarLen ();
};

// PUBLIC DATA DEFINITIONS -------------------------------------------------

char MIDI_EventLengths[7] = { 2, 2, 2, 2, 1, 1, 2 };
char MIDI_CommonLengths[15] = { 0, 1, 2, 1, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0 };

//==========================================================================
//
// MIDISong Constructor
//
// Buffers the file and does some validation of the SMF header.
//
//==========================================================================

MIDISong::MIDISong (FILE *file, const uint8_t *musiccache, int len)
: MIDIStreamer(), MusHeader(0), Tracks(0)
{
	int p;
	int i;

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

	// Do some validation of the MIDI file
	if (MusHeader[4] != 0 || MusHeader[5] != 0 || MusHeader[6] != 0 || MusHeader[7] != 6)
		return;

	if (MusHeader[8] != 0 || MusHeader[9] > 2)
		return;

	Format = MusHeader[9];

	if (Format == 0)
	{
		NumTracks = 1;
	}
	else
	{
		NumTracks = MusHeader[10] * 256 + MusHeader[11];
	}

	// The division is the number of pulses per quarter note (PPQN).
	Division = MusHeader[12] * 256 + MusHeader[13];
	if (Division == 0)
	{ // PPQN is zero? Then the song cannot play because it never pulses.
		return;
	}

	Tracks = new TrackInfo[NumTracks];

	// Gather information about each track
	for (i = 0, p = 14; i < NumTracks && p < len + 8; ++i)
	{
		uint32_t chunkLen =
			(MusHeader[p+4]<<24) |
			(MusHeader[p+5]<<16) |
			(MusHeader[p+6]<<8)  |
			(MusHeader[p+7]);

		if (chunkLen + p + 8 > (uint32_t)len)
		{ // Track too long, so truncate it
			chunkLen = len - p - 8;
		}

		if (MusHeader[p+0] == 'M' &&
			MusHeader[p+1] == 'T' &&
			MusHeader[p+2] == 'r' &&
			MusHeader[p+3] == 'k')
		{
			Tracks[i].TrackBegin = MusHeader + p + 8;
			Tracks[i].TrackP = 0;
			Tracks[i].MaxTrackP = chunkLen;
		}

		p += chunkLen + 8;
	}

	// In case there were fewer actual chunks in the file than the
	// header specified, update NumTracks with the current value of i
	NumTracks = i;

	if (NumTracks == 0)
	{ // No tracks, so nothing to play
		return;
	}
}

//==========================================================================
//
// MIDISong Destructor
//
//==========================================================================

MIDISong::~MIDISong ()
{
	if (Tracks != NULL)
	{
		delete[] Tracks;
	}
	if (MusHeader != NULL)
	{
		delete[] MusHeader;
	}
}

//==========================================================================
//
// MIDISong :: CheckCaps
//
// Find out if this is an FM synth or not for EMIDI's benefit.
// (Do any released EMIDIs use track designations?)
//
//==========================================================================

void MIDISong::CheckCaps(int tech)
{
	DesignationMask = 0xFF0F;
	if (tech == MOD_FMSYNTH)
	{
		DesignationMask = 0x00F0;
	}
	else if (tech == MOD_MIDIPORT)
	{
		DesignationMask = 0x0001;
	}
}

//==========================================================================
//
// MIDISong :: DoRestart
//
// Rewinds every track.
//
//==========================================================================

void MIDISong :: DoRestart()
{
	int i;

	// Set initial state.
	for (i = 0; i < NumTracks; ++i)
	{
		Tracks[i].TrackP = 0;
		Tracks[i].Finished = false;
		Tracks[i].RunningStatus = 0;
		Tracks[i].Designated = false;
		Tracks[i].Designation = 0;
		Tracks[i].LoopCount = -1;
		Tracks[i].EProgramChange = false;
		Tracks[i].EVolume = false;
		Tracks[i].PlayedTime = 0;
	}
	ProcessInitialMetaEvents ();
	for (i = 0; i < NumTracks; ++i)
	{
		Tracks[i].Delay = Tracks[i].ReadVarLen();
	}
	TrackDue = Tracks;
	TrackDue = FindNextDue();
}

//==========================================================================
//
// MIDISong :: CheckDone
//
//==========================================================================

bool MIDISong::CheckDone()
{
	return TrackDue == NULL;
}

//==========================================================================
//
// MIDISong :: MakeEvents
//
// Copies MIDI events from the SMF and puts them into a MIDI stream
// buffer. Returns the new position in the buffer.
//
//==========================================================================

uint32_t *MIDISong::MakeEvents(uint32_t *events, uint32_t *max_event_p, uint32_t max_time)
{
	uint32_t *start_events;
	uint32_t tot_time = 0;
	uint32_t time = 0;
	uint32_t delay;

	start_events = events;
	while (TrackDue && events < max_event_p && tot_time <= max_time)
	{
		// It's possible that this tick may be nothing but meta-events and
		// not generate any real events. Repeat this until we actually
		// get some output so we don't send an empty buffer to the MIDI
		// device.
		do
		{
			delay = TrackDue->Delay;
			time += delay;
			// Advance time for all tracks by the amount needed for the one up next.
			tot_time += delay * Tempo / Division;
			AdvanceTracks(delay);
			// Play all events for this tick.
			do
			{
				uint32_t *new_events = SendCommand(events, TrackDue, time);
				TrackDue = FindNextDue();
				if (new_events != events)
				{
					time = 0;
				}
				events = new_events;
			}
			while (TrackDue && TrackDue->Delay == 0 && events < max_event_p);
		}
		while (start_events == events && TrackDue);
		time = 0;
	}
	return events;
}

//==========================================================================
//
// MIDISong :: AdvanceTracks
//
// Advances time for all tracks by the specified amount.
//
//==========================================================================

void MIDISong::AdvanceTracks(uint32_t time)
{
	for (int i = 0; i < NumTracks; ++i)
	{
		if (!Tracks[i].Finished)
		{
			Tracks[i].Delay -= time;
			Tracks[i].PlayedTime += time;
		}
	}
}

//==========================================================================
//
// MIDISong :: SendCommand
//
// Places a single MIDIEVENT in the event buffer.
//
//==========================================================================

uint32_t *MIDISong::SendCommand (uint32_t *events, TrackInfo *track, uint32_t delay)
{
	uint32_t len;
	uint8_t event, data1 = 0, data2 = 0;
	int i;

	CHECK_FINISHED
	event = track->TrackBegin[track->TrackP++];
	CHECK_FINISHED

	if (event != MIDI_SYSEX && event != MIDI_META && event != MIDI_SYSEXEND)
	{
		// Normal short message
		if ((event & 0xF0) == 0xF0)
		{
			if (MIDI_CommonLengths[event & 15] > 0)
			{
				data1 = track->TrackBegin[track->TrackP++];
				if (MIDI_CommonLengths[event & 15] > 1)
				{
					data2 = track->TrackBegin[track->TrackP++];
				}
			}
		}
		else if ((event & 0x80) == 0)
		{
			data1 = event;
			event = track->RunningStatus;
		}
		else
		{
			track->RunningStatus = event;
			data1 = track->TrackBegin[track->TrackP++];
		}

		CHECK_FINISHED

		if (MIDI_EventLengths[(event&0x70)>>4] == 2)
		{
			data2 = track->TrackBegin[track->TrackP++];
		}

		switch (event & 0x70)
		{
		case MIDI_PRGMCHANGE & 0x70:
			if (track->EProgramChange)
			{
				event = MIDI_META;
			}
			break;

		case MIDI_CTRLCHANGE & 0x70:
			switch (data1)
			{
			case 7:		// Channel volume
				if (track->EVolume)
				{ // Tracks that use EMIDI volume ignore normal volume changes.
					event = MIDI_META;
				}
				else
				{
					data2 = VolumeControllerChange(event & 15, data2);
				}
				break;

			case 7+32:	// Channel volume (LSB)
				if (track->EVolume)
				{
					event = MIDI_META;
				}
				// It should be safe to pass this straight through to the
				// MIDI device, since it's a very fine amount.
				break;

			case 110:	// EMIDI Track Designation - InitBeat only
				// Instruments 4, 5, 6, and 7 are all FM synth.
				// The rest are all wavetable.
				if (track->PlayedTime < (uint32_t)Division)
				{
					if (data2 == 127)
					{
						track->Designation = ~0;
						track->Designated = true;
					}
					else if (data2 <= 9)
					{
						track->Designation |= 1 << data2;
						track->Designated = true;
					}
					event = MIDI_META;
				}
				break;

			case 111:	// EMIDI Track Exclusion - InitBeat only
				if (track->PlayedTime < (uint32_t)Division)
				{
					if (track->Designated && data2 <= 9)
					{
						track->Designation &= ~(1 << data2);
					}
					event = MIDI_META;
				}
				break;

			case 112:	// EMIDI Program Change
				// Ignored unless it also appears in the InitBeat
				if (track->PlayedTime < (uint32_t)Division || track->EProgramChange)
				{
					track->EProgramChange = true;
					event = 0xC0 | (event & 0x0F);
					data1 = data2;
					data2 = 0;
				}
				break;

			case 113:	// EMIDI Volume
				// Ignored unless it also appears in the InitBeat
				if (track->PlayedTime < (uint32_t)Division || track->EVolume)
				{
					track->EVolume = true;
					data1 = 7;
					data2 = VolumeControllerChange(event & 15, data2);
				}
				break;

			case 116:	// EMIDI Loop Begin
				{
					// We convert the loop count to XMIDI conventions before clamping.
					// Then we convert it back to EMIDI conventions after clamping.
					// (XMIDI can create "loops" that don't loop. EMIDI cannot.)
					int loopcount = ClampLoopCount(data2 == 0 ? 0 : data2 + 1);
					if (loopcount != 1)
					{
						track->LoopBegin = track->TrackP;
						track->LoopDelay = 0;
						track->LoopCount = loopcount == 0 ? 0 : loopcount - 1;
						track->LoopFinished = track->Finished;
					}
				}
				event = MIDI_META;
				break;

			case 117:	// EMIDI Loop End
				if (track->LoopCount >= 0 && data2 == 127)
				{
					if (track->LoopCount == 0)
					{
						track->Finished = true;
					}
					else
					{
						if (track->LoopCount > 0 && --track->LoopCount == 0)
						{
							track->LoopCount = -1;
						}
						track->TrackP = track->LoopBegin;
						track->Delay = track->LoopDelay;
						track->Finished = track->LoopFinished;
					}
				}
				event = MIDI_META;
				break;

			case 118:	// EMIDI Global Loop Begin
				{
					int loopcount = ClampLoopCount(data2 == 0 ? 0 : data2 + 1);
					if (loopcount != 1)
					{
						for (i = 0; i < NumTracks; ++i)
						{
							Tracks[i].LoopBegin = Tracks[i].TrackP;
							Tracks[i].LoopDelay = Tracks[i].Delay;
							Tracks[i].LoopCount = loopcount == 0 ? 0 : loopcount - 1;
							Tracks[i].LoopFinished = Tracks[i].Finished;
						}
					}
				}
				event = MIDI_META;
				break;

			case 119:	// EMIDI Global Loop End
				if (data2 == 127)
				{
					for (i = 0; i < NumTracks; ++i)
					{
						if (Tracks[i].LoopCount >= 0)
						{
							if (Tracks[i].LoopCount == 0)
							{
								Tracks[i].Finished = true;
							}
							else
							{
								if (Tracks[i].LoopCount > 0 && --Tracks[i].LoopCount == 0)
								{
									Tracks[i].LoopCount = -1;
								}
								Tracks[i].TrackP = Tracks[i].LoopBegin;
								Tracks[i].Delay = Tracks[i].LoopDelay;
								Tracks[i].Finished = Tracks[i].LoopFinished;
							}
						}
					}
				}
				event = MIDI_META;
				break;
			}
		}
		events[0] = delay;
		events[1] = 0;
		if (event != MIDI_META && (!track->Designated || (track->Designation & DesignationMask)))
		{
			events[2] = event | (data1<<8) | (data2<<16);
		}
		else
		{
			events[2] = MEVT_NOP << 24;
		}
		events += 3;
	}
	else
	{
		// Skip SysEx events just because I don't want to bother with them.
		// The old MIDI player ignored them too, so this won't break
		// anything that played before.
		if (event == MIDI_SYSEX || event == MIDI_SYSEXEND)
		{
			len = track->ReadVarLen ();
			track->TrackP += len;
		}
		else if (event == MIDI_META)
		{
			// It's a meta-event
			event = track->TrackBegin[track->TrackP++];
			CHECK_FINISHED
			len = track->ReadVarLen ();
			CHECK_FINISHED

			if (track->TrackP + len <= track->MaxTrackP)
			{
				switch (event)
				{
				case MIDI_META_EOT:
					track->Finished = true;
					break;

				case MIDI_META_TEMPO:
					Tempo =
						(track->TrackBegin[track->TrackP+0]<<16) |
						(track->TrackBegin[track->TrackP+1]<<8)  |
						(track->TrackBegin[track->TrackP+2]);
					events[0] = delay;
					events[1] = 0;
					events[2] = (MEVT_TEMPO << 24) | Tempo;
					events += 3;
					break;
				}
				track->TrackP += len;
				if (track->TrackP == track->MaxTrackP)
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
		track->Delay = track->ReadVarLen();
	}
	return events;
}

//==========================================================================
//
// MIDISong :: ProcessInitialMetaEvents
//
// Handle all the meta events at the start of each track.
//
//==========================================================================

void MIDISong::ProcessInitialMetaEvents ()
{
	TrackInfo *track;
	int i;
	uint8_t event;
	uint32_t len;

	for (i = 0; i < NumTracks; ++i)
	{
		track = &Tracks[i];
		while (!track->Finished &&
				track->TrackP < track->MaxTrackP - 4 &&
				track->TrackBegin[track->TrackP] == 0 &&
				track->TrackBegin[track->TrackP+1] == 0xFF)
		{
			event = track->TrackBegin[track->TrackP+2];
			track->TrackP += 3;
			len = track->ReadVarLen ();
			if (track->TrackP + len <= track->MaxTrackP)
			{
				switch (event)
				{
				case MIDI_META_EOT:
					track->Finished = true;
					break;

				case MIDI_META_TEMPO:
					SetTempo(
						(track->TrackBegin[track->TrackP+0]<<16) |
						(track->TrackBegin[track->TrackP+1]<<8)  |
						(track->TrackBegin[track->TrackP+2])
					);
					break;
				}
			}
			track->TrackP += len;
		}
		if (track->TrackP >= track->MaxTrackP - 4)
		{
			track->Finished = true;
		}
	}
}

//==========================================================================
//
// MIDISong :: TrackInfo :: ReadVarLen
//
// Reads a variable-length SMF number.
//
//==========================================================================

uint32_t MIDISong::TrackInfo::ReadVarLen ()
{
	uint32_t time = 0, t = 0x80;

	while ((t & 0x80) && TrackP < MaxTrackP)
	{
		t = TrackBegin[TrackP++];
		time = (time << 7) | (t & 127);
	}
	return time;
}

//==========================================================================
//
// MIDISong :: FindNextDue
//
// Scans every track for the next event to play. Returns NULL if all events
// have been consumed.
//
//==========================================================================

MIDISong::TrackInfo *MIDISong::FindNextDue ()
{
	TrackInfo *track;
	uint32_t best;
	int i;

	// Give precedence to whichever track last had events taken from it.
	if (!TrackDue->Finished && TrackDue->Delay == 0)
	{
		return TrackDue;
	}

	switch (Format)
	{
	case 0:
		return Tracks[0].Finished ? NULL : Tracks;
		
	case 1:
		track = NULL;
		best = 0xFFFFFFFF;
		for (i = 0; i < NumTracks; ++i)
		{
			if (!Tracks[i].Finished)
			{
				if (Tracks[i].Delay < best)
				{
					best = Tracks[i].Delay;
					track = &Tracks[i];
				}
			}
		}
		return track;

	case 2:
		track = TrackDue;
		if (track->Finished)
		{
			track++;
		}
		return track < &Tracks[NumTracks] ? track : NULL;
	}
	return NULL;
}
