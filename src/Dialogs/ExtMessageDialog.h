#pragma once

class ExtMessageDialog : public wxDialog
{
public:
	ExtMessageDialog(wxWindow* parent, const wxString& caption);
	~ExtMessageDialog() = default;

	void setMessage(const wxString& message) const;
	void setExt(const wxString& text) const;

private:
	wxStaticText* label_message_ = nullptr;
	wxTextCtrl*   text_ext_      = nullptr;

	// Events
	void onSize(wxSizeEvent& e);
};
