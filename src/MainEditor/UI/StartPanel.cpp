
#include "Main.h"
#include "StartPanel.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/SAction.h"
#include "General/UI.h"
#include "UI/WxUtils.h"
#include <wx/statbmp.h>

using namespace slade;
using namespace ui;

namespace
{
const string background_colour = "#1f242e";
const string foreground_colour = "#d5d7dd";
const string link_colour       = "#ffcc66";
const string blue_dark_colour  = "#496499";
const string blue_light_colour = "#507CD6";
} // namespace

namespace
{
wxBitmapBundle getIconBitmapBundle(string_view icon, int size)
{
	auto svg_entry = app::archiveManager().programResourceArchive()->entryAtPath(fmt::format("icons/{}", icon));
	return wxBitmapBundle::FromSVG(reinterpret_cast<const char*>(svg_entry->rawData()), wxutil::scaledSize(size, size));
}

wxWindow* createActionButton(wxWindow* parent, string_view icon, const string& text)
{
	auto button = new wxButton(parent, -1, text, wxDefaultPosition, wxutil::scaledSize(-1, 40), wxBU_LEFT);
	button->SetBitmap(getIconBitmapBundle(icon, 20));
	button->SetBitmapMargins(ui::pad(), 0);

	return button;
}

wxSizer* createLogoSizer(wxWindow* parent)
{
	auto sizer = new wxBoxSizer(wxHORIZONTAL);

	// Logo
	auto logo_bitmap = new wxStaticBitmap(parent, -1, getIconBitmapBundle("general/logo.svg", 112));
	sizer->Add(logo_bitmap, wxSizerFlags(1).CenterVertical().Border(wxRIGHT, ui::padLarge()));

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 1, wxEXPAND);

	vbox->AddStretchSpacer();

	// SLADE Label
	auto slade_label = new wxStaticText(parent, -1, "SLADE");
	slade_label->SetFont(slade_label->GetFont().Bold().Scale(4.0f));
	slade_label->SetForegroundColour(wxColour(blue_light_colour));
	vbox->Add(slade_label, wxSizerFlags().Left());

	// "It's a Doom Editor"
	auto tagline_label = new wxStaticText(parent, -1, "It's a Doom Editor");
	tagline_label->SetFont(tagline_label->GetFont().Bold().Italic().Scale(1.2f));
	tagline_label->SetForegroundColour(wxColour(blue_dark_colour));
	vbox->Add(tagline_label, wxSizerFlags().CenterHorizontal().Border(wxBOTTOM, ui::pad()));

	vbox->AddStretchSpacer();

	return sizer;
}

wxSizer* createActionsSizer(wxWindow* parent)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);

	auto title_label = new wxStaticText(parent, -1, "Get Started");
	title_label->SetFont(title_label->GetFont().Bold().Scale(1.5f));
	sizer->Add(title_label, wxSizerFlags().Expand().Border(wxBOTTOM, ui::pad()));

	auto open_button       = createActionButton(parent, "general/open.svg", "Open Archive");
	auto opendir_button    = createActionButton(parent, "general/opendir.svg", "Open Directory");
	auto newarchive_button = createActionButton(parent, "general/newarchive.svg", "Create New Archive");
	auto newmap_button     = createActionButton(parent, "general/mapeditor.svg", "Create New Map");

	open_button->Bind(wxEVT_BUTTON, [](wxCommandEvent&) { SActionHandler::doAction("aman_open"); });
	opendir_button->Bind(wxEVT_BUTTON, [](wxCommandEvent&) { SActionHandler::doAction("aman_opendir"); });
	newarchive_button->Bind(wxEVT_BUTTON, [](wxCommandEvent&) { SActionHandler::doAction("aman_newarchive"); });
	newmap_button->Bind(wxEVT_BUTTON, [](wxCommandEvent&) { SActionHandler::doAction("aman_newmap"); });

	sizer->Add(open_button, wxSizerFlags().Expand().Border(wxBOTTOM, ui::pad()));
	sizer->Add(opendir_button, wxSizerFlags().Expand().Border(wxBOTTOM, ui::pad()));
	sizer->Add(newarchive_button, wxSizerFlags().Expand().Border(wxBOTTOM, ui::pad()));
	sizer->Add(newmap_button, wxSizerFlags().Expand());

	return sizer;
}
} // namespace

StartPanel::StartPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	wxPanel::SetName("startpage");

	wxWindowBase::SetBackgroundColour(wxColour(background_colour));
	wxWindowBase::SetForegroundColour(wxColour(foreground_colour));

	// Setup Recent Files panel
	recent_files_panel_      = new wxPanel(this);
	sc_recent_files_updated_ = app::archiveManager().signals().recent_files_changed.connect_scoped(
		[this] { updateRecentFilesPanel(); }); // Update panel when recent files list changes
	recent_files_panel_->SetBackgroundColour(wxColour(background_colour));
	recent_files_panel_->SetForegroundColour(wxColour(foreground_colour));
	updateRecentFilesPanel();

	setupLayout();
}

void StartPanel::setupLayout()
{
	auto main_sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(main_sizer);

	// Blue strip at the top
	auto top_panel = new wxPanel(this, -1, wxDefaultPosition, { -1, 4 });
	top_panel->SetBackgroundColour(wxColour(116, 135, 175));
	main_sizer->Add(top_panel, wxSizerFlags(0).Expand());

	// Left side (logo + actions)
	auto left_sizer = new wxBoxSizer(wxVERTICAL);
	left_sizer->Add(createLogoSizer(this), wxSizerFlags(0));
	left_sizer->Add(createActionsSizer(this), wxSizerFlags(1).Expand().Border(wxTOP, ui::pad()));

	// We need this so we can right-align left_sizer within content_sizer
	auto left_alignment_sizer = new wxBoxSizer(wxVERTICAL);
	left_alignment_sizer->Add(left_sizer, wxSizerFlags(1).Right());

	auto content_sizer = new wxBoxSizer(wxHORIZONTAL);
	content_sizer->Add(left_alignment_sizer, wxSizerFlags(1).Border(wxRIGHT, ui::padLarge() * 2));
	content_sizer->Add(recent_files_panel_, wxSizerFlags(1).CenterVertical().Border(wxLEFT, ui::padLarge() * 2));

	main_sizer->AddStretchSpacer();
	main_sizer->Add(content_sizer, wxSizerFlags(1).Center().Border(wxLEFT | wxRIGHT, ui::pad()));
	main_sizer->AddStretchSpacer();

	// Bottom (version)
	auto bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
	bottom_sizer->AddStretchSpacer();
	auto version_label = new wxStaticText(this, -1, "v" + app::version().toString());
	version_label->SetFont(version_label->GetFont().Bold());
	bottom_sizer->Add(version_label, wxSizerFlags().CenterVertical());

	main_sizer->Add(bottom_sizer, wxSizerFlags().Expand().Border(wxRIGHT, ui::pad()));
	main_sizer->AddSpacer(ui::padMin());
}

void StartPanel::updateRecentFilesPanel()
{
	auto sizer = recent_files_panel_->GetSizer();
	if (!sizer)
	{
		sizer = new wxBoxSizer(wxVERTICAL);
		recent_files_panel_->SetSizer(sizer);
	}

	sizer->Clear(true);

	auto title_label = new wxStaticText(recent_files_panel_, -1, "Recent Files");
	title_label->SetFont(title_label->GetFont().Bold().Scale(1.5f));
	sizer->Add(title_label, wxSizerFlags().Expand().Border(wxBOTTOM, ui::pad()));

	auto recent_files = app::archiveManager().recentFiles();
	if (recent_files.empty())
	{
		auto no_recent_label = new wxStaticText(recent_files_panel_, -1, "No recently opened files");
		no_recent_label->SetFont(no_recent_label->GetFont().Scale(1.2f).Italic());
		sizer->Add(no_recent_label);
	}
	else
	{
		auto index = 0;
		for (const auto& path : recent_files)
		{
			sizer->Add(createRecentFileSizer(path, index), wxSizerFlags().Border(wxBOTTOM, ui::padMin()));

			if (index++ > 13)
				break;
		}
	}

	Layout();
}

wxSizer* StartPanel::createRecentFileSizer(string_view full_path, int index) const
{
	auto sizer = new wxBoxSizer(wxHORIZONTAL);

	// Icon --------------------------------------------------------------------
	auto   path = strutil::Path(full_path);
	string icon = "entry_list/archive.svg";

	// Dir
	if (!path.hasExtension())
		icon = "entry_list/folder.svg";

	// Wad
	static auto wad_fmt = Archive::formatFromId("wad");
	for (const auto& fmt_ext : wad_fmt->extensions)
		if (strutil::equalCI(path.extension(), fmt_ext.first))
		{
			icon = "entry_list/wad.svg";
			break;
		}

	// Zip
	static auto zip_fmt = Archive::formatFromId("zip");
	for (const auto& fmt_ext : zip_fmt->extensions)
		if (strutil::equalCI(path.extension(), fmt_ext.first))
		{
			icon = "entry_list/zip.svg";
			break;
		}

	sizer->Add(
		new wxStaticBitmap(recent_files_panel_, -1, getIconBitmapBundle(icon, 16)),
		wxSizerFlags().Border(wxRIGHT, ui::pad()));


	// Text --------------------------------------------------------------------
	auto filename_label = new wxStaticText(recent_files_panel_, -1, wxutil::strFromView(path.fileName()));
	filename_label->SetFont(filename_label->GetFont().Bold());
	filename_label->SetForegroundColour(wxColour(link_colour));
	filename_label->SetCursor(wxCURSOR_HAND);
	filename_label->SetDoubleBuffered(true);
	sizer->Add(filename_label, wxSizerFlags().Bottom().Border(wxRIGHT, ui::padLarge()));

	auto path_label = new wxStaticText(recent_files_panel_, -1, wxutil::strFromView(path.path(false)));
	sizer->Add(path_label, wxSizerFlags().Bottom());

	// Open on filename click
	filename_label->Bind(
		wxEVT_LEFT_DOWN,
		[this, index](wxMouseEvent&)
		{
			SActionHandler::setWxIdOffset(index);
			SActionHandler::doAction("aman_recent");
		});

	// Underline filename on mouseover
	filename_label->Bind(
		wxEVT_IDLE,
		[filename_label](wxIdleEvent&)
		{
			auto font = filename_label->GetFont();
			auto mouseover = filename_label->GetScreenRect().Contains(wxGetMousePosition());
			if (!mouseover && font.GetUnderlined())
			{
				font.SetUnderlined(false);
				filename_label->SetFont(font);
				filename_label->Refresh();
			}
			else if (mouseover && !font.GetUnderlined())
			{
				font.SetUnderlined(true);
				filename_label->SetFont(font);
				filename_label->Refresh();
			}
		});
	// filename_label->Bind(
	// 	wxEVT_ENTER_WINDOW,
	// 	[filename_label](wxMouseEvent&)
	// 	{
	// 		filename_label->SetFont(filename_label->GetFont().Underlined());
	// 		filename_label->Refresh();
	// 	});
	// filename_label->Bind(
	// 	wxEVT_LEAVE_WINDOW,
	// 	[filename_label](wxMouseEvent&)
	// 	{
	// 		auto font = filename_label->GetFont();
	// 		font.SetUnderlined(false);
	// 		filename_label->SetFont(font);
	// 		filename_label->Refresh();
	// 	});

	return sizer;
}
