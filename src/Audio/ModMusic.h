#pragma once

#include <SFML/Audio.hpp>
#include <xmp.h>

namespace slade::audio
{
class ModMusic : public sf::SoundStream
{
public:
	ModMusic();
	~ModMusic() override;

	bool     openFromFile(const string& filename);
	bool     loadFromMemory(const u8* data, u32 size);
	sf::Time duration() const;

private:
	void close();
	bool onGetData(Chunk& data) override;
	void onSeek(sf::Time timeOffset) override;

	i16          samples_[8192]{};
	xmp_context  xmp_context_ = nullptr;
	int          sample_rate_ = 44100;
	bool         loaded_      = false;
};
} // namespace slade::audio
