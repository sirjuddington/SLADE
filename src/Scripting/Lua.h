
#ifndef __LUA_H__
#define __LUA_H__

class wxWindow;
namespace sol { class state; }

namespace Lua
{
	bool	init();
	void	close();
	bool	run(string program);
	bool	runFile(string filename);

	sol::state&	state();

	wxWindow*	currentWindow();
	void		setCurrentWindow(wxWindow* window);
}

#endif//__LUA_H__
