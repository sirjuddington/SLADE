#pragma once

#include "common.h"

class wxWebView;

namespace slade
{
class SToolBar;
class SToolBarButton;

class DocsPage : public wxPanel
{
public:
	DocsPage(wxWindow* parent);
	~DocsPage() = default;

	void updateNavButtons() const;
	void openPage(string_view page_name) const;

private:
	wxWebView*      wv_browser_ = nullptr;
	SToolBar*       toolbar_    = nullptr;
	SToolBarButton* tb_home_    = nullptr;
	SToolBarButton* tb_back_    = nullptr;
	SToolBarButton* tb_forward_ = nullptr;

	// Events
	void onToolbarButton(wxCommandEvent& e);
	void onHTMLLinkClicked(wxEvent& e);
	void onNavigationDone(wxEvent& e);
};
} // namespace slade
