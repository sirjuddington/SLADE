#pragma once

namespace slade
{
class MapSector;
class TextBox;

class SectorInfoOverlay
{
public:
	SectorInfoOverlay();
	~SectorInfoOverlay();

	void update(const MapSector* sector);
	void draw(int bottom, int right, float alpha = 1.0f);

private:
	unique_ptr<TextBox> text_box_;
	string              ftex_;
	string              ctex_;
	int                 last_size_ = 100;

	void drawTexture(float alpha, int x, int y, string_view texture, string_view pos = "Upper") const;
};
} // namespace slade
