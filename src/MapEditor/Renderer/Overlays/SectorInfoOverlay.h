#pragma once

class MapSector;
class TextBox;

class SectorInfoOverlay
{
public:
	SectorInfoOverlay();
	~SectorInfoOverlay();

	void update(MapSector* sector);
	void draw(int bottom, int right, float alpha = 1.0f);

private:
	TextBox* text_box_;
	string   ftex_;
	string   ctex_;
	int      last_size_;

	void drawTexture(float alpha, int x, int y, string texture, string pos = "Upper");
};
