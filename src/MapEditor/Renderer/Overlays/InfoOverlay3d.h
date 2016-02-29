
#ifndef __INFO_OVERLAY_3D_H__
#define __INFO_OVERLAY_3D_H__

#include "OpenGL/GLUI/Panel.h"

class SLADEMap;
class MapObject;
class TextureBox;
namespace GLUI { class TextBox; }

class InfoOverlay3D : public GLUI::Panel
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


#if 0

class SLADEMap;
class GLTexture;
class MapObject;
class InfoOverlay3D
{
private:
	vector<string>	info;
	vector<string>	info2;
	int				current_type;
	string			texname;
	GLTexture*		texture;
	bool			thing_icon;
	MapObject*		object;
	long			last_update;

public:
	InfoOverlay3D();
	~InfoOverlay3D();

	void	update(int item_index, int item_type, SLADEMap* map);
	void	draw(int bottom, int right, int middle, float alpha = 1.0f);
	void	drawTexture(float alpha, int x, int y);
};

#endif

#endif//__INFO_OVERLAY_3D_H__
