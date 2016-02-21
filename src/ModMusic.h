#ifndef __MOD_MUSIC_H__
#define __MOD_MUSIC_H__

#include <SFML/Audio.hpp>

struct DUH;
struct DUH_SIGRENDERER;

class ModMusic : public sf::SoundStream
{
private:
	void close();
	bool onGetData(Chunk& data);
	void onSeek(sf::Time timeOffset);

	sf::Int16        samples[44100];
	DUH*             dumb_module;
	DUH_SIGRENDERER* dumb_player;

	static bool init_done;

public:
	ModMusic();
	~ModMusic();

	bool openFromFile(const std::string& filename);
	bool loadFromMemory(const uint8_t* data, const uint32_t size);
	sf::Time getDuration() const;

	static void initDumb();
};


#endif//__MOD_MUSIC_H__
