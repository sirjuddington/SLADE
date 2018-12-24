#pragma once

class MapSector;
class TextBox;

class SectorInfoOverlay
{
public:
	SectorInfoOverlay();
	~SectorInfoOverlay() = default;

	void update(MapSector* sector);
	void draw(int bottom, int right, float alpha = 1.0f);

private:
	std::unique_ptr<TextBox> text_box_;
	string                   ftex_;
	string                   ctex_;
	int                      last_size_ = 100;

	void drawTexture(float alpha, int x, int y, string texture, const string& pos = "Upper") const;
};
