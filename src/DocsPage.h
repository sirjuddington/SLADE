
#ifndef __DOCS_PAGE_H__
#define __DOCS_PAGE_H__

#include "SToolBar.h"

class wxWebView;
class DocsPage : public wxPanel
{
private:
	wxWebView*		wv_browser;
	SToolBar*		toolbar;
	SToolBarButton*	tb_home;
	SToolBarButton*	tb_back;
	SToolBarButton*	tb_forward;

public:
	DocsPage(wxWindow* parent);
	~DocsPage();

	void	updateNavButtons();
	void	openPage(string page_name);

	// Events
	void	onToolbarButton(wxCommandEvent& e);
	void	onHTMLLinkClicked(wxEvent& e);
	void	onNavigationDone(wxEvent& e);
};

#endif//__DOCS_PAGE_H__
