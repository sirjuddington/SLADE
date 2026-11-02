#pragma once

wxDECLARE_EVENT(wxEVT_COMMAND_FLP_LOCATION_CHANGED, wxCommandEvent);

namespace slade
{
class FileLocationPanel : public wxPanel
{
public:
	FileLocationPanel(
		wxWindow*     parent,
		const string& path,
		bool          editable                = true,
		const string& browse_caption          = "Browse File",
		const string& browse_extensions       = "All Files|*",
		const string& browse_default_filename = "");

	const string& browseCaption() const { return browse_caption_; }
	const string& browseExtensions() const { return browse_extensions_; }
	const string& defaultFilename() const { return browse_default_filename_; }

	void setBrowseCaption(const string& caption) { browse_caption_ = caption; }
	void setBrowseExtensions(const string& extensions) { browse_extensions_ = extensions; }
	void setDefaultFilename(const string& filename) { browse_default_filename_ = filename; }

	string location() const;
	void   setLocation(const string& path) const;

private:
	wxTextCtrl* text_path_  = nullptr;
	wxButton*   btn_browse_ = nullptr;
	string      browse_caption_;
	string      browse_extensions_;
	string      browse_default_filename_;
};
} // namespace slade
