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
	wxString                 ftex_;
	wxString                 ctex_;
	int                      last_size_ = 100;

	void drawTexture(float alpha, int x, int y, wxString texture, const wxString& pos = "Upper") const;
};
