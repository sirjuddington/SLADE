#pragma once

#include "MCOverlay.h"
#include "MapEditor/Edit/Edit3D.h"

class ItemSelection;
class GLTexture;
class MapEditContext;

class QuickTextureOverlay3d : public MCOverlay
{
public:
	QuickTextureOverlay3d(MapEditContext* editor);
	~QuickTextureOverlay3d();

	void setTexture(string name);
	void applyTexture();

	void update(long frametime);

	void   draw(int width, int height, float fade = 1.0f);
	void   drawTexture(unsigned index, double x, double bottom, double size, float fade);
	double determineSize(double x, int width);

	void close(bool cancel = false);
	void mouseMotion(int x, int y);
	void mouseLeftClick();
	void mouseRightClick();

	void doSearch();
	void keyDown(string key);

	static bool ok(const ItemSelection& sel);

private:
	struct QTTex
	{
		GLTexture* texture;
		string     name;
		QTTex(string name)
		{
			texture    = nullptr;
			this->name = name;
		}
	};
	vector<QTTex>   textures_;
	unsigned        current_index_;
	string          search_;
	double          anim_offset_;
	MapEditContext* editor_;
	int             sel_type_; // 0=flats, 1=walls, 2=both
};
