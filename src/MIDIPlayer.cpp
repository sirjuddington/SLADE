
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
#include <wx/stdpaths.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
#ifndef NO_FLUIDSYNTH
MIDIPlayer*	MIDIPlayer::instance = NULL;
CVAR(String, fs_soundfont_path, "", CVAR_SAVE);
CVAR(String, fs_driver, "", CVAR_SAVE);
#endif

MIDIPlayerApp* MIDIPlayerApp::instance = NULL;

/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, snd_volume)
EXTERN_CVAR(String, snd_timidity_path)
EXTERN_CVAR(String, snd_timidity_options)

#ifndef NO_FLUIDSYNTH

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

	// Set fluidsynth driver to alsa in linux (no idea why it defaults to jack)
#if !defined __WXMSW__ && !defined __WXOSX__
	if (fs_driver == "")
		fs_driver = "alsa";
#endif

	// Init soundfont path
	if (fs_soundfont_path == "")
	{
#ifdef __WXGTK__
		fs_soundfont_path = "/usr/share/sounds/sf2/FluidR3_GM.sf2:/usr/share/sounds/sf2/FluidR3_GS.sf2";
#else
		wxLogMessage("Warning: No fluidsynth soundfont set, MIDI playback will not work");
#endif
	}

	// Setup fluidsynth
	initFluidsynth();
	reloadSoundfont();

	if (!fs_player || !fs_adriver)
		wxLogMessage("Warning: Failed to initialise FluidSynth, MIDI playback disabled");
}

/* MIDIPlayer::~MIDIPlayer
 * MIDIPlayer class destructor
 *******************************************************************/
MIDIPlayer::~MIDIPlayer()
{
	delete_fluid_audio_driver(fs_adriver);
	delete_fluid_player(fs_player);
	delete_fluid_synth(fs_synth);
	delete_fluid_settings(fs_settings);
}

/* MIDIPlayer::initFluidsynth
 * Initialises fluidsynth
 *******************************************************************/
bool MIDIPlayer::initFluidsynth()
{
	// Don't re-init
	if (fs_initialised)
		return true;

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

	// Init unsuccessful
	return false;
}

/* MIDIPlayer::reloadSoundfont
 * Reloads the current soundfont
 *******************************************************************/
bool MIDIPlayer::reloadSoundfont()
{
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
}

/* MIDIPlayer::openFile
 * Opens the MIDI file at [filename] for playback. Returns true if
 * successful, false otherwise
 *******************************************************************/
bool MIDIPlayer::openFile(string filename)
{
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
		return false;
}

/* MIDIPlayer::play
 * Begins playback of the currently loaded MIDI stream. Returns true
 * if successful, false otherwise
 *******************************************************************/
bool MIDIPlayer::play()
{
	if (!fs_initialised)
		return false;

	return (fluid_player_play(fs_player) == FLUID_OK);
}

/* MIDIPlayer::pause
 * Pauses playback of the currently loaded MIDI stream
 *******************************************************************/
bool MIDIPlayer::pause()
{
	if (!fs_initialised)
		return false;

	return stop();
}

/* MIDIPlayer::stop
 * Stops playback of the currently loaded MIDI stream
 *******************************************************************/
bool MIDIPlayer::stop()
{
	if (!fs_initialised)
		return false;

	fluid_player_stop(fs_player);
	fluid_synth_system_reset(fs_synth);
	return true;
}

/* MIDIPlayer::isPlaying
 * Returns true if the MIDI stream is currently playing, false if not
 *******************************************************************/
bool MIDIPlayer::isPlaying()
{
	if (!fs_initialised)
		return false;

	return (fluid_player_get_status(fs_player) == FLUID_PLAYER_PLAYING);
}

/* MIDIPlayer::getPosition
 * Returns the current position of the playing MIDI stream
 *******************************************************************/
int MIDIPlayer::getPosition()
{
	// Cannot currently seek in fluidsynth
	return 0;
}

/* MIDIPlayer::setPosition
 * Seeks to [pos] in the currently loaded MIDI stream
 *******************************************************************/
bool MIDIPlayer::setPosition(int pos)
{
	// Cannot currently seek in fluidsynth
	return false;
}

/* MIDIPlayer::getLength
 * Returns the length (or maximum position) of the currently loaded
 * MIDI stream
 *******************************************************************/
int MIDIPlayer::getLength()
{
	// Cannot currently get length in fluidsynth
	return 0;
}

/* MIDIPlayer::setVolume
 * Sets the volume of the midi player
 *******************************************************************/
bool MIDIPlayer::setVolume(int volume)
{
	if (!fs_initialised)
		return false;

	// Clamp volume
	if (volume > 100) volume = 100;
	if (volume < 0) volume = 0;

	fluid_synth_set_gain(fs_synth, volume*0.01f);

	return true;
}

#endif

/*******************************************************************
 * MIDIPLAYER TIMIDITY IMPLEMENTATION
 *******************************************************************/

/* MIDIPlayerApp::MIDIPlayerApp
 * MIDIPlayerApp class constructor
 *******************************************************************/
MIDIPlayerApp::MIDIPlayerApp()
{
	program = NULL;
	file = "";
}

/* MIDIPlayerApp::openFile
 * Tell to timidity where is our midi
 *******************************************************************/
void MIDIPlayerApp::openFile(string filename)
{
	file = filename;
}

/* MIDIPlayerApp::~MIDIPlayerApp
 * MIDIPlayerApp class destructor
 *******************************************************************/
MIDIPlayerApp::~MIDIPlayerApp()
{
	stop();

	if (program)
		delete program;
}

/* MIDIPlayerApp::play
 * Begins playback by starting the new timidity process
 *******************************************************************/
bool MIDIPlayerApp::play()
{
	// prevent from process copies
	stop();

	string commandline = snd_timidity_path + " " + file + " " + snd_timidity_options;
	program = wxProcess::Open(commandline);

	if (isPlaying())
		return true;

	return false;
}

/* MIDIPlayerApp::stop
 * Stops playback by killing current timidity process
 *******************************************************************/
bool MIDIPlayerApp::stop()
{
	if (isPlaying())
	{
		int pid = program->GetPid();

		wxProcess::Kill(pid);
		return true;
	}
	return false;
}

/* MIDIPlayerApp::isPlaying
 * Returns true if timidity is running, false if not
 *******************************************************************/
bool MIDIPlayerApp::isPlaying()
{
	if (!program)
		return false;

	int pid = program->GetPid();
	// also ignore zero pid
	if (!pid || !wxProcess::Exists(pid))
		return false;

	return true;
}

/* MIDIPlayerApp::Reset
 * Reset everything
 *******************************************************************/
void MIDIPlayerApp::reset()
{
	stop();

	file = "";
	program = NULL;
}
