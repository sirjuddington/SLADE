#pragma once

#include "Graphics/CTexture/TextureXFormat.h"

namespace slade::texeditor
{
class CreateTextureXDialog : public wxDialog
{
public:
	CreateTextureXDialog(wxWindow* parent);
	~CreateTextureXDialog() override;

	TextureXFormat getSelectedFormat() const;

	bool createNewSelected() const { return rb_new_->GetValue(); }

	void onRadioNewSelected(wxCommandEvent& e);

private:
	wxRadioButton* rb_format_doom_;
	wxRadioButton* rb_format_strife_;
	wxRadioButton* rb_format_textures_;

	wxRadioButton* rb_new_;
	wxRadioButton* rb_import_bra_;
};
} // namespace slade::texeditor
