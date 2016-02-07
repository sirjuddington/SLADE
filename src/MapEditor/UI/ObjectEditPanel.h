
#ifndef __OBJECT_EDIT_PANEL_H__
#define __OBJECT_EDIT_PANEL_H__

#include <wx/panel.h>
#include "UI/WxBasicControls.h"

class ObjectEditGroup;
class ObjectEditPanel : public wxPanel
{
private:
	wxTextCtrl*	text_xoff;
	wxTextCtrl*	text_yoff;
	wxTextCtrl*	text_scalex;
	wxTextCtrl*	text_scaley;
	wxComboBox*	combo_rotation;
	wxButton*	btn_preview;
	wxButton*	btn_apply;
	wxButton*	btn_cancel;
	wxCheckBox*	cb_mirror_x;
	wxCheckBox*	cb_mirror_y;

	double	old_x;
	double	old_y;
	double	old_width;
	double	old_height;

public:
	ObjectEditPanel(wxWindow* parent);
	~ObjectEditPanel();

	void	init(ObjectEditGroup* group);
	void	update(ObjectEditGroup* group, bool lock_rotation = false);

	void	onBtnPreviewClicked(wxCommandEvent& e);
	void	onBtnApplyClicked(wxCommandEvent& e);
	void	onBtnCancelClicked(wxCommandEvent& e);
};

#endif//__OBJECT_EDIT_PANEL_H__
