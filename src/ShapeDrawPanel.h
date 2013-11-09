
#ifndef __SHAPE_DRAW_PANEL_H__
#define __SHAPE_DRAW_PANEL_H__

#include <wx/spinctrl.h>

class ShapeDrawPanel : public wxPanel
{
private:
	wxChoice*	choice_shape;
	wxCheckBox*	cb_centered;
	wxCheckBox*	cb_lockratio;
	wxBoxSizer*	sizer_main;

	wxSpinCtrl*		spin_sides;
	wxPanel*		panel_sides;

public:
	ShapeDrawPanel(wxWindow* parent);
	~ShapeDrawPanel();

	void	showShapeOptions(int shape);

	// Events
	void	onShapeChanged(wxCommandEvent& e);
	void	onCenteredChecked(wxCommandEvent& e);
	void	onLockRatioChecked(wxCommandEvent& e);
	void	onSidesChanged(wxSpinEvent& e);
};

#endif//__SHAPE_DRAW_PANEL_H__
