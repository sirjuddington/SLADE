#pragma once

#include <SFML/Audio.hpp>

struct DUH;
struct DUH_SIGRENDERER;

namespace slade::audio
{
class ModMusic : public sf::SoundStream
{
public:
	ModMusic() = default;
	~ModMusic() override;

	bool     openFromFile(const string& filename);
	bool     loadFromMemory(const uint8_t* data, const uint32_t size);
	sf::Time duration() const;

	static void initDumb();

private:
	void close();
	bool onGetData(Chunk& data) override;
	void onSeek(sf::Time timeOffset) override;

	int16_t          samples_[44100]{};
	DUH*             dumb_module_ = nullptr;
	DUH_SIGRENDERER* dumb_player_ = nullptr;

	static bool init_done_;
};
} // namespace slade::audio
