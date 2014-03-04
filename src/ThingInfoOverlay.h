
#ifndef __THING_INFO_OVERLAY_H__
#define __THING_INFO_OVERLAY_H__

class MapThing;
class GLTexture;
class TextBox;

class ThingInfoOverlay
{
private:
	string		sprite;
	string		translation;
	string		palette;
	string		icon;
	int			zeth;
	TextBox*	text_box;
	int			last_size;

public:
	ThingInfoOverlay();
	~ThingInfoOverlay();

	void	update(MapThing* thing);
	void	draw(int bottom, int right, float alpha = 1.0f);
};

#endif//__THING_INFO_OVERLAY__
