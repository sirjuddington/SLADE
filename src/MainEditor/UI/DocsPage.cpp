
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    DocsPage.cpp
 * Description: A simple panel containing navigation buttons and a
 *              browser window to browse the SLADE documentation
 *              (GitHub wiki). Only available when compiled with
 *              wxWebView enabled
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/

#ifdef USE_WEBVIEW_STARTPAGE // Only available using wxWebView

/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "DocsPage.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "common.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
static const string docs_url = "http://slade.mancubus.net/embedwiki.php";


/*******************************************************************
 * DOCSPAGE CLASS FUNCTIONS
 *******************************************************************/

/* DocsPage::DocsPage
 * DocsPage class constructor
 *******************************************************************/
DocsPage::DocsPage(wxWindow* parent) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Create toolbar
	toolbar = new SToolBar(this);
	sizer->Add(toolbar, 0, wxEXPAND);

	// Toolbar 'Navigation' group
	SToolBarGroup* g_nav = new SToolBarGroup(toolbar, "Navigation");
	tb_back = g_nav->addActionButton("back", "Back", "left", "Go back");
	tb_forward = g_nav->addActionButton("forward", "Forward", "right", "Go forward");
	toolbar->addGroup(g_nav);

	// Toolbar 'Links' group
	SToolBarGroup* g_links = new SToolBarGroup(toolbar, "Links");
	tb_home = g_links->addActionButton("home", "Home", "wiki", "Return to the SLADE Documentation Wiki main page", true);
	g_links->addActionButton("tutorials", "Tutorials", "wiki", "Go to the tutorials index", true);
	g_links->addActionButton("index", "Wiki Index", "wiki", "Go to the wiki index", true);
	if (Global::debug)
		g_links->addActionButton("edit", "Edit on GitHub", "wiki", "Edit this page on GitHub", true);
	toolbar->addGroup(g_links);

	// Create browser
	wv_browser = wxWebView::New(this, -1, wxEmptyString);
	wv_browser->SetZoomType(wxWEBVIEW_ZOOM_TYPE_LAYOUT);
	sizer->Add(wv_browser, 1, wxEXPAND);

	// Load initial docs page
	wv_browser->ClearHistory();
	wv_browser->LoadURL(docs_url);

	// Bind button events
	Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &DocsPage::onToolbarButton, this, toolbar->GetId());
	wv_browser->Bind(wxEVT_WEBVIEW_NAVIGATING, &DocsPage::onHTMLLinkClicked, this);
	wv_browser->Bind(wxEVT_WEBVIEW_LOADED, &DocsPage::onNavigationDone, this);
}

/* DocsPage::~DocsPage
 * DocsPage class destructor
 *******************************************************************/
DocsPage::~DocsPage()
{
}

/* DocsPage::updateNavButtons
 * Enables/disables the navigation buttons
 *******************************************************************/
void DocsPage::updateNavButtons()
{
	tb_back->Enable(wv_browser->CanGoBack());
	tb_forward->Enable(wv_browser->CanGoForward());
	toolbar->updateLayout(true);

	/*if (wv_browser->CanGoBack())
		LOG_MESSAGE(0, "Can Go Back");

	if (wv_browser->CanGoForward())
		LOG_MESSAGE(0, "Can Go Forward");*/
}

/* DocsPage::openPage
 * Loads the wiki page [page_name]
 *******************************************************************/
void DocsPage::openPage(string page_name)
{
	wv_browser->LoadURL(docs_url + "?page=" + page_name);
}



/* DocsPage::onToolbarButton
 * Called when a toolbar button is clicked
 *******************************************************************/
void DocsPage::onToolbarButton(wxCommandEvent& e)
{
	string button = e.GetString();

	// Back
	if (button == "back" && wv_browser->CanGoBack())
		wv_browser->GoBack();

	// Forward
	else if (button == "forward" && wv_browser->CanGoForward())
		wv_browser->GoForward();

	// Home
	else if (button == "home")
		wv_browser->LoadURL(docs_url);

	// Tutorials
	else if (button == "tutorials")
		wv_browser->LoadURL(docs_url + "?page=Tutorials");

	// Index
	else if (button == "index")
		wv_browser->LoadURL(docs_url + "?page=Wiki-Index");

	// Edit
	else if (button == "edit")
	{
		// Stuff
		string page = wv_browser->GetCurrentURL().AfterLast('=');
		wxLaunchDefaultBrowser("https://github.com/sirjuddington/SLADE/wiki/" + page + "/_edit");
	}

	// None
	else
		return;

	updateNavButtons();
}

/* DocsPage::onHTMLLinkClicked
 * Called when a link is clicked in the browser
 *******************************************************************/
void DocsPage::onHTMLLinkClicked(wxEvent& e)
{
	wxWebViewEvent& ev = (wxWebViewEvent&)e;
	string href = ev.GetURL();

	// Open external links externally
	if (!href.StartsWith(docs_url))
	{
		wxLaunchDefaultBrowser(href);
		ev.Veto();
	}
}

/* DocsPage::onNavigationDone
 * Called when a page finishes loading in the browser
 *******************************************************************/
void DocsPage::onNavigationDone(wxEvent& e)
{
	updateNavButtons();

	//wxWebViewEvent& ev = (wxWebViewEvent&)e;
	//LOG_MESSAGE(0, ev.GetURL());
}

#endif//USE_WEBVIEW_STARTPAGE
