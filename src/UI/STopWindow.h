#pragma once

#include "common.h"

class SAction;
class SToolBar;
class wxMenu;

class STopWindow : public wxFrame
{
public:
	STopWindow(string title, string id, int xpos = 0, int ypos = 0, int width = 1024, int height = 768);
	~STopWindow();

	// Custom menu
	void	addCustomMenu(wxMenu* menu, string title);
	void	removeCustomMenu(wxMenu* menu);
	void	removeAllCustomMenus();

	// Toolbars
	void	enableToolBar(string name, bool enable = true);
	void	addCustomToolBar(string name, wxArrayString actions);
	void	removeCustomToolBar(string name);
	void	removeAllCustomToolBars();
	void	populateToolbarsMenu() const;

	// Static
	static STopWindow* currentActive() { return current_active_window_; }

protected:
	vector<wxMenu*>	custom_menus_;
	int				custom_menus_begin_		= 0;
	SToolBar*		toolbar_				= nullptr;
	string			id_;
	wxMenu*			toolbar_menu_			= nullptr;
	int				toolbar_menu_wx_id_		= 0;
	SAction*		action_toolbar_menu_	= nullptr;

private:
	static STopWindow*	current_active_window_;

	// Events
	void	onMenu(wxCommandEvent& e);
};
