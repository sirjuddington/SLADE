#pragma once

class ExtMessageDialog : public wxDialog
{
public:
	ExtMessageDialog(wxWindow* parent, const string& caption);
	~ExtMessageDialog() = default;

	void setMessage(const string& message) const;
	void setExt(const string& text) const;

private:
	wxStaticText* label_message_ = nullptr;
	wxTextCtrl*   text_ext_      = nullptr;

	// Events
	void onSize(wxSizeEvent& e);
};
