
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MIDIPlayer.cpp
 * Description: MIDIPlayer class, a singleton class that handles
 *              playback of MIDI files. Can only play one MIDI at a
 *              time
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
#include "MIDIPlayer.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
MIDIPlayer*	MIDIPlayer::instance = NULL;
CVAR(String, fs_soundfont_path, "", CVAR_SAVE);
CVAR(String, fs_driver, "", CVAR_SAVE);

/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, snd_volume)
EXTERN_CVAR(String, snd_timidity_path)
EXTERN_CVAR(String, snd_timidity_options)
#ifndef NO_FLUIDSYNTH
EXTERN_CVAR(Bool, snd_midi_usetimidity)
#define usetimidity snd_midi_usetimidity
#else
#define usetimidity true
#endif

/*******************************************************************
 * MIDIPLAYER FLUIDSYNTH IMPLEMENTATION
 *******************************************************************/

/* MIDIPlayer::MIDIPlayer
 * MIDIPlayer class constructor
 *******************************************************************/
MIDIPlayer::MIDIPlayer()
{
	// Init variables
	fs_initialised = false;
	fs_soundfont_ids.clear();
	program = NULL;
	file = "";

#ifndef NO_FLUIDSYNTH
	// Set fluidsynth driver to alsa in linux (no idea why it defaults to jack)
#if !defined __WXMSW__ && !defined __WXOSX__
	if (fs_driver == "")
		fs_driver = "alsa";
#endif // !defined __WXMSW__ && !defined __WXOSX__

	// Init soundfont path
	if (fs_soundfont_path == "")
	{
#ifdef __WXGTK__
		fs_soundfont_path = "/usr/share/sounds/sf2/FluidR3_GM.sf2:/usr/share/sounds/sf2/FluidR3_GS.sf2";
#else // __WXGTK__
		wxLogMessage("Warning: No fluidsynth soundfont set, MIDI playback will not work");
#endif // __WXGTK__
	}

	// Setup fluidsynth
	initFluidsynth();
	reloadSoundfont();

	if (!fs_player || !fs_adriver)
		wxLogMessage("Warning: Failed to initialise FluidSynth, MIDI playback disabled");
#endif // NO_FLUIDSYNTH
}

/* MIDIPlayer::~MIDIPlayer
 * MIDIPlayer class destructor
 *******************************************************************/
MIDIPlayer::~MIDIPlayer()
{
	stop();

	if (program)
		delete program;
#ifndef NO_FLUIDSYNTH
	delete_fluid_audio_driver(fs_adriver);
	delete_fluid_player(fs_player);
	delete_fluid_synth(fs_synth);
	delete_fluid_settings(fs_settings);
#endif
}

/* MIDIPlayer::isReady
 * Returns true if the MIDIPlayer is ready to play some MIDI
 *******************************************************************/
bool MIDIPlayer::isReady()
{
	if (usetimidity)
		return !snd_timidity_path.value.IsEmpty();
#ifndef NO_FLUIDSYNTH
	return fs_initialised && fs_soundfont_ids.size() > 0;
#endif
	return false;
}

void MIDIPlayer::resetPlayer()
{
	stop();

	if (instance)
		delete instance;

	instance = new MIDIPlayer();
}


/* MIDIPlayer::initFluidsynth
 * Initialises fluidsynth
 *******************************************************************/
bool MIDIPlayer::initFluidsynth()
{
	// Don't re-init
	if (fs_initialised)
		return true;

#ifndef NO_FLUIDSYNTH
	// Init fluidsynth settings
	fs_settings = new_fluid_settings();
	if (fs_driver != "")
		fluid_settings_setstr(fs_settings, "audio.driver", wxString(fs_driver).ToAscii());

	// Create fluidsynth objects
	fs_synth = new_fluid_synth(fs_settings);
	fs_player = new_fluid_player(fs_synth);
	fs_adriver = new_fluid_audio_driver(fs_settings, fs_synth);

	// Check init succeeded
	if (fs_synth)
	{
		if (fs_adriver)
		{
			setVolume(snd_volume);
			fs_initialised = true;
			return true;
		}

		// Driver creation unsuccessful
		delete_fluid_synth(fs_synth);
		return false;
	}
#endif

	// Init unsuccessful
	return false;
}

/* MIDIPlayer::reloadSoundfont
 * Reloads the current soundfont
 *******************************************************************/
bool MIDIPlayer::reloadSoundfont()
{
#ifndef NO_FLUIDSYNTH
	// Can't do anything if fluidsynth isn't initialised for whatever reason
	if (!fs_initialised)
		return false;

#ifdef WIN32
	char separator = ';';
#else
	char separator = ':';
#endif

	// Unload any current soundfont
	for (int a = fs_soundfont_ids.size() - 1; a >= 0; --a)
	{
		if (fs_soundfont_ids[a] != FLUID_FAILED)
			fluid_synth_sfunload(fs_synth, fs_soundfont_ids[a], 1);
		fs_soundfont_ids.pop_back();
	}

	// Load soundfonts
	wxArrayString paths = wxSplit(fs_soundfont_path, separator);
	bool retval = false;
	for (int a = paths.size() - 1; a >= 0; --a)
	{
		string path = paths[a];
		if (path.size())
		{
			int fs_id = fluid_synth_sfload(fs_synth, CHR(path), 1);
			fs_soundfont_ids.push_back(fs_id);
			if (fs_id != FLUID_FAILED)
				retval = true;
		}
	}

	return retval;
#else
	return true;
#endif
}

/* MIDIPlayer::openFile
 * Opens the MIDI file at [filename] for playback. Returns true if
 * successful, false otherwise
 *******************************************************************/
bool MIDIPlayer::openFile(string filename)
{
	file = filename;
#ifndef NO_FLUIDSYNTH
	if (!fs_initialised)
		return false;

	// Delete+Recreate player
	delete_fluid_player(fs_player);
	fs_player = NULL;
	fs_player = new_fluid_player(fs_synth);

	// Open midi
	if (fs_player)
	{
		fluid_player_add(fs_player, CHR(filename));
		return true;
	}
	else
		return usetimidity;
#endif
	return true;
}

/* MIDIPlayer::openData
 * Opens the MIDI data contained in [mc] for playback. Returns true if
 * successful, false otherwise
 *******************************************************************/
bool MIDIPlayer::openData(MemChunk &mc)
{
	// Open midi
	mc.seek(0, SEEK_SET);
	data.importMem(mc.getData(), mc.getSize());

	if (usetimidity)
	{
		wxFileName path(appPath("slade-timidity.mid", DIR_TEMP));
		file = path.GetFullPath();
		mc.exportFile(file);
		return true;
	}
#ifndef NO_FLUIDSYNTH
	else
	{
		if (!fs_initialised)
			return false;

		// Delete+Recreate player
		delete_fluid_player(fs_player);
		fs_player = NULL;
		fs_player = new_fluid_player(fs_synth);

		if (fs_player)
		{
			//fluid_player_set_loop(fs_player, -1);
			fluid_player_add_mem(fs_player, mc.getData(), mc.getSize());
			return true;
		}
		else
			return false;
	}
#endif // NO_FLUIDSYNTH
	return false;
}

/* MIDIPlayer::play
 * Begins playback of the currently loaded MIDI stream. Returns true
 * if successful, false otherwise
 *******************************************************************/
bool MIDIPlayer::play()
{
	stop();
	timer.restart();
	if (usetimidity)
	{
		string commandline = snd_timidity_path + " " + file + " " + snd_timidity_options;
		program = wxProcess::Open(commandline);

		int pid = program->GetPid();
		return program->Exists(pid);
	}
#ifndef NO_FLUIDSYNTH
	else
	{
		if (!fs_initialised)
			return false;

		return (fluid_player_play(fs_player) == FLUID_OK);
	}
#endif // NO_FLUIDSYNTH
}

/* MIDIPlayer::pause
 * Pauses playback of the currently loaded MIDI stream
 *******************************************************************/
bool MIDIPlayer::pause()
{
	if (!isReady())
		return false;

	return stop();
}

/* MIDIPlayer::stop
 * Stops playback of the currently loaded MIDI stream
 *******************************************************************/
bool MIDIPlayer::stop()
{
	bool stopped = false;
	if (program)
	{
		int pid = program->GetPid();
		if (isPlaying())
#ifdef WIN32
			program->Kill(pid, wxSIGKILL, wxKILL_CHILDREN);
#else
			program->Kill(pid);
#endif
		stopped = !(program->Exists(pid));
	}
#ifndef NO_FLUIDSYNTH
	if (fs_initialised)
	{
		fluid_player_stop(fs_player);
		fluid_synth_system_reset(fs_synth);
		stopped = true;
	}
#endif // NO_FLUIDSYNTH
	return stopped;
}

/* MIDIPlayer::isPlaying
 * Returns true if the MIDI stream is currently playing, false if not
 *******************************************************************/
bool MIDIPlayer::isPlaying()
{
	if (usetimidity)
	{
		if (!program)
			return false;

		int pid = program->GetPid();
		// also ignore zero pid
		if (!pid || !program->Exists(pid))
			return false;

		return true;
	}
#ifndef NO_FLUIDSYNTH
	else
	{
		if (!fs_initialised)
			return false;

		return (fluid_player_get_status(fs_player) == FLUID_PLAYER_PLAYING);
	}
#endif // NO_FLUISYNTH
	return false;
}

/* MIDIPlayer::getPosition
 * Returns the current position of the playing MIDI stream
 *******************************************************************/
int MIDIPlayer::getPosition()
{
	// We cannot query this information from fluidsynth or timidity,
	// se we cheat by querying our own timer
	return timer.getElapsedTime().asMilliseconds();
}

/* MIDIPlayer::setPosition
 * Seeks to [pos] in the currently loaded MIDI stream
 *******************************************************************/
bool MIDIPlayer::setPosition(int pos)
{
	// Cannot currently seek in fluidsynth or timidity
	return false;
}

/* MIDIPlayer::getLength
 * Returns the length (or maximum position) of the currently loaded
 * MIDI stream, in milliseconds.
 * MIDI time division is the number of pulses per quarter note, aka
 * PPQN, or clock tick per beat; but it doesn't tell us how long a
 * beat or a tick lasts. To know that we also need to know the tempo
 * which is a meta event and therefore optional. The tempo tells us
 * how many microseconds there are in a quarter note, so from that
 * and the PPQN we can compute how many microseconds a time division
 * lasts.
 * tempo / time_div = microseconds per tick
 * time_div / tempo = ticks per microsecond
 * We can also theoretically get the BPM this way, but in most game
 * midi files this value will be kinda meaningless since conversion
 * between variant formats can squeeze or stretch notes to fit a set
 * PPQN, so ticks per microseconds will generally be more accurate.
 * 60000000 / tempo = BPM
 *******************************************************************/
int MIDIPlayer::getLength()
{
	size_t microseconds = 0;
	size_t pos = 0;
	size_t end = data.getSize();
	size_t track_counter = 0;
	uint16_t num_tracks = 0;
	uint16_t format = 0;
	uint16_t time_div = 0;
	int tempo = 500000; // Default value to assume if there are no tempo change event
	bool smpte = false;

	while (pos + 8 < end)
	{
		size_t chunk_name = READ_B32(data, pos);
		size_t chunk_size = READ_B32(data, pos+4);
		pos += 8;
		size_t chunk_end = pos + chunk_size;
		uint8_t running_status = 0;
		if (chunk_name == (size_t)(('M'<<24)|('T'<<16)|('h'<<8)|'d')) // MThd
		{
			format = READ_B16(data, pos);
			num_tracks = READ_B16(data, pos + 2);
			time_div = READ_B16(data, pos + 4);
			if (data[pos + 4] & 0x80)
			{
				smpte = true;
				time_div = (256 - data[pos + 4]) * data[pos + 5];
			}
			if (time_div == 0) // Not a valid MIDI file
				return 0;
		}
		else if (chunk_name == (size_t)(('M'<<24)|('T'<<16)|('r'<<8)|'k')) // MTrk
		{
			size_t tpos = pos;
			size_t tracklength = 0;
			while (tpos + 4 < chunk_end)
			{
				// Read delta time
				size_t dtime = 0;
				for (int a = 0; a < 4; ++a)
				{
					dtime = (dtime<<7) + (data[tpos] & 0x7F);
					if ((data[tpos++] & 0x80) != 0x80)
						break;
				}
				// Compute length in microseconds
				if (smpte) tracklength += dtime * time_div;
				else tracklength += dtime * tempo / time_div;

				// Update status
				uint8_t evtype = 0;
				uint8_t status = data[tpos++];
				size_t evsize = 0;
				if (status < 0x80)
				{
					evtype = status;
					status = running_status;
				}
				else
				{
					running_status = status;
					evtype = data[tpos++];
				}
				// Handle meta events
				if (status == 0xFF)
				{
					evsize = 0;
					for (int a = 0; a < 4; ++a)
					{
						evsize = (evsize<<7) + (data[tpos]&0x7F);
						if ((data[tpos++] & 0x80) != 0x80)
							break;
					}

					// Tempo event is important
					if (evtype == 0x51)
						tempo = READ_B24(data, tpos);

					tpos += evsize;
				}
				// Handle other events. Program change and channel aftertouch
				// have only one parameter, other non-meta events have two.
				// Sysex events have variable length
				else switch (status & 0xF0)
				{
					case 0xC0:	// Program Change
					case 0xD0:	// Channel Aftertouch
						break;
					case 0xF0:	// Sysex events
						evsize = 0;
						for (int a = 0; a < 4; ++a)
						{
							evsize = (evsize<<7) + (data[tpos]&0x7F);
							if ((data[tpos++] & 0x80) != 0x80)
								break;
						}
						tpos += evsize;
						break;
					default:
						tpos++;	// Skip next parameter
				}
			}
			// Is this the longest track yet?
			// [TODO] MIDI Format 2 has different songs on different tracks
			if (tracklength > microseconds)
				microseconds = tracklength;
		}
		pos = chunk_end;
	}
	// MIDI durations are in microseconds
	return (int)(microseconds / 1000);
}

/* MIDIPlayer::setVolume
 * Sets the volume of the midi player
 *******************************************************************/
bool MIDIPlayer::setVolume(int volume)
{
	if (!isReady())
		return false;

	// Clamp volume
	if (volume > 100) volume = 100;
	if (volume < 0) volume = 0;

#ifndef NO_FLUIDSYNTH
	fluid_synth_set_gain(fs_synth, volume*0.01f);
#endif // NO_FLUIDSYNTH

	return true;
}

/* MIDIPlayer::getInfo
 * Parses the MIDI data to find text events, and return a string
 * where they are each on a separate line. MIDI text events include:
 * Text event (FF 01)
 * Copyright notice (FF 02)
 * Track title (FF 03)
 * Instrument name (FF 04)
 * Lyrics (FF 05)
 * Marker (FF 06)
 * Cue point (FF 07)
 *******************************************************************/
string MIDIPlayer::getInfo()
{
	string ret = wxEmptyString;
	size_t pos = 0;
	size_t end = data.getSize();
	size_t track_counter = 0;
	uint16_t num_tracks = 0;
	uint16_t format = 0;

	while (pos + 8 < end)
	{
		size_t chunk_name = READ_B32(data, pos);
		size_t chunk_size = READ_B32(data, pos+4);
		pos += 8;
		size_t chunk_end = pos + chunk_size;
		uint8_t running_status = 0;
		if (chunk_name == (size_t)(('M'<<24)|('T'<<16)|('h'<<8)|'d')) // MThd
		{
			format = READ_B16(data, pos);
			num_tracks = READ_B16(data, pos + 2);
			uint16_t time_div = READ_B16(data, pos + 4);
			if (format == 0)
				ret += S_FMT("MIDI format 0 with time division %u\n", time_div);
			else ret += S_FMT("MIDI format %u with %u tracks and time division %u\n", 
				format, num_tracks, time_div);
		}
		else if (chunk_name == (size_t)(('M'<<24)|('T'<<16)|('r'<<8)|'k')) // MTrk
		{
			if (format == 2)
				ret += S_FMT("\nTrack %u/%u\n", ++track_counter, num_tracks);
			size_t tpos = pos;
			while (tpos + 4 < chunk_end)
			{
				// Skip past delta time
				for (int a = 0; a < 4; ++a)
					if ((data[tpos++] & 0x80) != 0x80)
						break;

				// Update status
				uint8_t evtype = 0;
				uint8_t status = data[tpos++];
				size_t evsize = 0;
				if (status < 0x80)
				{
					evtype = status;
					status = running_status;
				}
				else
				{
					running_status = status;
					evtype = data[tpos++];
				}
				// Handle meta events
				if (status == 0xFF)
				{
					evsize = 0;
					for (int a = 0; a < 4; ++a)
					{
						evsize = (evsize<<7) + (data[tpos]&0x7F);
						if ((data[tpos++] & 0x80) != 0x80)
							break;
					}

					string tmp = wxEmptyString;
					if (evtype > 0 && evtype < 8 && evsize)
						tmp.Append((const char*)(&data[tpos]), evsize);

					switch (evtype)
					{
					case 1: ret += S_FMT("Text: %s\n", tmp); break;
					case 2: ret += S_FMT("Copyright: %s\n", tmp); break;
					case 3: ret += S_FMT("Title: %s\n", tmp); break;
					case 4: ret += S_FMT("Instrument: %s\n", tmp); break;
					case 5: ret += S_FMT("Lyrics: %s\n", tmp); break;
					case 6: ret += S_FMT("Marker: %s\n", tmp); break;
					case 7: ret += S_FMT("Cue point: %s\n", tmp); break;
					default: break;
					}
					tpos += evsize;
				}
				// Handle other events. Program change and channel aftertouch
				// have only one parameter, other non-meta events have two.
				// Sysex events have variable length
				else switch (status & 0xF0)
				{
					case 0xC0:	// Program Change
					case 0xD0:	// Channel Aftertouch
						break;
					case 0xF0:	// Sysex events
						evsize = 0;
						for (int a = 0; a < 4; ++a)
						{
							evsize = (evsize<<7) + (data[tpos]&0x7F);
							if ((data[tpos++] & 0x80) != 0x80)
								break;
						}
						tpos += evsize;
						break;
					default:
						tpos++;	// Skip next parameter
				}
			}
		}
		pos = chunk_end;
	}
	return ret;
}
