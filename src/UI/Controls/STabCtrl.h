#pragma once

namespace slade
{
#ifdef WIN32
class STabCtrl;
typedef STabCtrl TabControl;
#else
typedef wxNotebook TabControl;
#endif

class STabCtrl : public wxAuiNotebook
{
public:
	STabCtrl(
		wxWindow* parent,
		bool      close_buttons = false,
		bool      window_list   = false,
		int       height        = -1,
		bool      main_tabs     = false,
		bool      move_tabs     = false);
	~STabCtrl() override = default;

	static TabControl* createControl(
		wxWindow* parent,
		bool      close_buttons = false,
		bool      window_list   = false,
		int       height        = -1,
		bool      main_tabs     = false,
		bool      move_tabs     = false);

protected:
	wxSize DoGetBestClientSize() const override;
};
} // namespace slade
