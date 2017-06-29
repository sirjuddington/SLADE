#pragma once

// Test
//#undef USE_WEBVIEW_STARTPAGE

class ArchiveEntry;

class SStartPage : public wxPanel
{
public:
	SStartPage(wxWindow* parent);

	void	init();
	void	load(bool new_tip = true);
	void	refresh();
	void	updateAvailable(string version_name);

#ifdef USE_WEBVIEW_STARTPAGE
	typedef wxWebView WebView;
#else
	typedef wxHtmlWindow WebView;
#endif

private:
	WebView*	html_startpage_ = nullptr;

	vector<string>	tips_;
	int				last_tip_index_ = -1;
	string			latest_news_;
	string			update_version_;

	ArchiveEntry*			entry_base_html_	= nullptr;
	ArchiveEntry*			entry_css_			= nullptr;
	vector<ArchiveEntry*>	entry_export_;

	void	onHTMLLinkClicked(wxEvent& e);
};
