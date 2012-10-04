
#include "Main.h"
#include "WxStuff.h"
#include "STopWindow.h"
#include "SToolBar.h"
#include <wx/aui/aui.h>

/* STopWindow::STopWindow
 * STopWindow class constructor
 *******************************************************************/
STopWindow::STopWindow(string title, int x, int y, int width, int height)
: wxFrame(NULL, -1, title, wxPoint(x, y), wxSize(width, height)) {
	custom_menus_begin = 0;
}

/* STopWindow::~STopWindow
 * STopWindow class destructor
 *******************************************************************/
STopWindow::~STopWindow() {
}

/* STopWindow::addCustomMenu
 * Adds [menu] to the menu bar after the 'Entry' menu
 *******************************************************************/
void STopWindow::addCustomMenu(wxMenu* menu, string title) {
	// Check menu doesn't already exist
	for (unsigned a = 0; a < custom_menus.size(); a++) {
		if (custom_menus[a] == menu)
			return;
	}

	// Insert custom menu after the last existing custom menu
	GetMenuBar()->Insert(custom_menus_begin + custom_menus.size(), menu, title);
	GetMenuBar()->Refresh();
	custom_menus.push_back(menu);
}

/* STopWindow::removeCustomMenu
 * Removes the menu matching [title] from the menu bar
 *******************************************************************/
void STopWindow::removeCustomMenu(wxMenu* menu) {
	// Go through custom menus
	for (unsigned a = 0; a < custom_menus.size(); a++) {
		if (custom_menus[a] == menu) {
			// Menu found, remove it
			custom_menus.erase(custom_menus.begin() + a);
			GetMenuBar()->Remove(custom_menus_begin + a);
			return;
		}
	}
}

/* STopWindow::removeAllCustomMenus
 * Removes all custom menus from the menu bar
 *******************************************************************/
void STopWindow::removeAllCustomMenus() {
	// Remove all custom menus from menubar
	while (custom_menus.size() > 0)
		removeCustomMenu(custom_menus[0]);
}

/* STopWindow::enableToolBar
 * Enables/disables the toolbar group matching [name]
 *******************************************************************/
void STopWindow::enableToolBar(string name, bool enable) {
	toolbar->enableGroup(name, enable);
}

/* STopWindow::addCustomToolBar
 * Adds a custom toolbar group to the toolbar, with buttons for each
 * action in [actions]
 *******************************************************************/
void STopWindow::addCustomToolBar(string name, wxArrayString actions) {
	toolbar->addActionGroup(name, actions);
}

/* STopWindow::removeCustomToolBar
 * Removes the toolbar group matching [name]
 *******************************************************************/
void STopWindow::removeCustomToolBar(string name) {
	toolbar->deleteGroup(name);
}

/* STopWindow::removeAllCustomToolBars
 * Removes all custom toolbar groups
 *******************************************************************/
void STopWindow::removeAllCustomToolBars() {
	toolbar->deleteCustomGroups();
}
