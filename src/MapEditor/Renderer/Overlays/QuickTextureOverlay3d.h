
#ifndef __QUICK_TEXTURE_OVERLAY_3D_H__
#define __QUICK_TEXTURE_OVERLAY_3D_H__

#include "MCOverlay.h"
#include "MapEditor/Edit/Edit3D.h"

class ItemSelection;
class GLTexture;
class MapEditContext;

class QuickTextureOverlay3d : public MCOverlay
{
private:
	struct qt_tex_t
	{
		GLTexture*	texture;
		string		name;
		qt_tex_t(string name) { texture = NULL; this->name = name; }
	};
	vector<qt_tex_t>	textures;
	unsigned			current_index;
	string				search;
	double				anim_offset;
	MapEditContext*		editor;
	int					sel_type;	// 0=flats, 1=walls, 2=both

public:
	QuickTextureOverlay3d(MapEditContext* editor);
	~QuickTextureOverlay3d();

	void	setTexture(string name);
	void	applyTexture();

	void	update(long frametime);

	void	draw(int width, int height, float fade = 1.0f);
	void	drawTexture(unsigned index, double x, double bottom, double size, float fade);
	double	determineSize(double x, int width);

	void	close(bool cancel = false);
	void	mouseMotion(int x, int y);
	void	mouseLeftClick();
	void	mouseRightClick();

	void	doSearch();
	void	keyDown(string key);

	static bool	ok(const ItemSelection& sel);
};

#endif//__QUICK_TEXTURE_OVERLAY_3D_H__
