
#ifndef __SECTOR_INFO_OVERLAY_H__
#define __SECTOR_INFO_OVERLAY_H__

class MapSector;
class TextBox;

class SectorInfoOverlay
{
private:
	TextBox*	text_box;
	string		ftex;
	string		ctex;
	int			last_size;

public:
	SectorInfoOverlay();
	~SectorInfoOverlay();

	void	update(MapSector* sector);
	void	draw(int bottom, int right, float alpha = 1.0f);
	void	drawTexture(float alpha, int x, int y, string texture, string pos = "Upper");
};

#endif//__SECTOR_INFO_OVERLAY_H__
