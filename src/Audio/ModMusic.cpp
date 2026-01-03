
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ModMusic.cpp
// Description: ModMusic class, an SFML sound stream class to play mod music
//              using DUMB
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
#include "ModMusic.h"
#include <dumb.h>

using namespace slade;
using namespace audio;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
bool ModMusic::init_done_ = false;


// -----------------------------------------------------------------------------
//
// ModMusic Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ModMusic class destructor
// -----------------------------------------------------------------------------
ModMusic::~ModMusic()
{
	close();
}

// -----------------------------------------------------------------------------
// Loads a mod file for playback
// -----------------------------------------------------------------------------
bool ModMusic::openFromFile(const string& filename)
{
	// Init DUMB if needed
	if (!init_done_)
		initDumb();

	// Close current module if any
	close();

	// Load module file
	dumb_module_ = dumb_load_any(filename.c_str(), 0, 0);
	if (dumb_module_ != nullptr)
	{
#if (SFML_VERSION_MAJOR > 2)
		initialize(2, 44100, getChannelMap());
#else
		initialize(2, 44100);
#endif
		return true;
	}
	else
	{
		log::error(fmt::format("Failed to load module music file \"{}\"", filename.c_str()));
		return false;
	}
}

// -----------------------------------------------------------------------------
// Loads mod data for playback
// -----------------------------------------------------------------------------
bool ModMusic::loadFromMemory(const uint8_t* data, const uint32_t size)
{
	// Init DUMB if needed
	if (!init_done_)
		initDumb();

	// Close current module if any
	close();

	// Load module file
	dumb_module_ = dumb_read_any(dumbfile_open_memory(reinterpret_cast<const char*>(data), size), 0, 0);
	if (dumb_module_ != nullptr)
	{
#if (SFML_VERSION_MAJOR > 2)
		initialize(2, 44100, getChannelMap());
#else
		initialize(2, 44100);
#endif
		dumb_player_ = duh_start_sigrenderer(dumb_module_, 0, 2, 0);
		return true;
	}
	else
	{
		log::error("Failed to load module music data");
		return false;
	}
}

// -----------------------------------------------------------------------------
// Returns the duration of the currently loaded mod
// -----------------------------------------------------------------------------
sf::Time ModMusic::duration() const
{
	return sf::seconds(static_cast<float>(duh_get_length(dumb_module_)) / 65536.f);
}

// -----------------------------------------------------------------------------
// Closes and unloads any currently open mod
// -----------------------------------------------------------------------------
void ModMusic::close()
{
	if (dumb_player_ != nullptr)
	{
		duh_end_sigrenderer(dumb_player_);
		dumb_player_ = nullptr;
	}
	if (dumb_module_ != nullptr)
	{
		unload_duh(dumb_module_);
		dumb_module_ = nullptr;
	}
}

// -----------------------------------------------------------------------------
// Called when seeking is requested on the sound stream
// -----------------------------------------------------------------------------
void ModMusic::onSeek(sf::Time timeOffset)
{
	long pos     = static_cast<long>(timeOffset.asSeconds() * 65536);
	dumb_player_ = duh_start_sigrenderer(dumb_module_, 0, 2, pos);
	// dumb_it_set_loop_callback(duh_get_it_sigrenderer(dumb_player), dumb_it_callback_terminate, NULL);
}

// -----------------------------------------------------------------------------
// Called when sound data is requested from the stream
// -----------------------------------------------------------------------------
bool ModMusic::onGetData(Chunk& data)
{
	duh_render(dumb_player_, 16, 0, 1.0f, (65536.0f / 44100.0f), 44100 / 2, samples_);
	data.samples     = samples_;
	data.sampleCount = 44100;
	return true;
}

// -----------------------------------------------------------------------------
// Initialises the DUMB library
// -----------------------------------------------------------------------------
void ModMusic::initDumb()
{
	dumb_register_stdfiles();
	init_done_ = true;
}
