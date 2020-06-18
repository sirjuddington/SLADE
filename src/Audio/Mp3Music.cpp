
#include "Main.h"
#include "Mp3Music.h"
#include <iostream>

using namespace slade;
using namespace audio;

namespace slade::audio
{
struct Mp3MemoryData
{
	void*  data;
	size_t size;
	off_t  offset;
};

ssize_t memoryDataRead(void* raw_mp3_data, void* buffer, size_t nbyte)
{
	auto mp3_data = static_cast<Mp3MemoryData*>(raw_mp3_data);
	if (mp3_data->offset >= (ssize_t)mp3_data->size)
	{
		memset(buffer, 0, nbyte);
		return (ssize_t)0;
	}
	else if (mp3_data->offset + (ssize_t)nbyte > (ssize_t)mp3_data->size)
	{
		size_t read_size    = mp3_data->size - mp3_data->offset;
		size_t mem_set_size = mp3_data->offset + nbyte - (ssize_t)mp3_data->size;
		memcpy(buffer, (unsigned char*)mp3_data->data + mp3_data->offset, read_size);
		memset(buffer, 0, mem_set_size);
		mp3_data->offset += read_size;
		return (ssize_t)read_size;
	}
	else
	{
		memcpy(buffer, (unsigned char*)mp3_data->data + mp3_data->offset, nbyte);
		mp3_data->offset += nbyte;
		return (ssize_t)nbyte;
	}
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
	delete raw_mp3_data;
}

} // namespace slade::audio


Mp3Music::Mp3Music()
{
	int err = MPG123_OK;
	if ((err = mpg123_init()) != MPG123_OK)
	{
		std::cerr << mpg123_plain_strerror(err) << std::endl;
		return;
	}

	handle_ = mpg123_new(nullptr, &err);
	if (!handle_)
	{
		std::cerr << "Unable to create mpg123 handle: " << mpg123_plain_strerror(err) << std::endl;
		return;
	}

	mpg123_replace_reader_handle(handle_, &memoryDataRead, &memoryDataLSeek, &memoryDataCleanup);
}

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
		std::cerr << mpg123_strerror(handle_) << std::endl;
		return false;
	}

	long rate     = 0;
	int  channels = 0, encoding = 0;
	if (mpg123_getformat(handle_, &rate, &channels, &encoding) != MPG123_OK)
	{
		std::cerr << "Failed to get format information for \"" << filename << "\"" << std::endl;
		return false;
	}
	sampling_rate_ = rate;

	buffer_size_ = mpg123_outblock(handle_);
	buffer_      = new unsigned char[buffer_size_];
	if (!buffer_)
	{
		std::cerr << "Failed to reserve memory for decoding one frame for \"" << filename << "\"" << std::endl;
		return false;
	}

	initialize(channels, rate);

	return true;
}

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

	log::info("rate {}, channels {}", rate, channels);

	initialize(channels, rate);

	return true;
}

sf::Time Mp3Music::duration() const
{
	if (!handle_ || sampling_rate_ == 0)
		return {};

	auto len  = mpg123_length(handle_);
	auto secs = (float)len / (float)sampling_rate_;

	log::info("len {} rate {} secs {}", len, sampling_rate_, secs);

	return sf::seconds(secs);
}

bool Mp3Music::onGetData(Chunk& data)
{
	sf::Lock lock(mutex_);

	if (handle_)
	{
		size_t done;
		mpg123_read(handle_, buffer_, buffer_size_, &done);

		data.samples     = (short*)buffer_;
		data.sampleCount = done / sizeof(short);

		return (data.sampleCount > 0);
	}
	else
		return false;
}

void Mp3Music::onSeek(sf::Time time_offset)
{
	sf::Lock lock(mutex_);

	// tschumacher: sampleoff must be (seconds * samplingRate) to make this working correctly
	if (handle_)
		mpg123_seek(handle_, static_cast<off_t>(time_offset.asSeconds() * sampling_rate_), SEEK_SET);
}
