
#ifndef __SECTOR_TEXTURE_OVERLAY_H__
#define __SECTOR_TEXTURE_OVERLAY_H__

#include "MCOverlay.h"

class MapSector;
class SectorTextureOverlay : public MCOverlay
{
private:
	vector<MapSector*>	sectors;
	bool				hover_ceil;
	bool				hover_floor;
	vector<string>		tex_floor;
	vector<string>		tex_ceil;
	float				anim_floor;
	float				anim_ceil;

	// Drawing info
	int	middlex;
	int	middley;
	int	tex_size;
	int	border;

public:
	SectorTextureOverlay();
	~SectorTextureOverlay();

	void	openSectors(vector<MapSector*>& list);
	void	close(bool cancel);
	void	update(long frametime);

	// Drawing
	void	draw(int width, int height, float fade);
	void	drawTexture(float alpha, int x, int y, int size, vector<string>& textures, bool hover);

	// Input
	void	mouseMotion(int x, int y);
	void	mouseLeftClick();
	void	mouseRightClick();
	void	keyDown(string key);

	void	browseFloorTexture();
	void	browseCeilingTexture();
};

#endif//__SECTOR_TEXTURE_OVERLAY_H__
