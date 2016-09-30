
#ifndef __THING_INFO_OVERLAY_H__
#define __THING_INFO_OVERLAY_H__

#include "InfoOverlay.h"

class MapThing;
namespace GLUI { class TextBox; class ImageBox; }

class ThingInfoOverlay : public InfoOverlay
{
private:
	GLUI::TextBox*	text_info;
	GLUI::ImageBox*	image_sprite;

public:
	ThingInfoOverlay();
	~ThingInfoOverlay();

	void	update(MapThing* thing);

	void	drawWidget(fpoint2_t pos, float alpha, fpoint2_t scale) override;
	void	updateLayout(dim2_t fit) override;
};

#endif//__THING_INFO_OVERLAY__
