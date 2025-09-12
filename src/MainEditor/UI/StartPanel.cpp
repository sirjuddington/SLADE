
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    StartPanel.cpp
// Description: StartPanel class - A simple 'start' page containing buttons for
//              useful actions to do on startup (open archive, etc.) and a list
//              of recently opened archives
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
#include "StartPanel.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormat.h"
#include "Archive/ArchiveManager.h"
#include "Database/Database.h"
#include "Database/Tables/ArchiveFile.h"
#include "General/SActionHandler.h"
#include "UI/Controls/SToolButton.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"
#include <wx/statbmp.h>

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
const wxString BACKGROUND_COLOUR_DARK  = wxS("#1F242E");
const wxString BACKGROUND_COLOUR_LIGHT = wxS("#E0EBFF");
const wxString FOREGROUND_COLOUR_DARK  = wxS("#D5D7DD");
const wxString LINK_COLOUR_DARK        = wxS("#FFCC66");
const wxString LINK_COLOUR_LIGHT       = wxS("#0044CC");
const wxString BLUE_DARK_COLOUR        = wxS("#4D6FB3");
const wxString BLUE_LIGHT_COLOUR       = wxS("#4D83F0");
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns the background colour depending on the current theme
// -----------------------------------------------------------------------------
wxColour backgroundColour()
{
	return { app::isDarkTheme() ? BACKGROUND_COLOUR_DARK : BACKGROUND_COLOUR_LIGHT };
}

// -----------------------------------------------------------------------------
// Returns a wxBitmapBundle of [icon] at base [size]
// -----------------------------------------------------------------------------
wxBitmapBundle getIconBitmapBundle(string_view icon, int size)
{
	auto svg_entry = app::archiveManager().programResourceArchive()->entryAtPath(fmt::format("icons/{}", icon));
	return wxBitmapBundle::FromSVG(reinterpret_cast<const char*>(svg_entry->rawData()), { size, size });
}

// -----------------------------------------------------------------------------
// Creates a custom button (SToolButton) for an action with [text] and [icon]
// -----------------------------------------------------------------------------
SToolButton* createActionButton(wxWindow* parent, const string& action_id, const string& text, const string& icon)
{
	auto button = new SToolButton(parent, action_id, text, icon, "", true, 24);
	button->SetBackgroundColour(backgroundColour());
	button->setExactFit(false);
	button->setFontSize(1.1f);
	button->setPadding(8);
	button->Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, [action_id](wxCommandEvent&) { SActionHandler::doAction(action_id); });
	return button;
}

// -----------------------------------------------------------------------------
// Creates the layout sizer and widgets for the SLADE logo and title
// -----------------------------------------------------------------------------
wxSizer* createLogoSizer(wxWindow* parent)
{
	auto lh    = LayoutHelper(parent);
	auto sizer = new wxBoxSizer(wxHORIZONTAL);

	// Logo
	auto logo_bitmap = new wxStaticBitmap(parent, -1, getIconBitmapBundle("general/logo.svg", 112));
	sizer->Add(logo_bitmap, lh.sfWithLargeBorder(1, wxRIGHT).CenterVertical());

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, wxSizerFlags(1).Expand());

	vbox->AddStretchSpacer();

	// SLADE Label
	auto slade_label = new wxStaticText(parent, -1, wxS("SLADE"));
	slade_label->SetFont(slade_label->GetFont().Bold().Scale(4.0f));
	slade_label->SetForegroundColour(wxColour(app::isDarkTheme() ? BLUE_LIGHT_COLOUR : BLUE_DARK_COLOUR));
	vbox->Add(slade_label, wxSizerFlags().Left());

	// "It's a Doom Editor"
	auto tagline_label = new wxStaticText(parent, -1, wxS("It's a Doom Editor"));
	tagline_label->SetFont(tagline_label->GetFont().Bold().Italic().Scale(1.2f));
	tagline_label->SetForegroundColour(wxColour(app::isDarkTheme() ? BLUE_DARK_COLOUR : BLUE_LIGHT_COLOUR));
	vbox->Add(tagline_label, lh.sfWithBorder(0, wxBOTTOM).CenterHorizontal());

	// Version
	auto version_label = new wxStaticText(parent, -1, wxString::FromUTF8("v" + app::version().toString()));
	version_label->SetFont(version_label->GetFont().Bold());
	version_label->SetForegroundColour(wxColour(app::isDarkTheme() ? BLUE_DARK_COLOUR : BLUE_LIGHT_COLOUR));
	vbox->Add(version_label, wxSizerFlags().Center());

	vbox->AddStretchSpacer();

	return sizer;
}

// -----------------------------------------------------------------------------
// Creates the layout sizer and widgets for the start page action buttons
// -----------------------------------------------------------------------------
wxSizer* createActionsSizer(wxWindow* parent)
{
	auto sizer  = new wxBoxSizer(wxVERTICAL);
	auto sflags = LayoutHelper(parent).sfWithBorder(0, wxBOTTOM).Expand();

	sizer->Add(createActionButton(parent, "aman_open", "Open Archive", "open"), sflags);
	sizer->Add(createActionButton(parent, "aman_opendir", "Open Directory", "opendir"), sflags);
	sizer->Add(createActionButton(parent, "aman_newarchive", "Create New Archive", "newarchive"), sflags);
	sizer->Add(createActionButton(parent, "aman_newmap", "Create New Map", "mapeditor"), sflags);

	return sizer;
}

// -----------------------------------------------------------------------------
// Returns the icon for an archive at [path]
// -----------------------------------------------------------------------------
string getArchiveIcon(const strutil::Path& path)
{
	string icon = "archive";

	// Dir
	if (!path.hasExtension())
		icon = "folder";

	// Wad
	static auto wad_fmt = archive::formatInfo(ArchiveFormat::Wad);
	for (const auto& fmt_ext : wad_fmt.extensions)
		if (strutil::equalCI(path.extension(), fmt_ext.first))
		{
			icon = "wad";
			break;
		}

	// Zip
	static auto zip_fmt = archive::formatInfo(ArchiveFormat::Zip);
	for (const auto& fmt_ext : zip_fmt.extensions)
		if (strutil::equalCI(path.extension(), fmt_ext.first))
		{
			icon = "zip";
			break;
		}

	return icon;
}
} // namespace


// -----------------------------------------------------------------------------
// RecentFileButton Class
//
// A custom SToolButton for recent files, will underline text on mouse over
// instead of changing the background colour
// -----------------------------------------------------------------------------
namespace
{
class RecentFileButton : public SToolButton
{
public:
	RecentFileButton(wxWindow* parent, const strutil::Path& path, int index) :
		SToolButton(parent, "aman_recent", getArchiveIcon(path), true),
		filename_{ path.fileName() }
	{
		action_wx_id_offset_ = index;
		click_can_delete_    = true;

		wxWindow::SetBackgroundColour(backgroundColour());
		wxWindow::SetForegroundColour(wxColour(app::isDarkTheme() ? LINK_COLOUR_DARK : LINK_COLOUR_LIGHT));
		wxWindow::SetCursor(wxCURSOR_HAND);
		setExactFit(true);
		setPadding(FromDIP(1), 0);
		setTextOffset(FromDIP(4));

		// Determine text width
		SetFont(GetFont().MakeBold());
		text_width_ = ToDIP(GetTextExtent(wxString::FromUTF8(filename_)).GetWidth()) + pad_inner_ * 2;

		updateSize();
	}

protected:
	void drawContent(wxGraphicsContext* gc, bool mouse_over) override
	{
		// Get system colours needed
		auto col_background = GetBackgroundColour();
		auto col_hilight    = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

		// Get size of text
		auto name_size   = GetTextExtent(wxString::FromUTF8(filename_));
		auto name_height = name_size.y;

		// Draw icon
		if (auto icon = icon_.GetBitmapFor(this); icon.IsOk())
		{
			// Draw normal icon
			gc->DrawBitmap(
				icon,
				FromDIP(pad_outer_ + pad_inner_),
				FromDIP(pad_outer_ + pad_inner_),
				FromPhys(icon.GetWidth()),
				FromPhys(icon.GetHeight()));
		}

		// Draw text
		if (mouse_over)
			gc->SetFont(GetFont().MakeUnderlined(), GetForegroundColour());
		int top  = (static_cast<double>(GetSize().y) * 0.5) - (static_cast<double>(name_height) * 0.5);
		int left = pad_outer_ + pad_inner_ * 2 + icon_size_ + text_offset_;
		gc->DrawText(wxString::FromUTF8(filename_), FromDIP(left), top);
	}

private:
	string filename_;
};
} // namespace


// -----------------------------------------------------------------------------
//
// StartPanel Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// StartPanel class  constructor
// -----------------------------------------------------------------------------
StartPanel::StartPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	const wxString wxs_black = wxS("#000000");

	wxPanel::SetName(wxS("startpage"));

	wxWindow::SetDoubleBuffered(true);
	wxWindowBase::SetBackgroundColour(backgroundColour());
	wxWindowBase::SetForegroundColour(wxColour(app::isDarkTheme() ? FOREGROUND_COLOUR_DARK : wxs_black));

	// Setup Recent Files panel
	recent_files_panel_      = new wxPanel(this);
	sc_recent_files_updated_ = database::signals().archive_file_updated.connect_scoped(
		[this] { updateRecentFilesPanel(); }); // Update panel when recent files list changes
	recent_files_panel_->SetBackgroundColour(backgroundColour());
	recent_files_panel_->SetForegroundColour(wxColour(app::isDarkTheme() ? FOREGROUND_COLOUR_DARK : wxs_black));
	updateRecentFilesPanel();

	setupLayout();
}

// -----------------------------------------------------------------------------
// Sets up the start panel layout
// -----------------------------------------------------------------------------
void StartPanel::setupLayout()
{
	auto lh = LayoutHelper(this);

	auto main_sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(main_sizer);

	// Blue strip at the top
	auto top_panel = new wxPanel(this, -1, wxDefaultPosition, lh.size(-1, 4));
	top_panel->SetBackgroundColour(wxColour(116, 135, 175));
	main_sizer->Add(top_panel, wxSizerFlags().Expand());

	// Left side (logo + actions)
	auto left_sizer = new wxBoxSizer(wxVERTICAL);
	left_sizer->Add(createActionsSizer(this), wxSizerFlags(1).Right());

	auto content_sizer = new wxBoxSizer(wxHORIZONTAL);
	content_sizer->Add(left_sizer, lh.sfWithLargeBorder(1, wxRIGHT).CenterVertical());
	content_sizer->Add(recent_files_panel_, lh.sfWithLargeBorder(1, wxLEFT).CenterVertical());

	main_sizer->AddStretchSpacer();
	main_sizer->Add(createLogoSizer(this), lh.sfWithLargeBorder(0, wxBOTTOM).Center());
	main_sizer->Add(content_sizer, lh.sfWithBorder(1, wxLEFT | wxRIGHT).Center());
	main_sizer->AddStretchSpacer();
}

// -----------------------------------------------------------------------------
// Updates and refreshes the recent files panel
// -----------------------------------------------------------------------------
void StartPanel::updateRecentFilesPanel()
{
	auto lh    = LayoutHelper(recent_files_panel_);
	auto sizer = recent_files_panel_->GetSizer();
	if (!sizer)
	{
		sizer = new wxBoxSizer(wxVERTICAL);
		recent_files_panel_->SetSizer(sizer);
	}

	sizer->Clear(true);

	auto title_label = new wxStaticText(recent_files_panel_, -1, wxS("Recent Files"));
	title_label->SetFont(title_label->GetFont().Bold().Scale(1.25f));
	sizer->Add(title_label, lh.sfWithBorder(0, wxBOTTOM).Expand());

	auto recent_files = database::recentFiles();
	if (recent_files.empty())
	{
		auto no_recent_label = new wxStaticText(recent_files_panel_, -1, wxS("No recently opened files"));
		no_recent_label->SetFont(no_recent_label->GetFont().Scale(1.2f).Italic());
		sizer->Add(no_recent_label);
	}
	else
	{
		auto index = 0;
		for (const auto& path : recent_files)
		{
			sizer->Add(createRecentFileSizer(path, index), wxSizerFlags());

			if (index++ > 10)
				break;
		}
	}

	Layout();
}

// -----------------------------------------------------------------------------
// Creates the layout sizer and widgets for a recent file at [full_path]
// -----------------------------------------------------------------------------
wxSizer* StartPanel::createRecentFileSizer(string_view full_path, int index) const
{
	auto lh    = LayoutHelper(recent_files_panel_);
	auto sizer = new wxBoxSizer(wxHORIZONTAL);

	// File button
	auto path   = strutil::Path(full_path);
	auto button = new RecentFileButton(recent_files_panel_, path, index);
	sizer->Add(button, lh.sfWithLargeBorder(0, wxRIGHT));

	// Path label
	auto path_label = new wxStaticText(recent_files_panel_, -1, wxutil::strFromView(path.path(false)));
	sizer->Add(path_label, wxSizerFlags().CenterVertical());

	return sizer;
}
