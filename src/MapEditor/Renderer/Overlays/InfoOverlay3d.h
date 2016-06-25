
#ifndef __INFO_OVERLAY_3D_H__
#define __INFO_OVERLAY_3D_H__

#include "InfoOverlay.h"

class SLADEMap;
class MapObject;
class TextureBox;
namespace GLUI { class TextBox; }

class InfoOverlay3D : public InfoOverlay
{
private:
	TextureBox*		tex_box;
	GLUI::TextBox*	text_info1;
	GLUI::TextBox*	text_info2;
	MapObject*		object;
	long			last_update;

public:
	InfoOverlay3D();
	~InfoOverlay3D();

	void	update(int item_index, int item_type, SLADEMap* map);

	void	drawWidget(point2_t pos, float alpha) override;
	void	updateLayout(dim2_t fit) override;
};

#endif//__INFO_OVERLAY_3D_H__
