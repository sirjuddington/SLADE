
#ifndef __TEMP_FOLDER_WIZARD_PAGE_H__
#define __TEMP_FOLDER_WIZARD_PAGE_H__

#include "WizardPageBase.h"

class TempFolderWizardPage : public WizardPageBase
{
private:
	wxRadioButton*	rb_use_system;
	wxRadioButton*	rb_use_slade_dir;
	wxRadioButton*	rb_use_custom_dir;
	wxTextCtrl*		text_custom_dir;
	wxButton*		btn_browse_dir;

public:
	TempFolderWizardPage(wxWindow* parent);
	~TempFolderWizardPage();

	bool	canGoNext();
	void	applyChanges();
	string	getTitle() { return "SLADE Temp Folder"; }
	string	getDescription();

	// Events
	void	onRadioButtonChanged(wxCommandEvent& e);
	void	onBtnBrowse(wxCommandEvent& e);
};

#endif//__TEMP_FOLDER_WIZARD_PAGE__
