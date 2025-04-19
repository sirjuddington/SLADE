#pragma once

#include "UI/Canvas/OGLCanvas.h"

namespace slade
{
class MapSide;
class NumberTextCtrl;

class SideTexCanvas : public OGLCanvas
{
public:
	SideTexCanvas(wxWindow* parent);
	~SideTexCanvas() = default;

	string texName() const { return texname_; }
	void   setTexture(const string& texture);
	void   draw() override;

private:
	unsigned texture_ = 0;
	string   texname_;
};

class TextureComboBox : public wxComboBox
{
public:
	TextureComboBox(wxWindow* parent);
	~TextureComboBox() = default;

private:
	bool list_down_ = false;

	void onDropDown(wxCommandEvent& e);
	void onCloseUp(wxCommandEvent& e);
	void onKeyDown(wxKeyEvent& e);
};

class SidePropsPanel : public wxPanel
{
public:
	SidePropsPanel(wxWindow* parent);
	~SidePropsPanel() = default;

	void openSides(vector<MapSide*>& sides) const;
	void applyTo(vector<MapSide*>& sides) const;

private:
	SideTexCanvas*   gfx_lower_    = nullptr;
	SideTexCanvas*   gfx_middle_   = nullptr;
	SideTexCanvas*   gfx_upper_    = nullptr;
	TextureComboBox* tcb_lower_    = nullptr;
	TextureComboBox* tcb_middle_   = nullptr;
	TextureComboBox* tcb_upper_    = nullptr;
	NumberTextCtrl*  text_offsetx_ = nullptr;
	NumberTextCtrl*  text_offsety_ = nullptr;

	// Events
	void onTextureChanged(wxCommandEvent& e);
	void onTextureClicked(wxMouseEvent& e);
};
} // namespace slade
