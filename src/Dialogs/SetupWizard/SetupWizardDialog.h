#pragma once

class WizardPageBase;
class SetupWizardDialog : public wxDialog
{
public:
	SetupWizardDialog(wxWindow* parent);
	~SetupWizardDialog();

	void setupLayout();
	void showPage(unsigned index);

private:
	wxButton*     btn_next_;
	wxButton*     btn_prev_;
	wxStaticText* label_page_title_;
	wxStaticText* label_page_description_;

	vector<WizardPageBase*> pages_;
	unsigned                current_page_;

	// Events
	void onBtnNext(wxCommandEvent& e);
	void onBtnPrev(wxCommandEvent& e);
};
