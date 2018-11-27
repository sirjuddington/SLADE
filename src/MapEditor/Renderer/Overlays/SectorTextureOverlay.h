#pragma once

#include "MCOverlay.h"

class MapSector;
class SectorTextureOverlay : public MCOverlay
{
public:
	SectorTextureOverlay();
	~SectorTextureOverlay();

	void openSectors(vector<MapSector*>& list);
	void close(bool cancel);
	void update(long frametime);

	// Drawing
	void draw(int width, int height, float fade);
	void drawTexture(float alpha, int x, int y, int size, vector<string>& textures, bool hover);

	// Input
	void mouseMotion(int x, int y);
	void mouseLeftClick();
	void mouseRightClick();
	void keyDown(string key);

	void browseFloorTexture();
	void browseCeilingTexture();

private:
	vector<MapSector*> sectors_;
	bool               hover_ceil_;
	bool               hover_floor_;
	vector<string>     tex_floor_;
	vector<string>     tex_ceil_;
	float              anim_floor_;
	float              anim_ceil_;

	// Drawing info
	int middlex_;
	int middley_;
	int tex_size_;
	int border_;
};
