
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <fluidsynth.h>

using namespace slade;
using namespace audio;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
#ifndef NO_FLUIDSYNTH
CVAR(String, snd_midi_player, "fluidsynth", CVar::Flag::Save)
#else
CVAR(String, snd_midi_player, "none", CVar::Flag::Save)
#endif
CVAR(String, fs_soundfont_path, "", CVar::Flag::Save)
CVAR(String, fs_driver, "", CVar::Flag::Save)
CVAR(String, snd_timidity_path, "", CVar::Flag::Save)
CVAR(String, snd_timidity_options, "", CVar::Flag::Save)
namespace slade::audio
{
unique_ptr<MIDIPlayer> midi_player;
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

		// Set fluidsynth driver to pulseaudio in linux (no idea why it defaults to jack)
		if (app::platform() == app::Platform::Linux && fs_driver.value.empty())
			fs_driver = "pulseaudio";

		// Init soundfont path
		if (fs_soundfont_path.value.empty())
		{
			if (app::platform() == app::Platform::Linux)
				fs_soundfont_path =
					"/usr/share/sounds/sf2/FluidR3_GM.sf2"
					":/usr/share/sounds/sf2/FluidR3_GS.sf2"
					":/usr/share/sounds/sf2/default-GM.sf2";
		}

		// Setup fluidsynth
		initFluidsynth();
		FluidSynthMIDIPlayer::reloadSoundfont();

		if (!fs_player_ || !fs_adriver_)
			log::warning(1, "Failed to initialise FluidSynth, MIDI playback disabled");
	}

	// -------------------------------------------------------------------------
	// FluidSynthMIDIPlayer class destructor
	// -------------------------------------------------------------------------
	~FluidSynthMIDIPlayer() override
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

		char separator = app::platform() == app::Platform::Windows ? ';' : ':';

		// Unload any current soundfont
		for (int a = fs_soundfont_ids_.size() - 1; a >= 0; --a)
		{
			if (fs_soundfont_ids_[a] != FLUID_FAILED)
				fluid_synth_sfunload(fs_synth_, fs_soundfont_ids_[a], 1);
			fs_soundfont_ids_.pop_back();
		}

		// Load soundfonts
		auto paths  = strutil::split(fs_soundfont_path, separator);
		bool retval = false;
		for (int a = paths.size() - 1; a >= 0; --a)
		{
			const auto& path = paths[a];
			if (!path.empty())
			{
				int fs_id = fluid_synth_sfload(fs_synth_, path.c_str(), 1);
				fs_soundfont_ids_.push_back(fs_id);
				if (fs_id != FLUID_FAILED)
					retval = true;
			}
		}

		// If no soundfont was loaded, use the internal soundfont in the program resource
		if (!retval)
		{
			auto sf_entry = app::programResource()->entryAtPath("soundfont/default.sf2");
			if (sf_entry)
			{
				// Export to temp file
				auto path = app::path("default.sf2", app::Dir::Temp);
				sf_entry->exportFile(path);

				int fs_id = fluid_synth_sfload(fs_synth_, path.c_str(), 1);
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
	bool openFile(const string& filename) override
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
			fluid_player_add(fs_player_, filename.c_str());
			parseMidiTiming(filename);
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
			// fluid_player_set_loop(fs_player_, -1);
			fluid_player_add_mem(fs_player_, mc.data(), mc.size());
			parseMidiTimingFromData(mc);
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
		if (!fs_initialised_ || isPlaying())
			return false;

		return fluid_player_play(fs_player_) == FLUID_OK;
	}

	// -------------------------------------------------------------------------
	// Pauses playback of the currently loaded MIDI stream
	// -------------------------------------------------------------------------
	bool pause() override
	{
		if (!isPlaying())
			return false;

		auto ok = fluid_player_stop(fs_player_) == FLUID_OK;
		fluid_synth_all_notes_off(fs_synth_, -1);

		return ok;
	}

	// -------------------------------------------------------------------------
	// Stops playback of the currently loaded MIDI stream
	// -------------------------------------------------------------------------
	bool stop() override
	{
		fluid_player_stop(fs_player_);
		fluid_synth_all_notes_off(fs_synth_, -1);
		fluid_player_seek(fs_player_, 0);

		return true;
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
		if (!fs_initialised_ || !fs_player_)
			return 0;

		// Get current tick position from FluidSynth
		auto current_tick = fluid_player_get_current_tick(fs_player_);

		// Convert to milliseconds
		return tickToMilliseconds(current_tick);
	}

	// -------------------------------------------------------------------------
	// Seeks to [pos] in the currently loaded MIDI stream
	// -------------------------------------------------------------------------
	bool setPosition(int pos) override
	{
		if (!fs_initialised_ || !fs_player_ || tempo_changes_.empty())
			return false;

		// Convert milliseconds to ticks
		auto target_tick = millisecondsToTick(pos);
		if (target_tick < 0)
			return false;

		auto was_playing = isPlaying();

		// Stop playback and clear all notes
		if (was_playing)
			fluid_player_stop(fs_player_);
		fluid_synth_all_notes_off(fs_synth_, -1);

		// Seek to the target tick
		if (fluid_player_seek(fs_player_, target_tick) != FLUID_OK)
			return false;

		// Resume playback if it was playing before
		if (was_playing)
			fluid_player_play(fs_player_);

		return true;
	}

	// -------------------------------------------------------------------------
	// Sets the volume of the midi player
	// -------------------------------------------------------------------------
	bool setVolume(int volume) override
	{
		if (!isReady())
			return false;

		volume = std::clamp(volume, 0, 100);

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

	// MIDI timing information for seeking
	struct MidiTick
	{
		int tick;         // Tick position
		int milliseconds; // Time in milliseconds
		int tempo;        // Tempo at this point (microseconds per quarter note)
	};
	vector<MidiTick> tempo_changes_;
	int              time_division_ = 0; // MIDI time division (PPQN)

	// -------------------------------------------------------------------------
	// Initialises fluidsynth
	// -------------------------------------------------------------------------
	bool initFluidsynth()
	{
		// Don't re-init
		if (fs_initialised_)
			return true;

		// Init fluidsynth settings
		fs_settings_         = new_fluid_settings();
		string fs_driver_str = fs_driver;
		if (!fs_driver_str.empty())
			fluid_settings_setstr(fs_settings_, "audio.driver", fs_driver_str.c_str());

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
			fs_synth_ = nullptr;
			return false;
		}

		// Init unsuccessful
		return false;
	}

	// -------------------------------------------------------------------------
	// Parses MIDI file at [filename] to extract timing information for seeking
	// -------------------------------------------------------------------------
	void parseMidiTiming(const string& filename)
	{
		// Read the file
		MemChunk file_data;
		if (!file_data.importFile(filename))
			return;

		parseMidiTimingFromData(file_data);
	}

	// -------------------------------------------------------------------------
	// Parses MIDI [data] to extract timing information for seeking
	// -------------------------------------------------------------------------
	void parseMidiTimingFromData(const MemChunk& data)
	{
		tempo_changes_.clear();
		time_division_ = 0;

		unsigned pos   = 0;
		unsigned end   = data.size();
		int      tempo = 500000; // Default tempo (120 BPM)
		bool     smpte = false;

		// Structure to hold tempo changes from all tracks
		vector<MidiTick> tempo_changes;

		while (pos + 8 < end)
		{
			auto chunk_name = data.readB32(pos);
			auto chunk_size = data.readB32(pos + 4);
			pos += 8;
			auto chunk_end = pos + chunk_size;

			if (chunk_name == static_cast<u32>(('M' << 24) | ('T' << 16) | ('h' << 8) | 'd')) // MThd
			{
				time_division_ = data.readB16(pos + 4);
				if (data[pos + 4] & 0x80)
				{
					smpte          = true;
					time_division_ = (256 - data[pos + 4]) * data[pos + 5];
				}
			}
			else if (chunk_name == static_cast<u32>(('M' << 24) | ('T' << 16) | ('r' << 8) | 'k')) // MTrk
			{
				auto tpos           = pos;
				int  current_tick   = 0;
				u8   running_status = 0;

				while (tpos + 4 < chunk_end)
				{
					// Read delta time
					int dtime = 0;
					for (int a = 0; a < 4; ++a)
					{
						dtime = (dtime << 7) + (data[tpos] & 0x7F);
						if ((data[tpos++] & 0x80) != 0x80)
							break;
					}

					current_tick += dtime;

					// Read event
					u8       evtype = 0;
					u8       status = data[tpos++];
					unsigned evsize = 0;

					if (status < 0x80)
					{
						evtype = status;
						status = running_status;
					}
					else
					{
						running_status = status;
						evtype         = data[tpos++];
					}

					// Handle meta events
					if (status == 0xFF)
					{
						evsize = 0;
						for (int a = 0; a < 4; ++a)
						{
							evsize = (evsize << 7) + (data[tpos] & 0x7F);
							if ((data[tpos++] & 0x80) != 0x80)
								break;
						}

						// Tempo change event
						if (evtype == 0x51 && evsize == 3)
						{
							int new_tempo = data.readB24(tpos);
							tempo_changes.push_back({ .tick = current_tick, .milliseconds = 0, .tempo = new_tempo });
						}

						tpos += evsize;
					}
					else
					{
						// Handle other events
						switch (status & 0xF0)
						{
						case 0xC0: // Program Change
						case 0xD0: // Channel Aftertouch
							break;
						case 0xF0: // Sysex events
							evsize = 0;
							for (int a = 0; a < 4; ++a)
							{
								evsize = (evsize << 7) + (data[tpos] & 0x7F);
								if ((data[tpos++] & 0x80) != 0x80)
									break;
							}
							tpos += evsize;
							break;
						default: tpos++; // Skip next parameter
						}
					}
				}
			}

			pos = chunk_end;
		}

		// Sort tempo changes by tick position
		std::ranges::sort(tempo_changes, [](const MidiTick& a, const MidiTick& b) { return a.tick < b.tick; });

		// Build timing map from tempo changes
		tempo_changes_.push_back({ .tick = 0, .milliseconds = 0, .tempo = tempo });

		int current_tick = 0;
		int current_ms   = 0;

		for (auto& change : tempo_changes)
		{
			// Calculate time elapsed since last tempo change
			int tick_delta = change.tick - current_tick;
			if (tick_delta > 0)
			{
				if (smpte)
					current_ms += (tick_delta * time_division_) / 1000;
				else
					current_ms += (tick_delta * tempo) / (time_division_ * 1000);
			}

			// Record this tempo change
			change.milliseconds = current_ms;
			current_tick = change.tick;
			tempo        = change.tempo;
			tempo_changes_.push_back(change);
		}
	}

	// -------------------------------------------------------------------------
	// Converts [ms] to MIDI ticks using the timing information
	// -------------------------------------------------------------------------
	int millisecondsToTick(int ms) const
	{
		if (tempo_changes_.empty() || time_division_ == 0)
			return -1;

		// Clamp to valid range
		if (ms < 0)
			return 0;

		// Find the tempo segment containing this time
		// We want the last segment with milliseconds <= [ms]
		unsigned segment = 0;
		for (unsigned i = 1; i < tempo_changes_.size(); ++i)
		{
			if (tempo_changes_[i].milliseconds > ms)
				break;
			segment = i;
		}

		const auto& tick_info = tempo_changes_[segment];
		auto        ms_offset = ms - tick_info.milliseconds;

		// Avoid division by zero
		if (tick_info.tempo == 0)
			return tick_info.tick;

		// Convert milliseconds offset to ticks (use 64-bit to avoid overflow)
		auto tick_offset = (static_cast<i64>(ms_offset) * time_division_ * 1000) / tick_info.tempo;

		return tick_info.tick + static_cast<int>(tick_offset);
	}

	// -------------------------------------------------------------------------
	// Converts MIDI [tick] to milliseconds using the timing information
	// -------------------------------------------------------------------------
	int tickToMilliseconds(int tick) const
	{
		if (tempo_changes_.empty() || time_division_ == 0)
			return 0;

		// Clamp to valid range
		if (tick < 0)
			return 0;

		// Find the tempo segment containing this tick
		// We want the last segment whose tick value is <= [tick]
		unsigned segment = 0;
		for (unsigned i = 1; i < tempo_changes_.size(); ++i)
		{
			if (tempo_changes_[i].tick > tick)
				break;
			segment = i;
		}

		const auto& tick_info   = tempo_changes_[segment];
		auto        tick_offset = tick - tick_info.tick;

		// Avoid division by zero
		if (time_division_ == 0)
			return tick_info.milliseconds;

		// Convert tick offset to milliseconds (use 64-bit to avoid overflow)
		auto ms_offset = (static_cast<i64>(tick_offset) * tick_info.tempo) / (time_division_ * 1000);

		return tick_info.milliseconds + static_cast<int>(ms_offset);
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
	~TimidityMIDIPlayer() override { TimidityMIDIPlayer::stop(); }

	// -------------------------------------------------------------------------
	// Returns true if the MIDIPlayer has a soundfont loaded
	// -------------------------------------------------------------------------
	bool isSoundfontLoaded() override { return true; }

	// -------------------------------------------------------------------------
	// Opens the MIDI file at [filename] for playback.
	// Returns true if successful, false otherwise
	// -------------------------------------------------------------------------
	bool openFile(const string& filename) override
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

		file_ = app::path("slade-timidity.mid", app::Dir::Temp);
		mc.exportFile(file_);

		return true;
	}

	// -------------------------------------------------------------------------
	// Returns true if the MIDIPlayer is ready to play some MIDI
	// -------------------------------------------------------------------------
	bool isReady() override { return !snd_timidity_path.empty(); }

	// -------------------------------------------------------------------------
	// Begins playback of the currently loaded MIDI stream.
	// Returns true if successful, false otherwise
	// -------------------------------------------------------------------------
	bool play() override
	{
		stop();
		play_start_time_ = timer_->getElapsedTime();

		// Setup environment and command line to run
		wxExecuteEnv env;
		env.cwd          = wxutil::strFromView(strutil::Path::pathOf(snd_timidity_path));
		auto commandline = fmt::format(R"("{}" "{}" {})", snd_timidity_path.value, file_, snd_timidity_options.value);

		// Execute program
		pid_ = wxExecute(wxString::FromUTF8(commandline), wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, nullptr, &env);

		return pid_ > 0 && wxProcess::Exists(pid_);
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

		if (pid_ > 0)
		{
			if (isPlaying())
			{
				if (app::platform() == app::Platform::Windows)
					wxProcess::Kill(pid_, wxSIGKILL, wxKILL_CHILDREN);
				else
					wxProcess::Kill(pid_);
			}

			stopped = !(wxProcess::Exists(pid_));
			pid_    = 0;
		}

		return stopped;
	}

	// -------------------------------------------------------------------------
	// Returns true if the MIDI stream is currently playing
	// -------------------------------------------------------------------------
	bool isPlaying() override
	{
		if (pid_ <= 0)
			return false;

		return !(!pid_ || !wxProcess::Exists(pid_)); // also ignore zero pid
	}

	// -------------------------------------------------------------------------
	// Returns the current position of the playing MIDI stream
	// -------------------------------------------------------------------------
	int position() override
	{
		// We cannot query this information from timidity, so we calculate elapsed time
		return (timer_->getElapsedTime() - play_start_time_).asMilliseconds();
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
	long     pid_ = 0;
	sf::Time play_start_time_;
};


// -----------------------------------------------------------------------------
//
// audio Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::audio
{
// -----------------------------------------------------------------------------
// MIDIPlayer class constructor
// -----------------------------------------------------------------------------
MIDIPlayer::MIDIPlayer() : timer_{ new sf::Clock() } {}

// -----------------------------------------------------------------------------
// MIDIPlayer class destructor
// -----------------------------------------------------------------------------
MIDIPlayer::~MIDIPlayer() = default;

// -----------------------------------------------------------------------------
// Returns the current MIDIPlayer instance.
// Creates one if there is no current instance, depending on what is configured
// (and available)
// -----------------------------------------------------------------------------
MIDIPlayer& midiPlayer()
{
	if (!midi_player)
	{
#ifndef NO_FLUIDSYNTH
		if (strutil::equalCI(snd_midi_player, "fluidsynth"))
			midi_player = std::make_unique<FluidSynthMIDIPlayer>();
		else if (strutil::equalCI(snd_midi_player, "timidity"))
			midi_player = std::make_unique<TimidityMIDIPlayer>();
#else
		if (strutil::equalCI(snd_midi_player, "timidity"))
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
void resetMIDIPlayer()
{
	if (midi_player)
		midi_player.reset(nullptr);
}

// -----------------------------------------------------------------------------
// Returns the length (or maximum position) of the MIDI [data], in milliseconds.
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
int midiLength(const MemChunk& data)
{
	size_t   microseconds  = 0;
	size_t   pos           = 0;
	size_t   end           = data.size();
	size_t   track_counter = 0;
	uint16_t num_tracks    = 0;
	uint16_t format        = 0;
	uint16_t time_div      = 0;
	int      tempo         = 500000; // Default value to assume if there are no tempo change event
	bool     smpte         = false;

	while (pos + 8 < end)
	{
		size_t chunk_name = data.readB32(pos);
		size_t chunk_size = data.readB32(pos + 4);
		pos += 8;
		size_t  chunk_end      = pos + chunk_size;
		uint8_t running_status = 0;
		if (chunk_name == static_cast<size_t>(('M' << 24) | ('T' << 16) | ('h' << 8) | 'd')) // MThd
		{
			format     = data.readB16(pos);
			num_tracks = data.readB16(pos + 2);
			time_div   = data.readB16(pos + 4);
			if (data[pos + 4] & 0x80)
			{
				smpte    = true;
				time_div = (256 - data[pos + 4]) * data[pos + 5];
			}
			if (time_div == 0) // Not a valid MIDI file
				return 0;
		}
		else if (chunk_name == static_cast<size_t>(('M' << 24) | ('T' << 16) | ('r' << 8) | 'k')) // MTrk
		{
			size_t tpos        = pos;
			size_t tracklength = 0;
			while (tpos + 4 < chunk_end)
			{
				// Read delta time
				size_t dtime = 0;
				for (int a = 0; a < 4; ++a)
				{
					dtime = (dtime << 7) + (data[tpos] & 0x7F);
					if ((data[tpos++] & 0x80) != 0x80)
						break;
				}
				// Compute length in microseconds
				if (smpte)
					tracklength += dtime * time_div;
				else
					tracklength += dtime * tempo / time_div;

				// Update status
				uint8_t evtype = 0;
				uint8_t status = data[tpos++];
				size_t  evsize = 0;
				if (status < 0x80)
				{
					evtype = status;
					status = running_status;
				}
				else
				{
					running_status = status;
					evtype         = data[tpos++];
				}
				// Handle meta events
				if (status == 0xFF)
				{
					evsize = 0;
					for (int a = 0; a < 4; ++a)
					{
						evsize = (evsize << 7) + (data[tpos] & 0x7F);
						if ((data[tpos++] & 0x80) != 0x80)
							break;
					}

					// Tempo event is important
					if (evtype == 0x51)
						tempo = data.readB24(tpos);

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
							evsize = (evsize << 7) + (data[tpos] & 0x7F);
							if ((data[tpos++] & 0x80) != 0x80)
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
	return static_cast<int>(microseconds / 1000);
}

// -----------------------------------------------------------------------------
// Parses the MIDI [data] to find text events, and return a string where they
// are each on a separate line. MIDI text events include:
// Text event (FF 01)
// Copyright notice (FF 02)
// Track title (FF 03)
// Instrument name (FF 04)
// Lyrics (FF 05)
// Marker (FF 06)
// Cue point (FF 07)
// -----------------------------------------------------------------------------
string midiInfo(const MemChunk& data)
{
	string   ret;
	size_t   pos           = 0;
	size_t   end           = data.size();
	size_t   track_counter = 0;
	uint16_t num_tracks    = 0;
	uint16_t format        = 0;

	while (pos + 8 < end)
	{
		size_t chunk_name = data.readB32(pos);
		size_t chunk_size = data.readB32(pos + 4);
		pos += 8;
		size_t  chunk_end      = pos + chunk_size;
		uint8_t running_status = 0;
		if (chunk_name == static_cast<size_t>(('M' << 24) | ('T' << 16) | ('h' << 8) | 'd')) // MThd
		{
			format            = data.readB16(pos);
			num_tracks        = data.readB16(pos + 2);
			uint16_t time_div = data.readB16(pos + 4);
			if (format == 0)
				ret += fmt::format("MIDI format 0 with time division {}\n", time_div);
			else
				ret += fmt::format(
					"MIDI format {} with {} tracks and time division {}\n", format, num_tracks, time_div);
		}
		else if (chunk_name == static_cast<size_t>(('M' << 24) | ('T' << 16) | ('r' << 8) | 'k')) // MTrk
		{
			if (format == 2)
				ret += fmt::format("\nTrack {}/{}\n", ++track_counter, num_tracks);
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
				size_t  evsize = 0;
				if (status < 0x80)
				{
					evtype = status;
					status = running_status;
				}
				else
				{
					running_status = status;
					evtype         = data[tpos++];
				}
				// Handle meta events
				if (status == 0xFF)
				{
					evsize = 0;
					for (int a = 0; a < 4; ++a)
					{
						evsize = (evsize << 7) + (data[tpos] & 0x7F);
						if ((data[tpos++] & 0x80) != 0x80)
							break;
					}

					string tmp;
					if (evtype > 0 && evtype < 8 && evsize)
						tmp.append(reinterpret_cast<const char*>(data.data() + tpos), evsize);

					switch (evtype)
					{
					case 1:  ret += fmt::format("Text: {}\n", tmp); break;
					case 2:  ret += fmt::format("Copyright: {}\n", tmp); break;
					case 3:  ret += fmt::format("Title: {}\n", tmp); break;
					case 4:  ret += fmt::format("Instrument: {}\n", tmp); break;
					case 5:  ret += fmt::format("Lyrics: {}\n", tmp); break;
					case 6:  ret += fmt::format("Marker: {}\n", tmp); break;
					case 7:  ret += fmt::format("Cue point: {}\n", tmp); break;
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
							evsize = (evsize << 7) + (data[tpos] & 0x7F);
							if ((data[tpos++] & 0x80) != 0x80)
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
} // namespace slade::audio
