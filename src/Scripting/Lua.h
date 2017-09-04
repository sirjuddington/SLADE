#pragma once

class wxWindow;
namespace sol { class state; }

namespace Lua
{
	bool	init();
	void	close();

	struct Error
	{
		string	type;
		string	message;
		int		line_no;
	};
	Error&	error();
	void	showErrorDialog(
		wxWindow* parent = nullptr,
		const string& title = "Script Error",
		const string& message = "An error occurred running the script, see details below"
	);

	bool	run(string program);
	bool	runFile(string filename);
	bool	runArchiveScript(const string& script, Archive* archive);
	bool	runEntryScript(const string& script, vector<ArchiveEntry*> entries);

	sol::state&	state();

	wxWindow*	currentWindow();
	void		setCurrentWindow(wxWindow* window);
}
