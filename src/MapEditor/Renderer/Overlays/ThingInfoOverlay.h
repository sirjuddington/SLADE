
#ifndef __THING_INFO_OVERLAY_H__
#define __THING_INFO_OVERLAY_H__

#include "OpenGL/GLUI/Panel.h"

class MapThing;
namespace GLUI { class TextBox; class ImageBox; }

class ThingInfoOverlay : public GLUI::Panel
{
private:
	GLUI::TextBox*	text_info;
	GLUI::ImageBox*	image_sprite;

public:
	ThingInfoOverlay();
	~ThingInfoOverlay();

	void	update(MapThing* thing);

	void	drawWidget(point2_t pos);
	void	updateLayout(dim2_t fit);
};


#if 0

class MapThing;
class GLTexture;
class STextBox;

class ThingInfoOverlay
{
private:
	string		sprite;
	string		translation;
	string		palette;
	string		icon;
	int			zeth;
	STextBox*	text_box;
	int			last_size;

public:
	ThingInfoOverlay();
	~ThingInfoOverlay();

	void	update(MapThing* thing);
	void	draw(int bottom, int right, float alpha = 1.0f);
};

#endif

#endif//__THING_INFO_OVERLAY__
