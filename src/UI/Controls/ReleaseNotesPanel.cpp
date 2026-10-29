
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ReleaseNotesPanel.cpp
// Description: ReleaseNotesPanel class, a panel that shows the release notes
//              for SLADE
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
#include "ReleaseNotesPanel.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "NoFocusHtmlWindow.h"
#include "SIconButton.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"
#include <md4c-html.h>

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
// Returns the release notes for the given [version] as HTML
// (parsed from Markdown in the program resource)
// -----------------------------------------------------------------------------
string releaseNotesHtml(string_view version)
{
	auto notes_entry = app::programResource()->entryAtPath(fmt::format("release_notes/{}.md", version));
	if (!notes_entry)
		return "<p>Release notes not found.</p>";

	auto markdown = fmt::format("# SLADE v{}\n\n{}", version, notes_entry->data().asString());

	string html;
	auto   result = md_html(
        markdown.data(),
        static_cast<MD_SIZE>(markdown.size()),
        [](const MD_CHAR* text, MD_SIZE size, void* userdata) { static_cast<string*>(userdata)->append(text, size); },
        &html,
        MD_DIALECT_GITHUB,
        0);

	if (result != 0)
		return "<p>Failed to parse release notes.</p>";

	return html;
}
} // namespace


// -----------------------------------------------------------------------------
//
// ReleaseNotesPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ReleaseNotesPanel class constructor
// -----------------------------------------------------------------------------
ReleaseNotesPanel::ReleaseNotesPanel(wxWindow* parent) : wxPanel(parent)
{
	SetDoubleBuffered(true);

	LayoutHelper lh(this);

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto style = wxHW_SCROLLBAR_AUTO;
	if (app::platform() == app::Platform::Windows)
		style |= wxBORDER_SIMPLE;
	html_notes_ = new NoFocusHtmlWindow(this, -1, style);
	html_notes_->SetStandardFonts(-1, wxEmptyString, wxutil::monospaceFont(parent->GetFont()).GetFaceName());
	sizer->Add(html_notes_, lh.sfWithBorder(1).Expand());


	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
	choice_version_ = new wxChoice(this, -1);
	hbox->AddStretchSpacer(1);
	hbox->Add(new wxStaticText(this, -1, wxS("Version:")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	hbox->Add(
		btn_prev_ = new SIconButton(this, "left", "", 16, choice_version_->GetSize().y),
		lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	hbox->Add(choice_version_, lh.sfWithBorder(0, wxRIGHT).Expand());
	hbox->Add(
		btn_next_ = new SIconButton(this, "right", "", 16, choice_version_->GetSize().y),
		wxSizerFlags(0).CenterVertical());

	// Populate version dropdown
	if (auto releases_dir = app::programResource()->dirAtPath("release_notes"))
	{
		vector<string> versions;
		for (unsigned a = 0; a < releases_dir->numEntries(); a++)
		{
			auto entry = releases_dir->entryAt(a);
			if (!entry || !entry->name().ends_with(".md"))
				continue;

			string version{ strutil::Path::fileNameOf(entry->name(), false) };

			if (strutil::contains(version, "-beta") && app::version().beta == 0)
				continue; // Don't show beta versions if the current version isn't a beta

			versions.push_back(version);
		}

		// Sort versions in descending order
		std::ranges::sort(
			versions,
			[](const string& a, const string& b)
			{
				app::Version ver_a{ a };
				app::Version ver_b{ b };
				return ver_a.cmp(ver_b) > 0;
			});

		versions[0] = fmt::format("{} (current)", versions[0]);

		for (const auto& version : versions)
			choice_version_->Append(wxString::FromUTF8(strutil::replace(version, "-beta", " Beta ")));

		choice_version_->SetSelection(0);
	}

	// Show the release notes for the selected version when the dropdown selection changes
	choice_version_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { loadSelectedVersion(); });

	// Select and show previous version notes when btn_prev_ is clicked
	btn_prev_->Bind(
		wxEVT_BUTTON,
		[this](wxCommandEvent&)
		{
			int sel = choice_version_->GetSelection();
			if (sel < choice_version_->GetCount() - 1)
				choice_version_->SetSelection(sel + 1);
			loadSelectedVersion();
		});

	// Select and show next version notes when btn_next_ is clicked
	btn_next_->Bind(
		wxEVT_BUTTON,
		[this](wxCommandEvent&)
		{
			int sel = choice_version_->GetSelection();
			if (sel > 0)
				choice_version_->SetSelection(sel - 1);
			loadSelectedVersion();
		});

	loadSelectedVersion();
}

// -----------------------------------------------------------------------------
// Loads the release notes for the currently selected version
// -----------------------------------------------------------------------------
void ReleaseNotesPanel::loadSelectedVersion() const
{
	auto selection = choice_version_->GetStringSelection().utf8_string();
	strutil::replaceIP(selection, " (current)", "");
	strutil::replaceIP(selection, " Beta ", "-beta");

	html_notes_->SetPage(wxString::FromUTF8(releaseNotesHtml(selection)));

	btn_next_->Enable(choice_version_->GetSelection() > 0);
	btn_prev_->Enable(choice_version_->GetSelection() < choice_version_->GetCount() - 1);
}
