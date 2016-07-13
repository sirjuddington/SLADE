
#ifndef __S_TAB_CTRL_H__
#define __S_TAB_CTRL_H__

#include "common.h"

class STabCtrl : public wxAuiNotebook
{
private:

public:
	STabCtrl(wxWindow* parent, bool close_buttons = false, bool window_list = false, int height = 24, bool main_tabs = false);
	~STabCtrl();

protected:
	wxSize	DoGetBestClientSize() const;
};

#endif//__S_TAB_CTRL_H__
