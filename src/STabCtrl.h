
#ifndef __S_TAB_CTRL_H__
#define __S_TAB_CTRL_H__

#include <wx/aui/auibook.h>

class STabCtrl : public wxAuiNotebook
{
private:

public:
	STabCtrl(wxWindow* parent, bool close_buttons = false, bool window_list = false, int height = 32);
	~STabCtrl();
};

#endif//__S_TAB_CTRL_H__
