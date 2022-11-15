#pragma once

#include <SFML/Audio.hpp>

namespace slade::audio
{
class ModMusic : public sf::SoundStream
{
public:
	ModMusic() = default;
	~ModMusic();

	bool     openFromFile(const string& filename);
	bool     loadFromMemory(const uint8_t* data, const uint32_t size);
	sf::Time duration() const;

	static void initXMP();

private:
	void close();
	bool onGetData(Chunk& data) override;
	void onSeek(sf::Time timeOffset) override;

	sf::Int16        samples_[44100]{};
	char*            xmp_player_ = nullptr;
};
} // namespace slade::audio
