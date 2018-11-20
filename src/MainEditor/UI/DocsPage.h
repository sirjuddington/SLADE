#pragma once

#include "common.h"

class wxWebView;
class SToolBar;
class SToolBarButton;

class DocsPage : public wxPanel
{
public:
	DocsPage(wxWindow* parent);
	~DocsPage();

	void	updateNavButtons();
	void	openPage(string page_name);

private:
	wxWebView*      wv_browser_;
	SToolBar*       toolbar_;
	SToolBarButton* tb_home_;
	SToolBarButton* tb_back_;
	SToolBarButton* tb_forward_;

	// Events
	void	onToolbarButton(wxCommandEvent& e);
	void	onHTMLLinkClicked(wxEvent& e);
	void	onNavigationDone(wxEvent& e);
};
