
#ifndef __LUA_H__
#define __LUA_H__

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

	sol::state&	state();

	wxWindow*	currentWindow();
	void		setCurrentWindow(wxWindow* window);
}

#endif//__LUA_H__
