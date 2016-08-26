#pragma once

#include "Panel.h"

class GLTexture;
namespace GLUI { class ImageBox; class TextBox; }

class TextureBox : public GLUI::Panel
{
protected:
	GLUI::ImageBox*	image_texture;
	GLUI::TextBox*	text_name;
	int				box_size;
	string			sprite_translation;
	string			sprite_palette;
	bool			show_always;

public:
	TextureBox(Widget* parent);

	int		getBoxSize() { return box_size; }
	void	setBoxSize(int size) { box_size = size; }

	void	setTexture(int type, string texname, string prefix = "", bool required = false);
	void	setTexture(GLTexture* texture, string texname);

	// Widget
	void	updateLayout(dim2_t fit = dim2_t(-1, -1));

	enum
	{
		TEXTURE,
		FLAT,
		SPRITE
	};
};
