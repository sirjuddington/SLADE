#pragma once

#include "MCOverlay.h"

class MapSector;
class SectorTextureOverlay : public MCOverlay
{
public:
	SectorTextureOverlay()  = default;
	~SectorTextureOverlay() = default;

	void openSectors(vector<MapSector*>& list);
	void close(bool cancel) override;
	void update(long frametime) override;

	// Drawing
	void draw(int width, int height, float fade) override;
	void drawTexture(float alpha, int x, int y, int size, vector<std::string>& textures, bool hover) const;

	// Input
	void mouseMotion(int x, int y) override;
	void mouseLeftClick() override;
	void keyDown(std::string_view key) override;

	void browseFloorTexture();
	void browseCeilingTexture();

private:
	vector<MapSector*>  sectors_;
	bool                hover_ceil_  = false;
	bool                hover_floor_ = false;
	vector<std::string> tex_floor_;
	vector<std::string> tex_ceil_;
	float               anim_floor_ = 0.f;
	float               anim_ceil_  = 0.f;

	// Drawing info
	int middlex_  = 0;
	int middley_  = 0;
	int tex_size_ = 0;
	int border_   = 0;
};
