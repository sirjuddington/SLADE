
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
	tabs->AddPage(createTextTab(tabs, wxString::FromUTF8(systemInfo()), false), wxS("System Info"), true);
	tabs->AddPage(createTextTab(tabs, wxString::FromUTF8(contributors()), true), wxS("Additional Credits"));
	vbox->Add(tabs, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxTOP).Expand());
	vbox->AddSpacer(lh.pad());

	// Close button
	auto btn_close = new wxButton(this, wxID_OK, wxS("Close"));
	vbox->Add(btn_close, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Right());
	btn_close->SetDefault();
	SetAffirmativeId(wxID_OK);
	SetEscapeId(wxID_OK);
	btn_close->SetFocus();

	SetInitialSize(lh.size(720, 440));
	Layout();
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// Creates the left panel containing the logo, name, tagline and version
// -----------------------------------------------------------------------------
wxPanel* AboutDialog::createLogoPanel(wxWindow* parent)
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
wxPanel* AboutDialog::createTextTab(wxWindow* parent, const wxString& text, bool wrap)
{
	auto panel = new wxPanel(parent);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	long style = wxTE_MULTILINE | wxTE_READONLY | (wrap ? 0 : wxTE_DONTWRAP);
	auto tc    = new wxTextCtrl(panel, -1, text, wxDefaultPosition, wxDefaultSize, style);
	// tc->SetFont(wxutil::monospaceFont(tc->GetFont()));
	sizer->Add(tc, LayoutHelper(panel).sfWithBorder(1).Expand());

	return panel;
}

// -----------------------------------------------------------------------------
// Returns the contributors list from the program resource archive
// -----------------------------------------------------------------------------
string AboutDialog::contributors() const
{
	auto entry = app::archiveManager().programResourceArchive()->entryAtPath("contributors.txt");
	if (!entry)
		return "!!! Error reading contributors list";

	auto entry_string = entry->data().asString();
	auto contributors = strutil::splitV(entry_string, '\n');

	string contributors_text =
		"Big thanks to all contributors who have helped with the development of SLADE over the years:\n\n";
	for (auto& c : contributors)
	{
		if (c.empty())
			continue;
		contributors_text.append(c);
		contributors_text.append(", ");
	}
	strutil::removeLastIP(contributors_text, 2);

	return contributors_text;
}

// -----------------------------------------------------------------------------
// Returns system information text
// -----------------------------------------------------------------------------
string AboutDialog::systemInfo() const
{
	auto   gl_info = gl::sysInfo();
	string info;

	info += fmt::format("SLADE Version: {}", app::version().toString());
	if (!global::sc_rev.empty())
		info += fmt::format(" ({})", global::sc_rev);
	if (app::platform() == app::Platform::Windows)
		info += app::isWin64Build() ? " (x64)" : " (x86)";
	info += "\n\n";

	info += fmt::format("Operating System: {}\n", app::osDescription());
	info += fmt::format("Graphics Vendor: {}\n", gl_info.vendor);
	info += fmt::format("Graphics Hardware: {}\n", gl_info.renderer);
	info += fmt::format("OpenGL Version: {}\n", gl_info.version);
	info += "\n";

	info += "Built with:\n";
	info += fmt::format("wxWidgets v{}.{}.{}\n", wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER);
	info += fmt::format("SFML v{}.{}.{}\n", SFML_VERSION_MAJOR, SFML_VERSION_MINOR, SFML_VERSION_PATCH);
#ifdef NO_LUA
	info += "Lua scripting support disabled\n";
#else
	info += fmt::format("Lua v{}.{}.{}\n", LUA_VERSION_MAJOR, LUA_VERSION_MINOR, LUA_VERSION_RELEASE);
#endif

	return info;
}
