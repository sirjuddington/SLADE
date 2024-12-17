#pragma once

// Test
// #undef USE_WEBVIEW_STARTPAGE

namespace slade
{
class ArchiveEntry;

class SStartPage : public wxPanel
{
public:
	SStartPage(wxWindow* parent);

	void init();
	void load(bool new_tip = true);
	void refresh() const;
	void updateAvailable(const wxString& version_name);

#ifdef USE_WEBVIEW_STARTPAGE
	typedef wxWebView WebView;
#else
	typedef wxHtmlWindow WebView;
#endif

private:
	WebView* html_startpage_ = nullptr;

	vector<wxString> tips_;
	int              last_tip_index_ = -1;
	wxString         latest_news_;
	wxString         update_version_;

	ArchiveEntry*         entry_base_html_ = nullptr;
	ArchiveEntry*         entry_css_       = nullptr;
	vector<ArchiveEntry*> entry_export_;

	void onHTMLLinkClicked(wxEvent& e);
};
} // namespace slade
