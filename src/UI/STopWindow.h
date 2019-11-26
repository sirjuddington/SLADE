#pragma once

class SAction;
class SToolBar;
class wxMenu;

class STopWindow : public wxFrame
{
public:
	STopWindow(
		const wxString& title,
		const wxString& id,
		int             xpos   = 0,
		int             ypos   = 0,
		int             width  = 1024,
		int             height = 768);
	~STopWindow();

	// Custom menu
	void addCustomMenu(wxMenu* menu, const wxString& title);
	void removeCustomMenu(wxMenu* menu);
	void removeAllCustomMenus();

	// Toolbars
	void enableToolBar(const wxString& name, bool enable = true) const;
	void addCustomToolBar(const wxString& name, const wxArrayString& actions) const;
	void removeCustomToolBar(const wxString& name) const;
	void removeAllCustomToolBars() const;
	void populateToolbarsMenu() const;

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
