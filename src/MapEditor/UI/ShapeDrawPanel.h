#pragma once

class wxBoxSizer;

class ShapeDrawPanel : public wxPanel
{
public:
	ShapeDrawPanel(wxWindow* parent);
	~ShapeDrawPanel() {}

	void showShapeOptions(int shape);

private:
	wxChoice*   choice_shape_ = nullptr;
	wxCheckBox* cb_centered_  = nullptr;
	wxCheckBox* cb_lockratio_ = nullptr;
	wxBoxSizer* sizer_main_   = nullptr;
	wxSpinCtrl* spin_sides_   = nullptr;
	wxPanel*    panel_sides_  = nullptr;
};
