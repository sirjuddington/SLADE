
#ifndef __LINE_TEXTURE_OVERLAY_H__
#define __LINE_TEXTURE_OVERLAY_H__

#include "MCOverlay.h"

class MapLine;
class MapSide;
class LineTextureOverlay : public MCOverlay {
private:
	vector<MapLine*>	lines;
	int					selected_side;

	struct tex_inf_t {
		point2_t		position;
		bool			hover;
		vector<string>	textures;
		bool			changed;

		tex_inf_t() { hover = false; changed = false; }

		void checkHover(int x, int y, int halfsize) {
			if (x >= position.x - halfsize &&
				x <= position.x + halfsize &&
				y >= position.y - halfsize &&
				y <= position.y + halfsize)
				hover = true;
			else
				hover = false;
		}
	};

	// Texture info
	enum {
		FRONT_UPPER = 0,
		FRONT_MIDDLE,
		FRONT_LOWER,
		BACK_UPPER,
		BACK_MIDDLE,
		BACK_LOWER,
	};
	tex_inf_t	textures[6];
	bool		side1;
	bool		side2;

	// Drawing info
	int	tex_size;
	int	last_width;
	int	last_height;

	// Helper functions
	void	addTexture(tex_inf_t& inf, string texture);

public:
	LineTextureOverlay();
	~LineTextureOverlay();

	void	openLines(vector<MapLine*>& list);
	void	close(bool cancel);
	void	update(long frametime);

	// Drawing
	void	updateLayout(int width, int height);
	void	draw(int width, int height, float fade);
	void	drawTexture(float alpha, int size, tex_inf_t& tex, string position);

	// Input
	void	mouseMotion(int x, int y);
	void	mouseLeftClick();
	void	mouseRightClick();
	void	keyDown(string key);

	void	browseTexture(tex_inf_t& tex, string position);
};

#endif//__LINE_TEXTURE_OVERLAY_H__
