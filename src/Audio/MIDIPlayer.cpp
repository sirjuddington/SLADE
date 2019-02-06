
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MIDIPlayer.cpp
// Description: MIDIPlayer class and subclasses, handles playback of MIDI files.
//              Can only play one MIDI at a time
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
#include "MIDIPlayer.h"
#include "App.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, snd_midi_player, "none", CVar::Flag::Save)
CVAR(String, fs_soundfont_path, "", CVar::Flag::Save)
CVAR(String, fs_driver, "", CVar::Flag::Save)
CVAR(String, snd_timidity_path, "", CVar::Flag::Save)
CVAR(String, snd_timidity_options, "", CVar::Flag::Save)
namespace MIDI
{
std::unique_ptr<MIDIPlayer> midi_player;
}


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, snd_volume)


// -----------------------------------------------------------------------------
//
// MIDIPlayer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the length (or maximum position) of the currently loaded MIDI stream,
// in milliseconds.
//
// MIDI time division is the number of pulses per quarter note, aka PPQN, or
// clock tick per beat; but it doesn't tell us how long a beat or a tick lasts.
// To know that we also need to know the tempo which is a meta event and
// therefore optional. The tempo tells us how many microseconds there are in a
// quarter note, so from that and the PPQN we can compute how many microseconds
// a time division lasts.
// tempo / time_div = microseconds per tick
// time_div / tempo = ticks per microsecond
// We can also theoretically get the BPM this way, but in most game midi files
// this value will be kinda meaningless since conversion between variant formats
// can squeeze or stretch notes to fit a set PPQN, so ticks per microseconds
// will generally be more accurate.
// 60000000 / tempo = BPM
// -----------------------------------------------------------------------------
int MIDIPlayer::length()
{
	size_t   microseconds  = 0;
	size_t   pos           = 0;
	size_t   end           = data_.size();
	size_t   track_counter = 0;
	uint16_t num_tracks    = 0;
	uint16_t format        = 0;
	uint16_t time_div      = 0;
	int      tempo         = 500000; // Default value to assume if there are no tempo change event
	bool     smpte         = false;

	while (pos + 8 < end)
	{
		size_t chunk_name = data_.readB32(pos);
		size_t chunk_size = data_.readB32(pos + 4);
		pos += 8;
		size_t  chunk_end      = pos + chunk_size;
		uint8_t running_status = 0;
		if (chunk_name == (size_t)(('M' << 24) | ('T' << 16) | ('h' << 8) | 'd')) // MThd
		{
			format     = data_.readB16(pos);
			num_tracks = data_.readB16(pos + 2);
			time_div   = data_.readB16(pos + 4);
			if (data_[pos + 4] & 0x80)
			{
				smpte    = true;
				time_div = (256 - data_[pos + 4]) * data_[pos + 5];
			}
			if (time_div == 0) // Not a valid MIDI file
				return 0;
		}
		else if (chunk_name == (size_t)(('M' << 24) | ('T' << 16) | ('r' << 8) | 'k')) // MTrk
		{
			size_t tpos        = pos;
			size_t tracklength = 0;
			while (tpos + 4 < chunk_end)
			{
				// Read delta time
				size_t dtime = 0;
				for (int a = 0; a < 4; ++a)
				{
					dtime = (dtime << 7) + (data_[tpos] & 0x7F);
					if ((data_[tpos++] & 0x80) != 0x80)
						break;
				}
				// Compute length in microseconds
				if (smpte)
					tracklength += dtime * time_div;
				else
					tracklength += dtime * tempo / time_div;

				// Update status
				uint8_t evtype = 0;
				uint8_t status = data_[tpos++];
				size_t  evsize = 0;
				if (status < 0x80)
				{
					evtype = status;
					status = running_status;
				}
				else
				{
					running_status = status;
					evtype         = data_[tpos++];
				}
				// Handle meta events
				if (status == 0xFF)
				{
					evsize = 0;
					for (int a = 0; a < 4; ++a)
					{
						evsize = (evsize << 7) + (data_[tpos] & 0x7F);
						if ((data_[tpos++] & 0x80) != 0x80)
							break;
					}

					// Tempo event is important
					if (evtype == 0x51)
						tempo = data_.readB24(tpos);

					tpos += evsize;
				}
				// Handle other events. Program change and channel aftertouch
				// have only one parameter, other non-meta events have two.
				// Sysex events have variable length
				else
					switch (status & 0xF0)
					{
					case 0xC0: // Program Change
					case 0xD0: // Channel Aftertouch
						break;
					case 0xF0: // Sysex events
						evsize = 0;
						for (int a = 0; a < 4; ++a)
						{
							evsize = (evsize << 7) + (data_[tpos] & 0x7F);
							if ((data_[tpos++] & 0x80) != 0x80)
								break;
						}
						tpos += evsize;
						break;
					default: tpos++; // Skip next parameter
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

// -----------------------------------------------------------------------------
// Parses the MIDI data to find text events, and return a string where they are
// each on a separate line. MIDI text events include:
// Text event (FF 01)
// Copyright notice (FF 02)
// Track title (FF 03)
// Instrument name (FF 04)
// Lyrics (FF 05)
// Marker (FF 06)
// Cue point (FF 07)
// -----------------------------------------------------------------------------
wxString MIDIPlayer::info()
{
	wxString ret           = wxEmptyString;
	size_t   pos           = 0;
	size_t   end           = data_.size();
	size_t   track_counter = 0;
	uint16_t num_tracks    = 0;
	uint16_t format        = 0;

	while (pos + 8 < end)
	{
		size_t chunk_name = data_.readB32(pos);
		size_t chunk_size = data_.readB32(pos + 4);
		pos += 8;
		size_t  chunk_end      = pos + chunk_size;
		uint8_t running_status = 0;
		if (chunk_name == (size_t)(('M' << 24) | ('T' << 16) | ('h' << 8) | 'd')) // MThd
		{
			format            = data_.readB16(pos);
			num_tracks        = data_.readB16(pos + 2);
			uint16_t time_div = data_.readB16(pos + 4);
			if (format == 0)
				ret += S_FMT("MIDI format 0 with time division %u\n", time_div);
			else
				ret += S_FMT("MIDI format %u with %u tracks and time division %u\n", format, num_tracks, time_div);
		}
		else if (chunk_name == (size_t)(('M' << 24) | ('T' << 16) | ('r' << 8) | 'k')) // MTrk
		{
			if (format == 2)
				ret += S_FMT("\nTrack %u/%u\n", ++track_counter, num_tracks);
			size_t tpos = pos;
			while (tpos + 4 < chunk_end)
			{
				// Skip past delta time
				for (int a = 0; a < 4; ++a)
					if ((data_[tpos++] & 0x80) != 0x80)
						break;

				// Update status
				uint8_t evtype = 0;
				uint8_t status = data_[tpos++];
				size_t  evsize = 0;
				if (status < 0x80)
				{
					evtype = status;
					status = running_status;
				}
				else
				{
					running_status = status;
					evtype         = data_[tpos++];
				}
				// Handle meta events
				if (status == 0xFF)
				{
					evsize = 0;
					for (int a = 0; a < 4; ++a)
					{
						evsize = (evsize << 7) + (data_[tpos] & 0x7F);
						if ((data_[tpos++] & 0x80) != 0x80)
							break;
					}

					wxString tmp = wxEmptyString;
					if (evtype > 0 && evtype < 8 && evsize)
						tmp.Append((const char*)(&data_[tpos]), evsize);

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
				else
					switch (status & 0xF0)
					{
					case 0xC0: // Program Change
					case 0xD0: // Channel Aftertouch
						break;
					case 0xF0: // Sysex events
						evsize = 0;
						for (int a = 0; a < 4; ++a)
						{
							evsize = (evsize << 7) + (data_[tpos] & 0x7F);
							if ((data_[tpos++] & 0x80) != 0x80)
								break;
						}
						tpos += evsize;
						break;
					default: tpos++; // Skip next parameter
					}
			}
		}
		pos = chunk_end;
	}
	return ret;
}


// -----------------------------------------------------------------------------
// FluidSynthMIDIPlayer Class
//
// A MIDIPlayer that uses fluidsynth to play MIDI
// Requires a soundfont file to be configured (fs_soundfont_path cvar)
// -----------------------------------------------------------------------------
#ifndef NO_FLUIDSYNTH
class FluidSynthMIDIPlayer : public MIDIPlayer
{
public:
	// -------------------------------------------------------------------------
	// FluidSynthMIDIPlayer class constructor
	// -------------------------------------------------------------------------
	FluidSynthMIDIPlayer()
	{
		// Init variables
		fs_initialised_ = false;
		file_           = "";

		// Set fluidsynth driver to alsa in linux (no idea why it defaults to jack)
		if (App::platform() == App::Platform::Linux && fs_driver.value.empty())
			fs_driver = "alsa";

		// Init soundfont path
		if (fs_soundfont_path.value.empty())
		{
			if (App::platform() == App::Platform::Linux)
				fs_soundfont_path = "/usr/share/sounds/sf2/FluidR3_GM.sf2:/usr/share/sounds/sf2/FluidR3_GS.sf2";
			else
				Log::warning(1, "No FluidSynth soundfont set, MIDI playback will not work");
		}

		// Setup fluidsynth
		initFluidsynth();
		FluidSynthMIDIPlayer::reloadSoundfont();

		if (!fs_player_ || !fs_adriver_)
			Log::warning(1, "Failed to initialise FluidSynth, MIDI playback disabled");
	}

	// -------------------------------------------------------------------------
	// FluidSynthMIDIPlayer class destructor
	// -------------------------------------------------------------------------
	virtual ~FluidSynthMIDIPlayer()
	{
		FluidSynthMIDIPlayer::stop();
		delete_fluid_audio_driver(fs_adriver_);
		delete_fluid_player(fs_player_);
		delete_fluid_synth(fs_synth_);
		delete_fluid_settings(fs_settings_);
	}

	// -------------------------------------------------------------------------
	// Returns true if the MIDIPlayer has a soundfont loaded
	// -------------------------------------------------------------------------
	bool isSoundfontLoaded() override { return !fs_soundfont_ids_.empty(); }

	// -------------------------------------------------------------------------
	// Reloads the current soundfont
	// -------------------------------------------------------------------------
	bool reloadSoundfont() override
	{
		// Can't do anything if fluidsynth isn't initialised for whatever reason
		if (!fs_initialised_)
			return false;

		char separator = App::platform() == App::Platform::Windows ? ';' : ':';

		// Unload any current soundfont
		for (int a = fs_soundfont_ids_.size() - 1; a >= 0; --a)
		{
			if (fs_soundfont_ids_[a] != FLUID_FAILED)
				fluid_synth_sfunload(fs_synth_, fs_soundfont_ids_[a], 1);
			fs_soundfont_ids_.pop_back();
		}

		// Load soundfonts
		auto paths  = wxSplit(fs_soundfont_path, separator);
		bool retval = false;
		for (int a = paths.size() - 1; a >= 0; --a)
		{
			wxString path = paths[a];
			if (!path.empty())
			{
				int fs_id = fluid_synth_sfload(fs_synth_, CHR(path), 1);
				fs_soundfont_ids_.push_back(fs_id);
				if (fs_id != FLUID_FAILED)
					retval = true;
			}
		}

		return retval;
	}

	// -------------------------------------------------------------------------
	// Opens the MIDI file at [filename] for playback.
	// Returns true if successful, false otherwise
	// -------------------------------------------------------------------------
	bool openFile(wxString filename) override
	{
		file_ = filename;
		if (!fs_initialised_)
			return false;

		// Delete+Recreate player
		delete_fluid_player(fs_player_);
		fs_player_ = nullptr;
		fs_player_ = new_fluid_player(fs_synth_);

		// Open midi
		if (fs_player_)
		{
			fluid_player_add(fs_player_, CHR(filename));
			return true;
		}

		return false;
	}

	// -------------------------------------------------------------------------
	// Opens the MIDI data contained in [mc] for playback.
	// Returns true if successful, false otherwise
	// -------------------------------------------------------------------------
	bool openData(MemChunk& mc) override
	{
		// Open midi
		mc.seek(0, SEEK_SET);
		data_.importMem(mc.data(), mc.size());

		if (!fs_initialised_)
			return false;

		// Delete+Recreate player
		delete_fluid_player(fs_player_);
		fs_player_ = nullptr;
		fs_player_ = new_fluid_player(fs_synth_);

		if (fs_player_)
		{
			// fluid_player_set_loop(fs_player, -1);
			fluid_player_add_mem(fs_player_, mc.data(), mc.size());
			return true;
		}

		return false;
	}

	// -------------------------------------------------------------------------
	// Returns true if the MIDIPlayer is ready to play some MIDI
	// -------------------------------------------------------------------------
	bool isReady() override { return fs_initialised_ && !fs_soundfont_ids_.empty(); }

	// -------------------------------------------------------------------------
	// Begins playback of the currently loaded MIDI stream.
	// Returns true if successful, false otherwise
	// -------------------------------------------------------------------------
	bool play() override
	{
		stop();
		timer_.restart();

		if (!fs_initialised_)
			return false;

		return (fluid_player_play(fs_player_) == FLUID_OK);
	}

	// -------------------------------------------------------------------------
	// Pauses playback of the currently loaded MIDI stream
	// -------------------------------------------------------------------------
	bool pause() override
	{
		if (!isReady())
			return false;

		return stop();
	}

	// -------------------------------------------------------------------------
	// Stops playback of the currently loaded MIDI stream
	// -------------------------------------------------------------------------
	bool stop() override
	{
		bool stopped = false;

		if (fs_initialised_)
		{
			fluid_player_stop(fs_player_);
			fluid_synth_system_reset(fs_synth_);
			stopped = true;
		}

		return stopped;
	}

	// -------------------------------------------------------------------------
	// Returns true if the MIDI stream is currently playing
	// -------------------------------------------------------------------------
	bool isPlaying() override
	{
		if (!fs_initialised_)
			return false;

		return (fluid_player_get_status(fs_player_) == FLUID_PLAYER_PLAYING);
	}

	// -------------------------------------------------------------------------
	// Returns the current position of the playing MIDI stream
	// -------------------------------------------------------------------------
	int position() override
	{
		// We cannot query this information from fluidsynth, so we cheat by querying our own timer
		return timer_.getElapsedTime().asMilliseconds();
	}

	// -------------------------------------------------------------------------
	// Seeks to [pos] in the currently loaded MIDI stream
	// -------------------------------------------------------------------------
	bool setPosition(int pos) override
	{
		// Cannot currently seek in fluidsynth
		return false;
	}

	// -------------------------------------------------------------------------
	// Sets the volume of the midi player
	// -------------------------------------------------------------------------
	bool setVolume(int volume) override
	{
		if (!isReady())
			return false;

		// Clamp volume
		if (volume > 100)
			volume = 100;
		if (volume < 0)
			volume = 0;

		fluid_synth_set_gain(fs_synth_, volume * 0.01f);

		return true;
	}

private:
	fluid_settings_t*     fs_settings_ = nullptr;
	fluid_synth_t*        fs_synth_    = nullptr;
	fluid_player_t*       fs_player_   = nullptr;
	fluid_audio_driver_t* fs_adriver_  = nullptr;

	bool        fs_initialised_ = false;
	vector<int> fs_soundfont_ids_;

	// -------------------------------------------------------------------------
	// Initialises fluidsynth
	// -------------------------------------------------------------------------
	bool initFluidsynth()
	{
		// Don't re-init
		if (fs_initialised_)
			return true;

		// Init fluidsynth settings
		fs_settings_ = new_fluid_settings();
		if (!fs_driver.value.empty())
			fluid_settings_setstr(fs_settings_, "audio.driver", wxString(fs_driver).ToAscii());

		// Create fluidsynth objects
		fs_synth_   = new_fluid_synth(fs_settings_);
		fs_player_  = new_fluid_player(fs_synth_);
		fs_adriver_ = new_fluid_audio_driver(fs_settings_, fs_synth_);

		// Check init succeeded
		if (fs_synth_)
		{
			if (fs_adriver_)
			{
				setVolume(snd_volume);
				fs_initialised_ = true;
				return true;
			}

			// Driver creation unsuccessful
			delete_fluid_synth(fs_synth_);
			return false;
		}

		// Init unsuccessful
		return false;
	}
};
#endif // !NO_FLUIDSYNTH


// -----------------------------------------------------------------------------
// TimidityMIDIPlayer Class
//
// A MIDIPlayer that uses an external timidity executable to play MIDI
// -----------------------------------------------------------------------------
class TimidityMIDIPlayer : public MIDIPlayer
{
public:
	// -------------------------------------------------------------------------
	// TimidityMIDIPlayer class destructor
	// -------------------------------------------------------------------------
	virtual ~TimidityMIDIPlayer() { delete program_; }

	// -------------------------------------------------------------------------
	// Returns true if the MIDIPlayer has a soundfont loaded
	// -------------------------------------------------------------------------
	bool isSoundfontLoaded() override { return true; }

	// -------------------------------------------------------------------------
	// Opens the MIDI file at [filename] for playback.
	// Returns true if successful, false otherwise
	// -------------------------------------------------------------------------
	bool openFile(wxString filename) override
	{
		file_ = filename;
		return true;
	}

	// -------------------------------------------------------------------------
	// Opens the MIDI data contained in [mc] for playback.
	// Returns true if successful, false otherwise
	// -------------------------------------------------------------------------
	bool openData(MemChunk& mc) override
	{
		// Open midi
		mc.seek(0, SEEK_SET);
		data_.importMem(mc.data(), mc.size());

		wxFileName path(App::path("slade-timidity.mid", App::Dir::Temp));
		file_ = path.GetFullPath();
		mc.exportFile(file_);

		return true;
	}

	// -------------------------------------------------------------------------
	// Returns true if the MIDIPlayer is ready to play some MIDI
	// -------------------------------------------------------------------------
	bool isReady() override { return !snd_timidity_path.value.IsEmpty(); }

	// -------------------------------------------------------------------------
	// Begins playback of the currently loaded MIDI stream.
	// Returns true if successful, false otherwise
	// -------------------------------------------------------------------------
	bool play() override
	{
		stop();
		timer_.restart();

		wxString commandline = snd_timidity_path + " " + file_ + " " + snd_timidity_options;
		if (!((program_ = wxProcess::Open(commandline))))
			return false;

		return wxProcess::Exists(program_->GetPid());
	}

	// -------------------------------------------------------------------------
	// Pauses playback of the currently loaded MIDI stream
	// -------------------------------------------------------------------------
	bool pause() override
	{
		if (!isReady())
			return false;

		return stop();
	}

	// -------------------------------------------------------------------------
	// Stops playback of the currently loaded MIDI stream
	// -------------------------------------------------------------------------
	bool stop() override
	{
		bool stopped = false;

		if (program_)
		{
			int pid = program_->GetPid();
			if (isPlaying())
			{
				if (App::platform() == App::Platform::Windows)
					wxProcess::Kill(pid, wxSIGKILL, wxKILL_CHILDREN);
				else
					wxProcess::Kill(pid);
			}

			stopped = !(wxProcess::Exists(pid));
		}

		return stopped;
	}

	// -------------------------------------------------------------------------
	// Returns true if the MIDI stream is currently playing
	// -------------------------------------------------------------------------
	bool isPlaying() override
	{
		if (!program_)
			return false;

		int pid = program_->GetPid();
		return !(!pid || !wxProcess::Exists(pid)); // also ignore zero pid
	}

	// -------------------------------------------------------------------------
	// Returns the current position of the playing MIDI stream
	// -------------------------------------------------------------------------
	int position() override
	{
		// We cannot query this information from timidity, so we cheat by querying our own timer
		return timer_.getElapsedTime().asMilliseconds();
	}

	// -------------------------------------------------------------------------
	// Seeks to [pos] in the currently loaded MIDI stream
	// -------------------------------------------------------------------------
	bool setPosition(int pos) override
	{
		// Cannot currently seek in timidity
		return false;
	}

	// -------------------------------------------------------------------------
	// Sets the volume of the midi player
	// -------------------------------------------------------------------------
	bool setVolume(int volume) override
	{
		// Can't change the volume for timidity playback
		return isReady();
	}

private:
	wxProcess* program_ = nullptr;
};


// -----------------------------------------------------------------------------
//
// MIDI Namespace Functions
//
// -----------------------------------------------------------------------------
namespace MIDI
{
// -----------------------------------------------------------------------------
// Returns the current MIDIPlayer instance.
// Creates one if there is no current instance, depending on what is configured
// (and available)
// -----------------------------------------------------------------------------
MIDIPlayer& player()
{
	if (!midi_player)
	{
#ifndef NO_FLUIDSYNTH
		if (S_CMPNOCASE(snd_midi_player.value, "fluidsynth"))
			midi_player = std::make_unique<FluidSynthMIDIPlayer>();
		else if (S_CMPNOCASE(snd_midi_player.value, "timidity"))
			midi_player = std::make_unique<TimidityMIDIPlayer>();
#else
		if (S_CMPNOCASE(snd_midi_player.value, "timidity"))
			midi_player = std::make_unique<TimidityMIDIPlayer>();
#endif

		if (!midi_player)
			midi_player = std::make_unique<NullMIDIPlayer>();
	}

	return *midi_player;
}

// -----------------------------------------------------------------------------
// Resets the current MIDIPlayer
// -----------------------------------------------------------------------------
void resetPlayer()
{
	if (midi_player)
		midi_player.reset(nullptr);
}
} // namespace MIDI
