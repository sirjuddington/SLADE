
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ModMusic.cpp
// Description: ModMusic class, an SFML sound stream class to play mod music
//              using libxmp
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
#include <xmp.h>

using namespace slade;
using namespace audio;


// -----------------------------------------------------------------------------
//
// ModMusic Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ModMusic class constructor
// -----------------------------------------------------------------------------
ModMusic::ModMusic()
{
	xmp_context_ = xmp_create_context();
}

// -----------------------------------------------------------------------------
// ModMusic class destructor
// -----------------------------------------------------------------------------
ModMusic::~ModMusic()
{
	close();

	if (xmp_context_)
	{
		xmp_free_context(xmp_context_);
		xmp_context_ = nullptr;
	}
}

// -----------------------------------------------------------------------------
// Loads a mod file for playback
// -----------------------------------------------------------------------------
bool ModMusic::openFromFile(const string& filename)
{
	// Close current module if any
	close();

	if (!xmp_context_)
	{
		log::error("XMP context not initialized");
		return false;
	}

	// Load module file
	if (xmp_load_module(xmp_context_, filename.c_str()) != 0)
	{
		log::error("Failed to load module music file \"{}\"", filename);
		return false;
	}

	loaded_ = true;

	// Start player
	if (xmp_start_player(xmp_context_, sample_rate_, 0) != 0)
	{
		log::error("Failed to start XMP player for \"{}\"", filename);
		xmp_release_module(xmp_context_);
		loaded_ = false;
		return false;
	}

#if (SFML_VERSION_MAJOR > 2)
	initialize(2, sample_rate_, getChannelMap());
#else
	initialize(2, sample_rate_);
#endif

	return true;
}

// -----------------------------------------------------------------------------
// Loads mod data for playback
// -----------------------------------------------------------------------------
bool ModMusic::loadFromMemory(const u8* data, const u32 size)
{
	// Close current module if any
	close();

	if (!xmp_context_)
	{
		log::error("XMP context not initialized");
		return false;
	}

	// Load module from memory
	if (xmp_load_module_from_memory(xmp_context_, data, size) != 0)
	{
		log::error("Failed to load module music data");
		return false;
	}

	loaded_ = true;

	// Start player
	if (xmp_start_player(xmp_context_, sample_rate_, 0) != 0)
	{
		log::error("Failed to start XMP player");
		xmp_release_module(xmp_context_);
		loaded_ = false;
		return false;
	}

#if (SFML_VERSION_MAJOR > 2)
	initialize(2, sample_rate_, getChannelMap());
#else
	initialize(2, sample_rate_);
#endif

	return true;
}

// -----------------------------------------------------------------------------
// Returns the duration of the currently loaded mod
// -----------------------------------------------------------------------------
sf::Time ModMusic::duration() const
{
	if (!xmp_context_ || !loaded_)
		return sf::Time::Zero;

	xmp_frame_info fi;
	xmp_get_frame_info(xmp_context_, &fi);

	return sf::milliseconds(fi.total_time);
}

// -----------------------------------------------------------------------------
// Closes and unloads any currently open mod
// -----------------------------------------------------------------------------
void ModMusic::close()
{
	if (xmp_context_ && loaded_)
	{
		xmp_end_player(xmp_context_);
		xmp_release_module(xmp_context_);
		loaded_ = false;
	}
}

// -----------------------------------------------------------------------------
// Called when seeking is requested on the sound stream
// -----------------------------------------------------------------------------
void ModMusic::onSeek(sf::Time timeOffset)
{
	if (!xmp_context_ || !loaded_)
		return;

	// Seek to the specified position (time in milliseconds)
	xmp_seek_time(xmp_context_, timeOffset.asMilliseconds());
}

// -----------------------------------------------------------------------------
// Called when sound data is requested from the stream
// -----------------------------------------------------------------------------
bool ModMusic::onGetData(Chunk& data)
{
	if (!xmp_context_ || !loaded_)
		return false;

	// Play frame and get audio data
	if (xmp_play_buffer(xmp_context_, samples_, sizeof(samples_), 0) != 0)
		return false; // End of module or error

	data.samples     = samples_;
	data.sampleCount = sizeof(samples_) / sizeof(i16);
	
	return true;
}
