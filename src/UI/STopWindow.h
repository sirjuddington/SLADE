#pragma once

namespace slade
{
class SAction;
class SToolBar;
class SAuiToolBar;

class STopWindow : public wxFrame
{
public:
	STopWindow(const string& title, const string& id, int xpos = 0, int ypos = 0, int width = 1024, int height = 768);
	~STopWindow() override = default;

	SAuiToolBar* toolbar() const { return toolbar_; }

	// Custom menu
	void addCustomMenu(wxMenu* menu, const string& title);
	void removeCustomMenu(const wxMenu* menu);
	void removeAllCustomMenus();

	// Toolbars
	void showToolbarGroup(string_view name, bool show = true) const;
	void enableToolBarGroup(const string& name, bool enable = true) const;

protected:
	vector<wxMenu*> custom_menus_;
	int             custom_menus_begin_ = 0;
	SAuiToolBar*    toolbar_            = nullptr;
	string          id_;
};
} // namespace slade
