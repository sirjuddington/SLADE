
#ifndef __SETUP_WIZARD_DIALOG_H__
#define __SETUP_WIZARD_DIALOG_H__

class WizardPageBase;
class SetupWizardDialog : public wxDialog
{
private:
	wxButton*		btn_next;
	wxButton*		btn_prev;
	wxStaticText*	label_page_title;
	wxStaticText*	label_page_description;

	vector<WizardPageBase*>	pages;
	unsigned				current_page;

public:
	SetupWizardDialog(wxWindow* parent);
	~SetupWizardDialog();

	void	setupLayout();
	void	showPage(unsigned index);

	// Events
	void	onBtnNext(wxCommandEvent& e);
	void	onBtnPrev(wxCommandEvent& e);
};

#endif//__SETUP_WIZARD_DIALOG_H__
