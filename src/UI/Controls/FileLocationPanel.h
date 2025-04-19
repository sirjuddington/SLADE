#pragma once

#include "Utility/SFileDialog.h"

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
