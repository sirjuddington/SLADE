
#include "Main.h"
#include "WxStuff.h"
#include "STabCtrl.h"

STabCtrl::STabCtrl(wxWindow* parent, bool close_buttons, bool window_list, int height, bool main_tabs) : wxAuiNotebook()
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
	SetArtProvider(getTabArt(close_buttons, main_tabs));
	SetTabCtrlHeight(height);
}

STabCtrl::~STabCtrl()
{
}

// wxAuiNotebook doesn't automatically set its own minimum size to the minimum
// size of its contents, so we have to do that for it.  See
// http://trac.wxwidgets.org/ticket/4698
wxSize STabCtrl::DoGetBestClientSize() const
{
	wxSize ret;
	for (unsigned i = 0; i < GetPageCount(); i++)
	{
		wxWindow* page = GetPage(i);
		ret.IncTo(page->GetBestSize());
	}

	ret.IncBy(0, GetTabCtrlHeight());

	return ret;
}
