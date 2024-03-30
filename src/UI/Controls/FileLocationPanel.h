#pragma once

wxDECLARE_EVENT(wxEVT_COMMAND_FLP_LOCATION_CHANGED, wxCommandEvent);

namespace slade
{
class FileLocationPanel : public wxPanel
{
public:
	FileLocationPanel(
		wxWindow*       parent,
		const wxString& path,
		bool            editable                = true,
		const wxString& browse_caption          = "Browse File",
		const wxString& browse_extensions       = "All Files|*.*",
		const wxString& browse_default_filename = "");

	wxString location() const;
	void     setLocation(const wxString& path) const;

private:
	wxTextCtrl* text_path_  = nullptr;
	wxButton*   btn_browse_ = nullptr;
	wxString    browse_caption_;
	wxString    browse_extensions_;
	wxString    browse_default_filename_;
};
} // namespace slade
