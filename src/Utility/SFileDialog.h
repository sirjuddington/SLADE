
#ifndef __S_FILE_DIALOG_H__
#define __S_FILE_DIALOG_H__

#include "common.h"

namespace SFileDialog
{
	struct fd_info_t
	{
		wxArrayString	filenames;
		string			extension;
		int				ext_index;
		string			path;
	};

	bool	openFile(
				fd_info_t& info,
				string caption,
				string extensions,
				wxWindow* parent = nullptr,
				string fn_default = "",
				int ext_default = 0
			);
	bool	openFiles(
				fd_info_t& info,
				string caption,
				string extensions,
				wxWindow* parent = nullptr,
				string fn_default = "",
				int ext_default = 0
			);
	bool	saveFile(
				fd_info_t& info,
				string caption,
				string extensions,
				wxWindow* parent = nullptr,
				string fn_default = "",
				int ext_default = 0
			);
	bool	saveFiles(
				fd_info_t& info,
				string caption,
				string extensions,
				wxWindow* parent = nullptr,
				int ext_default = 0
			);

	string	executableExtensionString();
	string	executableFileName(const string& exe_name);
}

#endif//__S_FILE_DIALOG_H__
