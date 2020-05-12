#pragma once

#include "WizardPageBase.h"

namespace slade
{
class TempFolderWizardPage : public WizardPageBase
{
public:
	TempFolderWizardPage(wxWindow* parent);
	~TempFolderWizardPage() = default;

	bool     canGoNext() override;
	void     applyChanges() override;
	wxString title() override { return "SLADE Temp Folder"; }
	wxString description() override;

private:
	wxRadioButton* rb_use_system_     = nullptr;
	wxRadioButton* rb_use_slade_dir_  = nullptr;
	wxRadioButton* rb_use_custom_dir_ = nullptr;
	wxTextCtrl*    text_custom_dir_   = nullptr;
	wxButton*      btn_browse_dir_    = nullptr;

	// Events
	void onRadioButtonChanged(wxCommandEvent& e);
	void onBtnBrowse(wxCommandEvent& e);
};
} // namespace slade
