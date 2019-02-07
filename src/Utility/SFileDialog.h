#pragma once

namespace SFileDialog
{
struct FDInfo
{
	wxArrayString filenames;
	wxString      extension;
	int           ext_index;
	wxString      path;
};

bool openFile(
	FDInfo&         info,
	const wxString& caption,
	const wxString& extensions,
	wxWindow*       parent      = nullptr,
	const wxString& fn_default  = "",
	int             ext_default = 0);

bool openFiles(
	FDInfo&         info,
	const wxString& caption,
	const wxString& extensions,
	wxWindow*       parent      = nullptr,
	const wxString& fn_default  = "",
	int             ext_default = 0);

bool saveFile(
	FDInfo&         info,
	const wxString& caption,
	const wxString& extensions,
	wxWindow*       parent      = nullptr,
	const wxString& fn_default  = "",
	int             ext_default = 0);

bool saveFiles(
	FDInfo&         info,
	const wxString& caption,
	const wxString& extensions,
	wxWindow*       parent      = nullptr,
	int             ext_default = 0);

wxString executableExtensionString();
wxString executableFileName(const wxString& exe_name);
} // namespace SFileDialog
