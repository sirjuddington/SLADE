
#ifndef __SECTOR_INFO_OVERLAY_H__
#define __SECTOR_INFO_OVERLAY_H__

class MapSector;
class SectorInfoOverlay {
private:
	//string	info;
	//string	height;
	//string	light;
	//string	tag;
	vector<string>	info;
	string			ftex;
	string			ctex;

public:
	SectorInfoOverlay();
	~SectorInfoOverlay();

	void	update(MapSector* sector);
	void	draw(int bottom, int right, float alpha = 1.0f);
	void	drawTexture(float alpha, int x, int y, string texture, string pos = "Upper");
};

#endif//__SECTOR_INFO_OVERLAY_H__
