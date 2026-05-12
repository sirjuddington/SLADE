
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Mp3Music.cpp
// Description: Mp3Music class, an SFML sound stream class to play mp3 music
//              using libmpg123
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
#include "Mp3Music.h"

using namespace slade;
using namespace audio;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace slade::audio
{
struct Mp3MemoryData
{
	void*  data;
	size_t size;
	off_t  offset;
};

mpg123_ssize_t memoryDataRead(void* raw_mp3_data, void* buffer, size_t nbyte)
{
	auto mp3_data = static_cast<Mp3MemoryData*>(raw_mp3_data);
	if (mp3_data->offset >= static_cast<ssize_t>(mp3_data->size))
	{
		memset(buffer, 0, nbyte);
		return (mpg123_ssize_t)0;
	}

	if (mp3_data->offset + static_cast<ssize_t>(nbyte) > static_cast<ssize_t>(mp3_data->size))
	{
		size_t read_size    = mp3_data->size - mp3_data->offset;
		size_t mem_set_size = mp3_data->offset + nbyte - static_cast<ssize_t>(mp3_data->size);
		memcpy(buffer, static_cast<unsigned char*>(mp3_data->data) + mp3_data->offset, read_size);
		memset(buffer, 0, mem_set_size);
		mp3_data->offset += read_size;
		return static_cast<mpg123_ssize_t>(read_size);
	}

	memcpy(buffer, static_cast<unsigned char*>(mp3_data->data) + mp3_data->offset, nbyte);
	mp3_data->offset += nbyte;
	return static_cast<mpg123_ssize_t>(nbyte);
}

off_t memoryDataLSeek(void* raw_mp3_data, off_t offset, int whence)
{
	auto mp3_data = static_cast<Mp3MemoryData*>(raw_mp3_data);
	switch (whence)
	{
	case SEEK_SET: mp3_data->offset = offset; break;
	case SEEK_CUR: mp3_data->offset += offset; break;
	case SEEK_END: mp3_data->offset = mp3_data->size + offset; break;
	}
	return mp3_data->offset;
}

void memoryDataCleanup(void* raw_mp3_data)
{
	delete static_cast<Mp3MemoryData*>(raw_mp3_data);
}

} // namespace slade::audio


// -----------------------------------------------------------------------------
//
// Mp3Music Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Mp3Music class constructor
// -----------------------------------------------------------------------------
Mp3Music::Mp3Music()
{
	int err = MPG123_OK;
	if ((err = mpg123_init()) != MPG123_OK)
	{
		log::error(mpg123_plain_strerror(err));
		return;
	}

	handle_ = mpg123_new(nullptr, &err);
	if (!handle_)
	{
		log::error("Unable to create mpg123 handle: {}", mpg123_plain_strerror(err));
		return;
	}

	mpg123_replace_reader_handle(handle_, &memoryDataRead, &memoryDataLSeek, &memoryDataCleanup);
}

// -----------------------------------------------------------------------------
// Mp3Music class destructor
// -----------------------------------------------------------------------------
Mp3Music::~Mp3Music()
{
	SoundStream::stop();

	if (buffer_)
	{
		delete[] buffer_;
		buffer_ = nullptr;
	}

	mpg123_close(handle_);
	mpg123_delete(handle_);
	mpg123_exit();
}

// -----------------------------------------------------------------------------
// Loads an mp3 file for playback
// -----------------------------------------------------------------------------
bool Mp3Music::openFromFile(const std::string& filename)
{
	stop();

	if (buffer_)
	{
		delete[] buffer_;
		buffer_ = nullptr;
	}

	if (handle_)
		mpg123_close(handle_);

	if (mpg123_open(handle_, filename.c_str()) != MPG123_OK)
	{
		log::error(mpg123_strerror(handle_));
		return false;
	}

	long rate     = 0;
	int  channels = 0, encoding = 0;
	if (mpg123_getformat(handle_, &rate, &channels, &encoding) != MPG123_OK)
	{
		log::error("Failed to get format information for \"{}\"", filename);
		return false;
	}
	sampling_rate_ = rate;

	buffer_size_ = mpg123_outblock(handle_);
	buffer_      = new unsigned char[buffer_size_];
	if (!buffer_)
	{
		log::error("Failed to reserve memory for decoding one frame for \"{}\"", filename);
		return false;
	}

#if (SFML_VERSION_MAJOR > 2)
	initialize(channels, rate, getChannelMap());
#else
	initialize(channels, rate);
#endif

	return true;
}

// -----------------------------------------------------------------------------
// Loads mp3 data for playback
// -----------------------------------------------------------------------------
bool Mp3Music::loadFromMemory(void* data, size_t size_in_bytes)
{
	stop();

	if (buffer_)
	{
		delete[] buffer_;
		buffer_ = nullptr;
	}

	if (handle_)
		mpg123_close(handle_);

	auto mp3_data = new Mp3MemoryData{ data, size_in_bytes, 0 };
	if (!mp3_data)
	{
		log::error("Failed to reserve memory for keeping track of Memory Object");
	}

	if (mpg123_open_handle(handle_, mp3_data) != MPG123_OK)
	{
		log::error(mpg123_strerror(handle_));
		delete mp3_data;
		mp3_data = nullptr;
		return false;
	}

	long rate     = 0;
	int  channels = 0, encoding = 0;
	if (mpg123_getformat(handle_, &rate, &channels, &encoding) != MPG123_OK)
	{
		log::error("Failed to get format information for Memory Object");
		return false;
	}
	sampling_rate_ = rate;

	buffer_size_ = mpg123_outblock(handle_);
	buffer_      = new unsigned char[buffer_size_];
	if (!buffer_)
	{
		log::error("Failed to reserve memory for decoding one frame for Memory Object");
		return false;
	}

	log::debug("rate {}, channels {}", rate, channels);

#if (SFML_VERSION_MAJOR > 2)
	initialize(channels, rate, getChannelMap());
#else
	initialize(channels, rate);
#endif

	return true;
}

// -----------------------------------------------------------------------------
// Returns the duration of the currently loaded mp3
// -----------------------------------------------------------------------------
sf::Time Mp3Music::duration() const
{
	if (!handle_ || sampling_rate_ == 0)
		return {};

	auto len  = mpg123_length(handle_);
	auto secs = static_cast<float>(len) / static_cast<float>(sampling_rate_);

	log::debug("len {} rate {} secs {}", len, sampling_rate_, secs);

	return sf::seconds(secs);
}

// -----------------------------------------------------------------------------
// Called when sound data is requested from the stream
// -----------------------------------------------------------------------------
bool Mp3Music::onGetData(Chunk& data)
{
#if (SFML_VERSION_MAJOR > 2)
	std::lock_guard lock(mutex_);
#else
	sf::Lock lock(mutex_);
#endif

	if (handle_)
	{
		size_t done;
		mpg123_read(handle_, buffer_, buffer_size_, &done);

		data.samples     = reinterpret_cast<short*>(buffer_);
		data.sampleCount = done / sizeof(short);

		return (data.sampleCount > 0);
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Called when seeking is requested on the sound stream
// -----------------------------------------------------------------------------
void Mp3Music::onSeek(sf::Time time_offset)
{
#if (SFML_VERSION_MAJOR > 2)
	std::lock_guard lock(mutex_);
#else
	sf::Lock lock(mutex_);
#endif


	// tschumacher: sampleoff must be (seconds * samplingRate) to make this working correctly
	if (handle_)
		mpg123_seek(handle_, static_cast<off_t>(time_offset.asSeconds() * sampling_rate_), SEEK_SET);
}
