
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    StartPage.cpp
// Description: SLADE Start Page implementation. If wxWebView support is
//              enabled, the full featured start page is shown in a wxWebView.
//              Otherwise, a (much) more basic version of the start page is
//              shown in a wxHtmlWindow.
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


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "StartPage.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/Library.h"
#include "General/SAction.h"
#include "General/Web.h"
#include "Utility/Tokenizer.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, web_dark_theme, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, iconset_general)
EXTERN_CVAR(String, iconset_entry_list)


// -----------------------------------------------------------------------------
//
// SStartPage Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SStartPage class constructor
// -----------------------------------------------------------------------------
SStartPage::SStartPage(wxWindow* parent) : wxPanel(parent, -1)
{
	wxPanel::SetName("startpage");

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
}

// -----------------------------------------------------------------------------
// Initialises the start page
// -----------------------------------------------------------------------------
void SStartPage::init()
{
	// wxWebView
#ifdef USE_WEBVIEW_STARTPAGE
	html_startpage_ = wxWebView::New(
		this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxWebViewBackendDefault, wxBORDER_NONE);
	html_startpage_->SetZoomType(app::platform() == app::MacOS ? wxWEBVIEW_ZOOM_TYPE_TEXT : wxWEBVIEW_ZOOM_TYPE_LAYOUT);

	// wxHtmlWindow
#else
	html_startpage_ = new wxHtmlWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_NEVER, "startpage");
#endif

	// Add to sizer
	GetSizer()->Add(html_startpage_, 1, wxEXPAND);

	// Bind events
#ifdef USE_WEBVIEW_STARTPAGE
	html_startpage_->Bind(wxEVT_WEBVIEW_NAVIGATING, &SStartPage::onHTMLLinkClicked, this);

	html_startpage_->Bind(
		wxEVT_WEBVIEW_ERROR,
		[&](wxWebViewEvent& e) { log::error(wxString::Format("wxWebView Error: %s", e.GetString())); });

	if (app::platform() == app::Platform::Windows)
	{
		html_startpage_->Bind(wxEVT_WEBVIEW_LOADED, [&](wxWebViewEvent& e) { html_startpage_->Reload(); });
	}

	Bind(
		wxEVT_THREAD_WEBGET_COMPLETED,
		[&](wxThreadEvent& e)
		{
			latest_news_ = e.GetString();
			latest_news_.Trim();

			if (latest_news_ == "connect_failed" || latest_news_.empty())
				latest_news_ = "<center>Unable to load latest SLADE news</center>";

			load(false);
		});

#else
	html_startpage_->Bind(wxEVT_COMMAND_HTML_LINK_CLICKED, &SStartPage::onHTMLLinkClicked, this);
#endif

	// Get data used to build the page
	auto res_archive = app::archiveManager().programResourceArchive();
	if (res_archive)
	{
		entry_base_html_ = res_archive->entryAtPath(
			app::useWebView() ? "html/startpage.htm" : "html/startpage_basic.htm");

		entry_css_ = res_archive->entryAtPath(web_dark_theme ? "html/theme-dark.css" : "html/theme-light.css");

		entry_export_.push_back(res_archive->entryAtPath("html/base.css"));
		entry_export_.push_back(res_archive->entryAtPath("fonts/FiraSans-Regular.woff"));
		entry_export_.push_back(res_archive->entryAtPath("fonts/FiraSans-Italic.woff"));
		entry_export_.push_back(res_archive->entryAtPath("fonts/FiraSans-Medium.woff"));
		entry_export_.push_back(res_archive->entryAtPath("fonts/FiraSans-Bold.woff"));
		entry_export_.push_back(res_archive->entryAtPath("fonts/FiraSans-Heavy.woff"));
		entry_export_.push_back(res_archive->entryAtPath("logo_icon.png"));
		entry_export_.push_back(res_archive->entryAtPath("icons/entry_list/archive.svg"));
		entry_export_.push_back(res_archive->entryAtPath("icons/entry_list/wad.svg"));
		entry_export_.push_back(res_archive->entryAtPath("icons/entry_list/zip.svg"));
		entry_export_.push_back(res_archive->entryAtPath("icons/entry_list/folder.svg"));
		entry_export_.push_back(res_archive->entryAtPath("icons/general/open.svg"));
		entry_export_.push_back(res_archive->entryAtPath("icons/general/opendir.svg"));
		entry_export_.push_back(res_archive->entryAtPath("icons/general/newarchive.svg"));
		entry_export_.push_back(res_archive->entryAtPath("icons/general/mapeditor.svg"));
		entry_export_.push_back(res_archive->entryAtPath("icons/general/wiki.svg"));

		// Load tips
		auto entry_tips = res_archive->entryAtPath("tips.txt");
		if (entry_tips)
		{
			Tokenizer tz;
			tz.openMem((const char*)entry_tips->rawData(), entry_tips->size(), entry_tips->name());
			while (!tz.atEnd() && !tz.peekToken().empty())
				tips_.emplace_back(tz.getToken());
		}
	}
}


#ifdef USE_WEBVIEW_STARTPAGE

// -----------------------------------------------------------------------------
// Loads the start page (wxWebView implementation). If [new_tip] is true, a new
// random 'tip of the day' is shown
// -----------------------------------------------------------------------------
void SStartPage::load(bool new_tip)
{
	// Get latest news post
	if (latest_news_.empty())
		web::getHttpAsync("slade.mancubus.net", "/news-latest.php", this);

	// Can't do anything without html entry
	if (!entry_base_html_)
	{
		log::error("No start page resource found");
		html_startpage_->SetPage(
			"<html><head><title>SLADE</title></head><body><center><h1>"
			"Something is wrong with slade.pk3 :(</h1><center></body></html>",
			wxEmptyString);
		return;
	}

	// Get html as string
	wxString html = wxString::FromAscii((const char*)(entry_base_html_->rawData()), entry_base_html_->size());

	// Read css
	wxString css;
	if (entry_css_)
		css = wxString::FromAscii((const char*)(entry_css_->rawData()), entry_css_->size());

	// Generate tip of the day string
	wxString tip;
	if (tips_.size() < 2) // Needs at least two choices or it's kinda pointless.
		tip = "Did you know? Something is wrong with the tips.txt file in your slade.pk3.";
	else
	{
		int tipindex = last_tip_index_;
		if (new_tip || last_tip_index_ <= 0)
		{
			// Don't show same tip twice in a row
			do
			{
				tipindex = 1 + rand() % (tips_.size() - 1);
			} while (tipindex == last_tip_index_);
		}

		last_tip_index_ = tipindex;
		tip             = tips_[tipindex];
	}

	// Generate recent files string
	wxString recent;
	auto     recent_files = library::recentFiles();
	if (!recent_files.empty())
	{
		for (unsigned a = 0; a < 12; a++)
		{
			if (a >= recent_files.size())
				break; // No more recent files

			// Determine icon
			wxString fn   = recent_files[a];
			wxString icon = "archive";
			if (fn.EndsWith(".wad"))
				icon = "wad";
			else if (fn.EndsWith(".zip") || fn.EndsWith(".pk3") || fn.EndsWith(".pke"))
				icon = "zip";
			else if (wxDirExists(fn))
				icon = "folder";

			// Add recent file row
			recent += wxString::Format(
				"<div class=\"link\">"
				"<img src=\"%s.svg\" class=\"link\" />"
				"<a class=\"link\" href=\"recent://%d\">%s</a>"
				"</div>",
				icon,
				a,
				fn);
		}
	}
	else
		recent = "No recently opened files";

	// Replace placeholders in the html (theme css, recent files, tip, etc.)
	html.Replace("/*#theme#*/", css);
	html.Replace("#recent#", recent);
	html.Replace("#totd#", tip);
	html.Replace("#news#", latest_news_);
	html.Replace("#version#", app::version().toString());
	if (update_version_.empty())
		html.Replace("/*#hideupdate#*/", "#update { display: none; }");
	else
		html.Replace("#updateversion#", update_version_);

	// Write html and images to temp folder
	for (auto& a : entry_export_)
		a->exportFile(app::path(a->name(), app::Dir::Temp));
	wxString html_file = app::path("startpage.htm", app::Dir::Temp);
	wxFile   outfile(html_file, wxFile::write);
	outfile.Write(html);
	outfile.Close();

	if (app::platform() == app::Linux)
		html_file = "file://" + html_file;

	// Load page
	html_startpage_->ClearHistory();
	if (app::platform() == app::Windows)
	{
		html_startpage_->LoadURL(html_file);
		html_startpage_->Reload();
	}
	else
		html_startpage_->SetPage(html, html_file);
}

#else

// -----------------------------------------------------------------------------
// Loads the start page (wxHtmlWindow implementation). If [new_tip] is true, a
// new random 'tip of the day' is shown
// -----------------------------------------------------------------------------
void SStartPage::load(bool new_tip)
{
	// Get relevant resource entries
	Archive* res_archive = app::archiveManager().programResourceArchive();
	if (!res_archive)
		return;
	ArchiveEntry* entry_html = res_archive->entryAtPath("html/startpage_basic.htm");
	ArchiveEntry* entry_logo = res_archive->entryAtPath("logo.png");
	ArchiveEntry* entry_tips = res_archive->entryAtPath("tips.txt");

	// Can't do anything without html entry
	if (!entry_html)
	{
		html_startpage_->SetPage(
			"<html><head><title>SLADE</title></head><body><center><h1>Something is wrong with slade.pk3 "
			":(</h1><center></body></html>");
		return;
	}

	// Get html as string
	wxString html = wxString::FromAscii((const char*)(entry_html->rawData()), entry_html->size());

	// Generate tip of the day string
	string tip;
	if (tips_.size() < 2) // Needs at least two choices or it's kinda pointless.
		tip = "Did you know? Something is wrong with the tips.txt file in your slade.pk3.";
	else
	{
		int tipindex = last_tip_index_;
		if (new_tip || last_tip_index_ <= 0)
		{
			// Don't show same tip twice in a row
			do
			{
				tipindex = 1 + rand() % (tips_.size() - 1);
			} while (tipindex == last_tip_index_);
		}

		last_tip_index_ = tipindex;
		tip = tips_[tipindex];
		// log::debug(wxString::Format("Tip index %d/%lu", last_tip_index_, (int)tips_.size()));
	}

	// Generate recent files string
	string recent;
	for (unsigned a = 0; a < 12; a++)
	{
		if (a >= app::archiveManager().numRecentFiles())
			break; // No more recent files

		// Add line break if needed
		if (a > 0)
			recent += "<br/>\n";

		// Add recent file link
		recent += wxString::Format("<a href=\"recent://%d\">%s</a>", a, app::archiveManager().recentFile(a));
	}

	// Insert tip and recent files into html
	html.Replace("#recent#", recent);
	html.Replace("#totd#", tip);

	// Write html and images to temp folder
	if (entry_logo)
		entry_logo->exportFile(app::path("logo.png", app::Dir::Temp));
	string html_file = app::path("startpage_basic.htm", app::Dir::Temp);
	wxFile outfile(html_file, wxFile::write);
	outfile.Write(html);
	outfile.Close();

	// Load page
	html_startpage_->LoadPage(html_file);

	// Clean up
	wxRemoveFile(html_file);
	wxRemoveFile(app::path("logo.png", app::Dir::Temp));
}

#endif

// -----------------------------------------------------------------------------
// Refreshes the page (wxWebView only)
// -----------------------------------------------------------------------------
void SStartPage::refresh() const
{
#ifdef USE_WEBVIEW_STARTPAGE
	html_startpage_->Reload();
#endif
}

// -----------------------------------------------------------------------------
// Updates the start page to show that an update to [version_name] is available
// -----------------------------------------------------------------------------
void SStartPage::updateAvailable(const wxString& version_name)
{
	update_version_ = version_name;
	load(false);
}


#ifdef USE_WEBVIEW_STARTPAGE

// -----------------------------------------------------------------------------
// Called when a link is clicked on the start page, so that external (http)
// links are opened in the default browser (wxWebView implementation)
// -----------------------------------------------------------------------------
void SStartPage::onHTMLLinkClicked(wxEvent& e)
{
	auto&    ev   = dynamic_cast<wxWebViewEvent&>(e);
	wxString href = ev.GetURL();

#ifdef __WXGTK__
	if (!href.EndsWith("startpage.htm"))
		href.Replace("file://", "");
#endif

	if (href.EndsWith("/"))
		href.RemoveLast(1);

	if (href.StartsWith("http://") || href.StartsWith("https://"))
	{
		wxLaunchDefaultBrowser(ev.GetURL());
		ev.Veto();
	}
	else if (href.StartsWith("recent://"))
	{
		// Recent file
		wxString      rs    = href.Mid(9);
		unsigned long index = 0;
		rs.ToULong(&index);
		SActionHandler::setWxIdOffset(index);
		SActionHandler::doAction("aman_recent");
		load();
		html_startpage_->Reload();
	}
	else if (href.StartsWith("action://"))
	{
		// Action
		if (href.EndsWith("open"))
			SActionHandler::doAction("aman_open");
		else if (href.EndsWith("opendir"))
			SActionHandler::doAction("aman_opendir");
		else if (href.EndsWith("newarchive"))
			SActionHandler::doAction("aman_newarchive");
		else if (href.EndsWith("newmap"))
		{
			SActionHandler::doAction("aman_newmap");
			return;
		}
		else if (href.EndsWith("reloadstartpage"))
			load();
		else if (href.EndsWith("hide-update"))
		{
			update_version_ = "";
			load(false);
		}
		else if (href.EndsWith("update"))
		{
			if (wxLaunchDefaultBrowser("http://slade.mancubus.net/index.php?page=downloads"))
			{
				update_version_ = "";
				load(false);
			}
		}

		html_startpage_->Reload();
	}
	else if (wxFileExists(href))
	{
		// Navigating to file, open it
		if (!href.EndsWith("startpage.htm"))
		{
			app::archiveManager().openArchive(href.ToStdString());
			ev.Veto();
		}
	}
	else if (wxDirExists(href))
	{
		// Navigating to folder, open it
		app::archiveManager().openDirArchive(href.ToStdString());
		ev.Veto();
	}
}

#else

// -----------------------------------------------------------------------------
// Called when a link is clicked on the start page, so that external (http)
// links are opened in the default browser (wxHtmlWindow implementation)
// -----------------------------------------------------------------------------
void SStartPage::onHTMLLinkClicked(wxEvent& e)
{
	wxHtmlLinkEvent& ev = (wxHtmlLinkEvent&)e;
	wxString href = ev.GetLinkInfo().GetHref();

	if (href.StartsWith("http://") || href.StartsWith("https://"))
		wxLaunchDefaultBrowser(ev.GetLinkInfo().GetHref());
	else if (href.StartsWith("recent://"))
	{
		// Recent file
		wxString rs = href.Mid(9);
		unsigned long index = 0;
		rs.ToULong(&index);
		SActionHandler::setWxIdOffset(index);
		SActionHandler::doAction("aman_recent");
		load();
	}
	else if (href.StartsWith("action://"))
	{
		// Action
		if (href.EndsWith("open"))
			SActionHandler::doAction("aman_open");
		else if (href.EndsWith("newwad"))
			SActionHandler::doAction("aman_newwad");
		else if (href.EndsWith("newzip"))
			SActionHandler::doAction("aman_newzip");
		else if (href.EndsWith("newmap"))
			SActionHandler::doAction("aman_newmap");
		else if (href.EndsWith("reloadstartpage"))
			load();
	}
	else
		html_startpage_->OnLinkClicked(ev.GetLinkInfo());
}

#endif
