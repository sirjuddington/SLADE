
#include "Main.h"
#include "AboutDialog.h"
#include "Archive/ArchiveManager.h"
#include <wx/generic/statbmpg.h>
#include "App.h"


AboutDialog::AboutDialog(wxWindow* parent) :
	wxDialog(parent, -1, "About SLADE")
{
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Export/load logo image
	string path = App::path("icon.png", App::Dir::Temp);
	App::archiveManager().programResourceArchive()->entryAtPath("icon.png")->exportFile(path);
	logo_bitmap_.LoadFile(path, wxBITMAP_TYPE_PNG);

	// Logo
	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 0, wxEXPAND|wxALL, 10);
	auto logo = new wxGenericStaticBitmap(this, -1, logo_bitmap_);
	vbox->Add(logo, 0, wxEXPAND);

	// SLADE
	auto label_slade = new wxStaticText(this, -1, "SLADE");
	wxFont font = label_slade->GetFont();
	font.MakeBold();
	font.SetPointSize(32);
	label_slade->SetFont(font);
	vbox->Add(label_slade, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 10);

	// It's a Doom Editor
	auto label_tagline = new wxStaticText(this, -1, "It's a Doom Editor");
	font = label_tagline->GetFont();
	font.MakeItalic();
	font.SetPointSize(14);
	label_tagline->SetFont(font);
	vbox->Add(label_tagline, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 8);

	// Version
	vbox->Add(new wxStaticText(this, -1, S_FMT("v%s", CHR(Global::version))), 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 8);

	// Website
	vbox->Add(new wxStaticText(this, -1, "http://slade.mancubus.net"), 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 8);



	// Developers
	vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 0, wxEXPAND|wxALL, 10);
	vbox->Add(new wxStaticText(this, -1, "Lead Developer: Simon Judd"), 0, wxEXPAND, 0);
	vbox->Add(new wxStaticText(this, -1, "Developer: Gez"), 0, wxEXPAND|wxTOP, 8);

	// Contributors
	vbox->Add(new wxStaticText(this, -1, "With contributions from:"), 0, wxEXPAND|wxTOP, 20);
	auto text_contributors = new wxTextCtrl(this, -1, "stuff", {}, {}, wxTE_MULTILINE|wxTE_READONLY);
	text_contributors->SetInitialSize({ 400, -1 });
	text_contributors->Enable(false);
	vbox->Add(text_contributors, 1, wxEXPAND|wxTOP, 4);

	SetMinClientSize(sizer->GetMinSize());
}
