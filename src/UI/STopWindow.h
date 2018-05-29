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

	// Events
	void	onMenu(wxCommandEvent& e);

protected:
	vector<wxMenu*>	custom_menus_;
	int				custom_menus_begin_;
	SToolBar*		toolbar_;
	string			id_;
	wxMenu*			toolbar_menu_;
	int				toolbar_menu_wx_id_;
	SAction*		action_toolbar_menu_;
};
