
#ifndef __OBJECT_EDIT_PANEL_H__
#define __OBJECT_EDIT_PANEL_H__

class ObjectEditGroup;
class ObjectEditPanel : public wxPanel
{
private:
	wxTextCtrl*	text_xoff;
	wxTextCtrl*	text_yoff;
	wxTextCtrl*	text_scalex;
	wxTextCtrl*	text_scaley;
	wxComboBox*	combo_rotation;
	wxButton*	btn_apply;

	double	old_x;
	double	old_y;
	double	old_width;
	double	old_height;

public:
	ObjectEditPanel(wxWindow* parent);
	~ObjectEditPanel();

	void	init(ObjectEditGroup* group);
	void	update(ObjectEditGroup* group, bool lock_rotation = false);

	void	onBtnApplyClicked(wxCommandEvent& e);
};

#endif//__OBJECT_EDIT_PANEL_H__
