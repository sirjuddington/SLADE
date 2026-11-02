#pragma once

namespace slade
{
namespace mapeditor
{
	class ObjectEditGroup;
}

class ObjectEditPanel : public wxPanel
{
public:
	ObjectEditPanel(wxWindow* parent);
	~ObjectEditPanel() override = default;

	void init(const mapeditor::ObjectEditGroup* group);
	void update(const mapeditor::ObjectEditGroup* group, bool lock_rotation = false) const;

private:
	wxTextCtrl* text_xoff_      = nullptr;
	wxTextCtrl* text_yoff_      = nullptr;
	wxTextCtrl* text_scalex_    = nullptr;
	wxTextCtrl* text_scaley_    = nullptr;
	wxComboBox* combo_rotation_ = nullptr;
	wxButton*   btn_preview_    = nullptr;
	wxButton*   btn_apply_      = nullptr;
	wxButton*   btn_cancel_     = nullptr;
	wxCheckBox* cb_mirror_x_    = nullptr;
	wxCheckBox* cb_mirror_y_    = nullptr;

	double old_x_      = 0;
	double old_y_      = 0;
	double old_width_  = 0;
	double old_height_ = 0;

	void setupLayout();
	void onBtnPreviewClicked(wxCommandEvent& e);
};
} // namespace slade
