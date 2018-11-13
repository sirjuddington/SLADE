#pragma once

#include <SFML/Audio.hpp>

struct DUH;
struct DUH_SIGRENDERER;

class ModMusic : public sf::SoundStream
{
public:
	ModMusic();
	~ModMusic();

	bool     openFromFile(const std::string& filename);
	bool     loadFromMemory(const uint8_t* data, const uint32_t size);
	sf::Time getDuration() const;

	static void initDumb();

private:
	void close();
	bool onGetData(Chunk& data);
	void onSeek(sf::Time timeOffset);

	sf::Int16        samples_[44100];
	DUH*             dumb_module_;
	DUH_SIGRENDERER* dumb_player_;

	static bool init_done_;
};
