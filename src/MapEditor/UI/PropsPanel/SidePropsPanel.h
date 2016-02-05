
#ifndef __SIDE_PROPS_PANEL_H__
#define __SIDE_PROPS_PANEL_H__

#include "UI/Canvas/OGLCanvas.h"

class GLTexture;
class SideTexCanvas : public OGLCanvas
{
private:
	GLTexture*	texture;
	string		texname;

public:
	SideTexCanvas(wxWindow* parent);
	~SideTexCanvas();

	string	getTexName();
	void	setTexture(string texture);
	void	draw();
};

class TextureComboBox : public wxComboBox
{
private:
	bool list_down;

public:
	TextureComboBox(wxWindow* parent);
	~TextureComboBox() {}

	void onDropDown(wxCommandEvent& e);
	void onCloseUp(wxCommandEvent& e);
	void onKeyDown(wxKeyEvent& e);
};

class MapSide;
class NumberTextCtrl;
class SidePropsPanel : public wxPanel
{
private:
	SideTexCanvas*		gfx_lower;
	SideTexCanvas*		gfx_middle;
	SideTexCanvas*		gfx_upper;
	TextureComboBox*	tcb_lower;
	TextureComboBox*	tcb_middle;
	TextureComboBox*	tcb_upper;
	NumberTextCtrl*		text_offsetx;
	NumberTextCtrl*		text_offsety;

public:
	SidePropsPanel(wxWindow* parent);
	~SidePropsPanel();

	void	openSides(vector<MapSide*>& sides);
	void	applyTo(vector<MapSide*>& sides);

	// Events
	void	onTextureChanged(wxCommandEvent& e);
	void	onTextureClicked(wxMouseEvent& e);
};

#endif//__SIDE_PROPS_PANEL_H__
