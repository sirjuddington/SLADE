/*
** i_music.cpp
** Plays music
**
**---------------------------------------------------------------------------
** Copyright 1998-2010 Randy Heit
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

#include "Main.h"

#include "i_musicinterns.h"
#include "i_music.h"
#include "m_swap.h"
#include "templates.h"

enum EMIDIType
{
	MIDI_NOTMIDI,
	MIDI_MIDI,
	MIDI_HMI,
	MIDI_XMI,
	MIDI_MUS
};

extern int MUSHeaderSearch(const BYTE *head, int len);

//==========================================================================
//
// create a streamer based on MIDI file type
//
//==========================================================================

static MIDIStreamer *CreateMIDIStreamer(FILE *file, const BYTE *musiccache, int len, EMidiDevice devtype, EMIDIType miditype)
{
	switch (miditype)
	{
	case MIDI_MUS:
		return new MUSSong(file, musiccache, len, devtype);

	case MIDI_MIDI:
		return new MIDISong(file, musiccache, len, devtype);

	case MIDI_HMI:
		return new HMISong(file, musiccache, len, devtype);

	case MIDI_XMI:
		return new XMISong(file, musiccache, len, devtype);

	default:
		return NULL;
	}
}

//==========================================================================
//
// identify MIDI file type
//
//==========================================================================

static EMIDIType IdentifyMIDIType(DWORD *id, int size)
{
	// Check for MUS format
	// Tolerate sloppy wads by searching up to 32 bytes for the header
	if (MUSHeaderSearch((BYTE*)id, size) >= 0)
	{
		return MIDI_MUS;
	}
	// Check for HMI format
	else 
	if (id[0] == MAKE_ID('H','M','I','-') &&
		id[1] == MAKE_ID('M','I','D','I') &&
		id[2] == MAKE_ID('S','O','N','G'))
	{
		return MIDI_HMI;
	}
	// Check for HMP format
	else
	if (id[0] == MAKE_ID('H','M','I','M') &&
		id[1] == MAKE_ID('I','D','I','P'))
	{
		return MIDI_HMI;
	}
	// Check for XMI format
	else
	if ((id[0] == MAKE_ID('F','O','R','M') &&
		 id[2] == MAKE_ID('X','D','I','R')) ||
		((id[0] == MAKE_ID('C','A','T',' ') || id[0] == MAKE_ID('F','O','R','M')) &&
		 id[2] == MAKE_ID('X','M','I','D')))
	{
		return MIDI_XMI;
	}
	// Check for MIDI format
	else if (id[0] == MAKE_ID('M','T','h','d'))
	{
		return MIDI_MIDI;
	}
	else
	{
		return MIDI_NOTMIDI;
	}
}


//==========================================================================
//
// Our little ZDoom-to-SLADE conversion function
//
//==========================================================================

bool zmus2mid(MemChunk& musinput, MemChunk& midioutput)
{
	EMIDIType type = IdentifyMIDIType((DWORD*)musinput.getData(), musinput.getSize());
	if (type != MIDI_NOTMIDI)
	{
		MIDIStreamer* streamer = CreateMIDIStreamer(NULL, musinput.getData(), musinput.getSize(), MDEV_FLUIDSYNTH, type);
		if (streamer)
		{
			TArray<BYTE> bytes;
			streamer->CreateSMF(bytes);
			return (bytes.Size() && midioutput.write(&bytes[0], bytes.Size()));
		}
	}
	return false;
}
