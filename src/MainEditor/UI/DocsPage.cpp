
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DocsPage.cpp
// Description: A simple panel containing navigation buttons and a browser
//              window to browse the SLADE documentation (GitHub wiki).
//              Only available when compiled with wxWebView enabled
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------

#ifdef USE_WEBVIEW_STARTPAGE // Only available using wxWebView

// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "DocsPage.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "common.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, web_dark_theme)


// -----------------------------------------------------------------------------
//
// Local Functions
//
// -----------------------------------------------------------------------------
namespace
{
string docsUrl()
{
	static const string docs_url      = "http://slade.mancubus.net/embedwiki.php";
	static const string docs_url_dark = "http://slade.mancubus.net/embedwiki-dark.php";

	return web_dark_theme ? docs_url_dark : docs_url;
}
} // namespace


// -----------------------------------------------------------------------------
//
// DocsPage Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DocsPage class constructor
// -----------------------------------------------------------------------------
DocsPage::DocsPage(wxWindow* parent) : wxPanel(parent, -1)
{
	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Create toolbar
	toolbar_ = new SToolBar(this);
	sizer->Add(toolbar_, 0, wxEXPAND);

	// Toolbar 'Navigation' group
	auto g_nav  = new SToolBarGroup(toolbar_, "Navigation");
	tb_back_    = g_nav->addActionButton("back", "Back", "left", "Go back");
	tb_forward_ = g_nav->addActionButton("forward", "Forward", "right", "Go forward");
	toolbar_->addGroup(g_nav);

	// Toolbar 'Links' group
	auto g_links = new SToolBarGroup(toolbar_, "Links");
	tb_home_     = g_links->addActionButton(
        "home", "Home", "wiki", "Return to the SLADE Documentation Wiki main page", true);
	g_links->addActionButton("tutorials", "Tutorials", "wiki", "Go to the tutorials index", true);
	g_links->addActionButton("index", "Wiki Index", "wiki", "Go to the wiki index", true);
	if (global::debug)
		g_links->addActionButton("edit", "Edit on GitHub", "wiki", "Edit this page on GitHub", true);
	toolbar_->addGroup(g_links);

	// Create browser
	wv_browser_ = wxWebView::New(this, -1, wxEmptyString);
	wv_browser_->SetZoomType(wxWEBVIEW_ZOOM_TYPE_LAYOUT);
	sizer->Add(wv_browser_, 1, wxEXPAND);

	// Load initial docs page
	wv_browser_->ClearHistory();
	wv_browser_->LoadURL(wxString::FromUTF8(docsUrl()));

	// Bind button events
	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &DocsPage::onToolbarButton, this, toolbar_->GetId());
	wv_browser_->Bind(wxEVT_WEBVIEW_NAVIGATING, &DocsPage::onHTMLLinkClicked, this);
	wv_browser_->Bind(wxEVT_WEBVIEW_LOADED, &DocsPage::onNavigationDone, this);
}

// -----------------------------------------------------------------------------
// Enables/disables the navigation buttons
// -----------------------------------------------------------------------------
void DocsPage::updateNavButtons() const
{
	tb_back_->Enable(wv_browser_->CanGoBack());
	tb_forward_->Enable(wv_browser_->CanGoForward());
	toolbar_->updateLayout(true);
}

// -----------------------------------------------------------------------------
// Loads the wiki page [page_name]
// -----------------------------------------------------------------------------
void DocsPage::openPage(string_view page_name) const
{
	wv_browser_->LoadURL(WX_FMT("{}?page={}", docsUrl(), page_name));
}

// -----------------------------------------------------------------------------
// Called when a toolbar button is clicked
// -----------------------------------------------------------------------------
void DocsPage::onToolbarButton(wxCommandEvent& e)
{
	auto button = e.GetString();

	// Back
	if (button == wxS("back") && wv_browser_->CanGoBack())
		wv_browser_->GoBack();

	// Forward
	else if (button == wxS("forward") && wv_browser_->CanGoForward())
		wv_browser_->GoForward();

	// Home
	else if (button == wxS("home"))
		wv_browser_->LoadURL(wxString::FromUTF8(docsUrl()));

	// Tutorials
	else if (button == wxS("tutorials"))
		wv_browser_->LoadURL(wxString::FromUTF8(docsUrl() + "?page=Tutorials"));

	// Index
	else if (button == wxS("index"))
		wv_browser_->LoadURL(wxString::FromUTF8(docsUrl() + "?page=Wiki-Index"));

	// Edit
	else if (button == wxS("edit"))
	{
		// Stuff
		auto page = wv_browser_->GetCurrentURL().AfterLast('=');
		wxLaunchDefaultBrowser(wxS("https://github.com/sirjuddington/SLADE/wiki/") + page + wxS("/_edit"));
	}

	// None
	else
		return;

	updateNavButtons();
}

// -----------------------------------------------------------------------------
// Called when a link is clicked in the browser
// -----------------------------------------------------------------------------
void DocsPage::onHTMLLinkClicked(wxEvent& e)
{
	auto& ev   = dynamic_cast<wxWebViewEvent&>(e);
	auto  href = ev.GetURL();

	// Open external links externally
	if (!href.StartsWith(wxString::FromUTF8(docsUrl())))
	{
		wxLaunchDefaultBrowser(href);
		ev.Veto();
	}
}

// -----------------------------------------------------------------------------
// Called when a page finishes loading in the browser
// -----------------------------------------------------------------------------
void DocsPage::onNavigationDone(wxEvent& e)
{
	updateNavButtons();
}

#endif // USE_WEBVIEW_STARTPAGE
