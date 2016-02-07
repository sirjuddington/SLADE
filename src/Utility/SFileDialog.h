
#ifndef __S_FILE_DIALOG_H__
#define __S_FILE_DIALOG_H__

#include <wx/filedlg.h>

namespace SFileDialog
{
	struct fd_info_t
	{
		wxArrayString	filenames;
		string			extension;
		int				ext_index;
		string			path;
	};

	bool	openFile(fd_info_t& info, string caption, string extensions, wxWindow* parent = NULL, string fn_default = "", int ext_default = 0);
	bool	openFiles(fd_info_t& info, string caption, string extensions, wxWindow* parent = NULL, string fn_default = "", int ext_default = 0);
	bool	saveFile(fd_info_t& info, string caption, string extensions, wxWindow* parent = NULL, string fn_default = "", int ext_default = 0);
	bool	saveFiles(fd_info_t& info, string caption, string extensions, wxWindow* parent = NULL, int ext_default = 0);
}

#endif//__S_FILE_DIALOG_H__
