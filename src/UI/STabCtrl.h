
#ifndef __S_TAB_CTRL_H__
#define __S_TAB_CTRL_H__

#include "common.h"

#ifdef WIN32
class STabCtrl;
typedef STabCtrl TabControl;
#else
typedef wxNotebook TabControl;
#endif

class STabCtrl : public wxAuiNotebook
{
private:

public:
	STabCtrl(
		wxWindow* parent,
		bool close_buttons = false,
		bool window_list = false,
		int height = -1,
		bool main_tabs = false
	);
	~STabCtrl();

	static TabControl*	createControl(
							wxWindow* parent,
							bool close_buttons = false,
							bool window_list = false,
							int height = -1,
							bool main_tabs = false
						);

protected:
	wxSize	DoGetBestClientSize() const;
};

#endif//__S_TAB_CTRL_H__
