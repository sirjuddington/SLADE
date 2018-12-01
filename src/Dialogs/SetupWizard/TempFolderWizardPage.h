#pragma once

#include "WizardPageBase.h"

class TempFolderWizardPage : public WizardPageBase
{
public:
	TempFolderWizardPage(wxWindow* parent);
	~TempFolderWizardPage();

	bool   canGoNext();
	void   applyChanges();
	string title() { return "SLADE Temp Folder"; }
	string description();

private:
	wxRadioButton* rb_use_system_;
	wxRadioButton* rb_use_slade_dir_;
	wxRadioButton* rb_use_custom_dir_;
	wxTextCtrl*    text_custom_dir_;
	wxButton*      btn_browse_dir_;

	// Events
	void onRadioButtonChanged(wxCommandEvent& e);
	void onBtnBrowse(wxCommandEvent& e);
};
