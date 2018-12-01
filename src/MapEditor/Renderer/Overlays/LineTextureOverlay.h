#pragma once

#include "MCOverlay.h"

class MapLine;
class MapSide;

class LineTextureOverlay : public MCOverlay
{
public:
	LineTextureOverlay();
	~LineTextureOverlay();

	void openLines(vector<MapLine*>& list);
	void close(bool cancel);
	void update(long frametime);

	// Drawing
	void updateLayout(int width, int height);
	void draw(int width, int height, float fade);

	// Input
	void mouseMotion(int x, int y);
	void mouseLeftClick();
	void mouseRightClick();
	void keyDown(string key);

private:
	vector<MapLine*> lines;
	int              selected_side;

	struct TexInfo
	{
		Vec2i       position;
		bool           hover;
		vector<string> textures;
		bool           changed;

		TexInfo()
		{
			hover   = false;
			changed = false;
		}

		void checkHover(int x, int y, int halfsize)
		{
			if (x >= position.x - halfsize && x <= position.x + halfsize && y >= position.y - halfsize
				&& y <= position.y + halfsize)
				hover = true;
			else
				hover = false;
		}
	};

	// Texture info
	enum
	{
		FrontUpper = 0,
		FrontMiddle,
		FrontLower,
		BackUpper,
		BackMiddle,
		BackLower,
	};
	TexInfo textures_[6];
	bool    side1_;
	bool    side2_;

	// Drawing info
	int tex_size_;
	int last_width_;
	int last_height_;

	// Helper functions
	void addTexture(TexInfo& inf, string texture);
	void drawTexture(float alpha, int size, TexInfo& tex, string position);
	void browseTexture(TexInfo& tex, string position);
};
