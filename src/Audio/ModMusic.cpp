
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ModMusic.cpp
// Description: ModMusic class, an SFML sound stream class to play mod music
//              using XMP
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
#include "thirdparty/libxmp/xmp.h"

using namespace slade;
using namespace audio;


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
	// Close current module if any
	if (xmp_player_)
	    close();

    xmp_player_ = xmp_create_context();

	// Load module file
	if (xmp_load_module(xmp_player_, filename.c_str()) == 0)
	{
	    xmp_start_player(xmp_player_, 44100, 0);
		initialize(2, 44100);
		return true;
	}
	else
	{
		log::error(fmt::format("Failed to load module music file \"{}\"", filename.c_str()));
		xmp_free_context(xmp_player_);
		return false;
	}
}

// -----------------------------------------------------------------------------
// Loads mod data for playback
// -----------------------------------------------------------------------------
bool ModMusic::loadFromMemory(const uint8_t* data, const uint32_t size)
{
	// Close current module if any
	if (xmp_player_)
	    close();

    xmp_player_ = xmp_create_context();

	// Load module file
	if (xmp_load_module_from_memory(xmp_player_, data, size) == 0)
	{
	    xmp_start_player(xmp_player_, 44100, 0);
		initialize(2, 44100);
		return true;
	}
	else
	{
		log::error("Failed to load module music data");
		xmp_free_context(xmp_player_);
		return false;
	}
}

// -----------------------------------------------------------------------------
// Returns the duration of the currently loaded mod
// -----------------------------------------------------------------------------
sf::Time ModMusic::duration() const
{
    xmp_module_info *track_info = new xmp_module_info;
    xmp_get_module_info(xmp_player_, track_info);
    int duration = 0;
    for (int i=0; i < track_info->num_sequences; i++)
    {
        duration += track_info->seq_data[i].duration;
    }
    delete track_info;
	return sf::milliseconds(duration);
}

// -----------------------------------------------------------------------------
// Closes and unloads any currently open mod
// -----------------------------------------------------------------------------
void ModMusic::close()
{
	if (xmp_player_ != nullptr)
	{
		xmp_end_player(xmp_player_);
		xmp_release_module(xmp_player_);
		xmp_free_context(xmp_player_);
		xmp_player_ = nullptr;
	}
}

// -----------------------------------------------------------------------------
// Called when seeking is requested on the sound stream
// -----------------------------------------------------------------------------
void ModMusic::onSeek(sf::Time timeOffset)
{
	xmp_seek_time(xmp_player_, timeOffset.asMilliseconds());
}

// -----------------------------------------------------------------------------
// Called when sound data is requested from the stream
// -----------------------------------------------------------------------------
bool ModMusic::onGetData(Chunk& data)
{
	data.sampleCount = xmp_play_buffer(xmp_player_, samples_, 44100 * sizeof(sf::Int16), 0) / sizeof(sf::Int16);
	data.samples     = samples_;
	return true;
}
