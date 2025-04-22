#pragma once

namespace slade
{
class ExtMessageDialog : public wxDialog
{
public:
	ExtMessageDialog(wxWindow* parent, string_view caption);
	~ExtMessageDialog() = default;

	void setMessage(const string& message) const;
	void setExt(const string& text) const;

private:
	wxStaticText* label_message_ = nullptr;
	wxTextCtrl*   text_ext_      = nullptr;

	// Events
	void onSize(wxSizeEvent& e);
};
} // namespace slade
