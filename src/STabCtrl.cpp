
#include "Main.h"
#include "WxStuff.h"
#include "STabCtrl.h"

STabCtrl::STabCtrl(wxWindow* parent, bool close_buttons, bool window_list, int height) : wxAuiNotebook()
{
	// Determine style
	long style = wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS;
	if (window_list)
		style |= wxAUI_NB_WINDOWLIST_BUTTON;
	if (close_buttons)
		style |= wxAUI_NB_CLOSE_ON_ALL_TABS;

	// Create tab control
	wxAuiNotebook::Create(parent, -1, wxDefaultPosition, wxDefaultSize, style);

	// Setup tabs
	SetArtProvider(getTabArt(close_buttons));
	SetTabCtrlHeight(height);
}

STabCtrl::~STabCtrl()
{
}
