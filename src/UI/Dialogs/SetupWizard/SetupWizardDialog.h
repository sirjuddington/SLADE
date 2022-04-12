#pragma once

namespace slade
{
class WizardPageBase;
class SetupWizardDialog : public wxDialog
{
public:
	SetupWizardDialog(wxWindow* parent);
	~SetupWizardDialog() override = default;

	void setupLayout();
	void showPage(unsigned index);

private:
	wxButton*     btn_next_               = nullptr;
	wxButton*     btn_prev_               = nullptr;
	wxStaticText* label_page_title_       = nullptr;
	wxStaticText* label_page_description_ = nullptr;

	vector<WizardPageBase*> pages_;
	unsigned                current_page_ = 0;

	// Events
	void onBtnNext(wxCommandEvent& e);
	void onBtnPrev(wxCommandEvent& e);
};
} // namespace slade
