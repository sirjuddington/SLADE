#pragma once

#include "UI/Canvas/GLCanvas.h"

class MapSide;
class NumberTextCtrl;

class SideTexCanvas : public GLCanvas
{
public:
	SideTexCanvas(wxWindow* parent);
	~SideTexCanvas() = default;

	wxString texName() const { return texname_; }
	void     setTexture(const wxString& texture);
	void     drawContent() override;

private:
	unsigned texture_ = 0;
	wxString texname_;
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
