
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    AboutDialog.cpp
// Description: AboutDialog class, a custom 'About' dialog for SLADE
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
#include "AboutDialog.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "OpenGL/OpenGL.h"
#include "UI/Controls/NoFocusHtmlWindow.h"
#include "UI/Controls/ReleaseNotesPanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"
#include <SFML/Config.hpp>
#include <wx/hyperlink.h>
#include <wx/statbmp.h>
#ifndef NO_LUA
#include <lua.h>
#endif

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns the side panel colour for the about dialog (same as settings dialog)
// -----------------------------------------------------------------------------
wxColour sidePanelColour()
{
	auto bgcol = wxutil::systemPanelBGColour();
	return app::isDarkTheme() ? bgcol.ChangeLightness(105) : bgcol.ChangeLightness(95);
}

// -----------------------------------------------------------------------------
// Returns a wxBitmapBundle containing the SLADE logo at the specified [size]
// -----------------------------------------------------------------------------
wxBitmapBundle logoBitmapBundle(int size)
{
	auto svg_entry = app::archiveManager().programResourceArchive()->entryAtPath("icons/general/logo.svg");
	return wxBitmapBundle::FromSVG(
		reinterpret_cast<const wxByte*>(svg_entry->rawData()), svg_entry->size(), { size, size });
}

// -----------------------------------------------------------------------------
// Returns the contributors list from the program resource archive as html text
// -----------------------------------------------------------------------------
string contributorsHtml()
{
	auto entry = app::archiveManager().programResourceArchive()->entryAtPath("contributors.txt");
	if (!entry)
		return "!!! Error reading contributors list";

	auto entry_string = entry->data().asString();
	auto contributors = strutil::splitV(entry_string, '\n');

	string contributors_text =
		"<p>Big thanks to all contributors who have helped with the development of SLADE over the years:</p><br><p>";
	for (auto& c : contributors)
	{
		if (c.empty())
			continue;
		contributors_text.append(c);
		contributors_text.append(", ");
	}
	strutil::removeLastIP(contributors_text, 2);
	contributors_text.append("</p>");

	return contributors_text;
}

// -----------------------------------------------------------------------------
// Creates the left panel containing the logo, name, tagline and version
// -----------------------------------------------------------------------------
wxPanel* createLogoPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	panel->SetBackgroundColour(sidePanelColour());
	auto lh   = LayoutHelper(panel);
	auto vbox = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(vbox);

	vbox->AddSpacer(lh.pad());

	// Logo
	vbox->Add(new wxStaticBitmap(panel, -1, logoBitmapBundle(180)), lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT).Center());

	// SLADE
	auto slade_label = new wxStaticText(panel, -1, wxS("SLADE"));
	slade_label->SetFont(slade_label->GetFont().Bold().Scale(3.0f));
	vbox->Add(slade_label, wxSizerFlags(0).Center());

	// Tagline
	auto tagline_label = new wxStaticText(panel, -1, wxS("It's a Doom Editor"));
	tagline_label->SetFont(tagline_label->GetFont().Italic().Scale(1.2f));
	vbox->Add(tagline_label, lh.sfWithBorder(0, wxBOTTOM).Center());

	// Version
	string version = "v" + app::version().toString();
	if (!global::sc_rev.empty())
		version += " (Git Rev " + global::sc_rev + ")";
	auto version_label = new wxStaticText(panel, -1, wxString::FromUTF8(version));
	version_label->SetFont(version_label->GetFont().Bold());
	vbox->Add(version_label, wxSizerFlags().Center());

	vbox->AddStretchSpacer();

	// Copyright
	auto year            = wxNow().Right(4);
	auto copyright_label = new wxStaticText(
		panel, -1, wxString::FromUTF8(fmt::format("\xC2\xA9 2008-{} Simon Judd", year.utf8_string())));
	vbox->Add(copyright_label, wxSizerFlags().Center());

	// Website link
	vbox->Add(
		new wxHyperlinkCtrl(panel, -1, wxS("slade.mancubus.net"), wxS("https://slade.mancubus.net")),
		lh.sfWithLargeBorder(0, wxBOTTOM).Center());

	return panel;
}

// -----------------------------------------------------------------------------
// Creates a panel containing a read-only, scrollable multiline text box
// -----------------------------------------------------------------------------
wxPanel* createTextTab(wxWindow* parent, string_view text, bool wrap)
{
	auto panel = new wxPanel(parent);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	auto style = wxHW_SCROLLBAR_AUTO;
	if (app::platform() == app::Platform::Windows)
		style |= wxBORDER_SIMPLE;
	auto html = new NoFocusHtmlWindow(panel, -1, style);
	html->SetStandardFonts(-1, wxEmptyString, wxutil::monospaceFont(html->GetFont()).GetFaceName());
	html->SetPage(wxString::FromUTF8(text));

	sizer->Add(html, LayoutHelper(panel).sfWithBorder(1).Expand());

	return panel;
}

// -----------------------------------------------------------------------------
// Creates a panel containing system and build info
// -----------------------------------------------------------------------------
wxPanel* createSystemInfoPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	auto lh       = LayoutHelper(panel);
	auto gb_sizer = new wxGridBagSizer(lh.padSmall(), lh.padLarge());
	sizer->AddStretchSpacer(1);
	sizer->Add(gb_sizer, lh.sfWithLargeBorder(0).Center());
	sizer->AddStretchSpacer(1);

	auto row     = 0;
	auto add_row = [&](string_view label, string_view value, int& row)
	{
		gb_sizer->Add(
			new wxStaticText(panel, -1, wxString::FromUTF8(label)), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		gb_sizer->Add(
			new wxStaticText(panel, -1, wxString::FromUTF8(value)), { row, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		row++;
	};

	auto header_label = [&](string_view text)
	{
		auto label = new wxStaticText(panel, -1, wxString::FromUTF8(text));
		label->SetFont(label->GetFont().Bold());
		return label;
	};

	// System info
	auto gl_info = gl::sysInfo();
	gb_sizer->Add(header_label("System Information"), { row++, 0 }, { 1, 2 }, wxALIGN_CENTER_VERTICAL);
	add_row("Operating System", app::osDescription(), row);
	add_row("Graphics Vendor", gl_info.vendor, row);
	add_row("Graphics Hardware", gl_info.renderer, row);
	add_row("OpenGL Version", gl_info.version, row);
	add_row("", "", row);

	// Build info
	gb_sizer->Add(header_label("Build Information"), { row++, 0 }, { 1, 2 }, wxALIGN_CENTER_VERTICAL);
	add_row("Build Date", __DATE__, row);
	if (app::platform() == app::Platform::Windows)
		add_row("Architecture", app::isWin64Build() ? "x64" : "x86 (32bit)", row);
	add_row("wxWidgets Version", fmt::format("v{}.{}.{}", wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER), row);
	add_row("SFML Version", fmt::format("v{}.{}.{}", SFML_VERSION_MAJOR, SFML_VERSION_MINOR, SFML_VERSION_PATCH), row);
#ifndef NO_LUA
	add_row("Lua Version", fmt::format("v{}.{}.{}", LUA_VERSION_MAJOR, LUA_VERSION_MINOR, LUA_VERSION_RELEASE), row);
#endif

	gb_sizer->AddGrowableCol(1, 1);

	return panel;
}
} // namespace


// -----------------------------------------------------------------------------
//
// AboutDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// AboutDialog class constructor
// -----------------------------------------------------------------------------
AboutDialog::AboutDialog(wxWindow* parent) : SDialog(parent, "About SLADE", "about")
{
	auto lh = LayoutHelper(this);

	wxutil::setWindowIcon(this, "logo");

	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(hbox);

	// Left - logo panel (full height)
	hbox->Add(createLogoPanel(this), wxSizerFlags(0).Expand());

	// Right - tabs and close button
	auto vbox = new wxBoxSizer(wxVERTICAL);
	hbox->Add(vbox, wxSizerFlags(1).Expand());

	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createSystemInfoPanel(tabs), wxS("System Info"), true);
	tabs->AddPage(createTextTab(tabs, contributorsHtml(), true), wxS("Additional Credits"));
	tabs->AddPage(new ReleaseNotesPanel(tabs), wxS("Release Notes"));
	vbox->Add(tabs, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxTOP).Expand());
	vbox->AddSpacer(lh.pad());

	// Close button
	auto btn_close = new wxButton(this, wxID_OK, wxS("Close"));
	vbox->Add(btn_close, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Right());
	btn_close->SetDefault();
	SetAffirmativeId(wxID_OK);
	SetEscapeId(wxID_OK);
	btn_close->SetFocus();

	SetInitialSize(lh.size(840, 500));
	Layout();
	CenterOnParent();
}
