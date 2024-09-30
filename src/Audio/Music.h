#pragma once

namespace slade::audio
{
class Music : public sf::Music
{
public:
	Music() = default;

	void allowSeek(bool allow) { allow_seek_ = allow; }

protected:
	void onSeek(sf::Time timeOffset) override
	{
		if (allow_seek_)
			sf::Music::onSeek(timeOffset);
	}

private:
	bool allow_seek_ = true;
};
} // namespace slade::audio
