#pragma once

#include "UI/SDialog.h"

namespace slade
{
class PatchBrowser;
}
namespace slade::texeditor
{
class NewTextureDialog : public SDialog
{
public:
	NewTextureDialog(wxWindow* parent, PatchBrowser* patch_browser);

	string texName() const { return text_name_->GetValue().utf8_string(); }
	bool   blankSelected() const { return rb_blank_->GetValue(); }
	int    texWidth() const { return spin_width_->GetValue(); }
	int    texHeight() const { return spin_height_->GetValue(); }
	string patch() const { return text_patch_->GetValue().utf8_string(); }

private:
	PatchBrowser*  patch_browser_    = nullptr;
	wxTextCtrl*    text_name_        = nullptr;
	wxRadioButton* rb_blank_         = nullptr;
	wxRadioButton* rb_patch_         = nullptr;
	wxSpinCtrl*    spin_width_       = nullptr;
	wxSpinCtrl*    spin_height_      = nullptr;
	wxTextCtrl*    text_patch_       = nullptr;
	wxButton*      btn_browse_patch_ = nullptr;

	bool checkValues() const;
	void browsePatch() const;
};
} // namespace slade::texeditor
