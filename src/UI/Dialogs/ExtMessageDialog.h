#pragma once

namespace slade
{
class ExtMessageDialog : public wxDialog
{
public:
	ExtMessageDialog(wxWindow* parent, const wxString& caption);
	~ExtMessageDialog() override = default;

	void setMessage(const wxString& message) const;
	void setExt(const wxString& text) const;

private:
	wxStaticText* label_message_ = nullptr;
	wxTextCtrl*   text_ext_      = nullptr;

	// Events
	void onSize(wxSizeEvent& e);
};
} // namespace slade
