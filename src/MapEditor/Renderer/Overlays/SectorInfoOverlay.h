
#ifndef __SECTOR_INFO_OVERLAY_H__
#define __SECTOR_INFO_OVERLAY_H__

#include "InfoOverlay.h"

class MapSector;
class TextureBox;
namespace GLUI { class TextBox; }

class SectorInfoOverlay : public InfoOverlay
{
private:
	GLUI::TextBox*	text_info;
	TextureBox*		tex_floor;
	TextureBox*		tex_ceiling;

public:
	SectorInfoOverlay();
	~SectorInfoOverlay();

	void	update(MapSector* sector);

	void	drawWidget(point2_t pos, float alpha) override;
	void	updateLayout(dim2_t fit) override;
};

#endif//__SECTOR_INFO_OVERLAY_H__
