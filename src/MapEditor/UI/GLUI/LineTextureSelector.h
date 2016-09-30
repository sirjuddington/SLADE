#pragma once

#include "OpenGL/GLUI/Panel.h"

class TextureBox;
class MapLine;

class LTSPanel : public GLUI::Panel
{
public:
	LTSPanel(GLUI::Widget* parent);
	~LTSPanel();

	void	setLine(MapLine* line);
	void	updateLayout(dim2_t fit) override;

private:
	TextureBox*	tex_front_upper;
	TextureBox*	tex_front_middle;
	TextureBox*	tex_front_lower;
	TextureBox*	tex_back_upper;
	TextureBox*	tex_back_middle;
	TextureBox*	tex_back_lower;
};

class LineTextureSelector : public GLUI::Panel
{
public:
	LineTextureSelector();
	~LineTextureSelector();

	void	setLine(MapLine* line) { panel_textures->setLine(line); }
	void	updateLayout(dim2_t fit) override;

private:
	LTSPanel*			panel_textures;
	GLUI::FadeAnimator*	anim_visible;

	void onKeyDown(GLUI::KeyEventInfo& e);
};
