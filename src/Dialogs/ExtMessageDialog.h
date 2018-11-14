#pragma once

class ExtMessageDialog : public wxDialog
{
public:
	ExtMessageDialog(wxWindow* parent, string caption);
	~ExtMessageDialog();

	void setMessage(string message);
	void setExt(string text);

private:
	wxStaticText* label_message_;
	wxTextCtrl*   text_ext_;

	// Events
	void onSize(wxSizeEvent& e);
};
