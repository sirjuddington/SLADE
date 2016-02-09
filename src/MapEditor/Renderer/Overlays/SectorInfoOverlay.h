
#ifndef __SECTOR_INFO_OVERLAY_H__
#define __SECTOR_INFO_OVERLAY_H__

#include "OpenGL/GLUI/Panel.h"

class MapSector;
class TextureBox;
namespace GLUI { class TextBox; }

class SectorInfoOverlay : public GLUI::Panel
{
private:
	GLUI::TextBox*	text_info;
	TextureBox*		tex_floor;
	TextureBox*		tex_ceiling;

public:
	SectorInfoOverlay();
	~SectorInfoOverlay();

	void	update(MapSector* sector);

	void	drawWidget(point2_t pos);
	void	updateLayout(dim2_t fit);
};

#if 0

class MapSector;
class STextBox;

class SectorInfoOverlay
{
private:
	STextBox*	text_box;
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

#endif

#endif//__SECTOR_INFO_OVERLAY_H__
