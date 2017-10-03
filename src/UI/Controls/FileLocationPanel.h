#pragma once

#include "Utility/SFileDialog.h"

wxDECLARE_EVENT(wxEVT_COMMAND_FLP_LOCATION_CHANGED, wxCommandEvent);

class FileLocationPanel : public wxPanel
{
public:
	FileLocationPanel(
		wxWindow* parent,
		string path,
		bool editable = true,
		string browse_caption = "Browse File",
		string browse_extensions = "All Files|*.*",
		string browse_default_filename = ""
	);

	string	location() const;
	void 	setLocation(const string& path) const;

private:
	wxTextCtrl*	text_path_;
	wxButton*	btn_browse_;
	string		browse_caption_;
	string		browse_extensions_;
	string		browse_default_filename_;
};
