
#ifndef __S_TOP_WINDOW_H__
#define __S_TOP_WINDOW_H__

class SToolBar;
class STopWindow : public wxFrame {
protected:
	vector<wxMenu*>	custom_menus;
	int				custom_menus_begin;
	SToolBar*		toolbar;

public:
	STopWindow(string title, int xpos = 0, int ypos = 0, int width = 1024, int height = 768);
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
};

#endif//__S_TOP_WINDOW_H__
