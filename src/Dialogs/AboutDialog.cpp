
#include "Main.h"
#include "AboutDialog.h"
#include "Archive/ArchiveManager.h"
#include <wx/generic/statbmpg.h>
#include "App.h"


namespace
{
	wxBoxSizer* createSladeBox(wxWindow* parent)
	{
		auto sizer = new wxBoxSizer(wxHORIZONTAL);

		// SLADE
		auto label_slade = new wxStaticText(parent, -1, "SLADE");
		wxFont font = label_slade->GetFont();
		font.MakeBold();
		font.SetPointSize(20);
		label_slade->SetFont(font);
		sizer->Add(label_slade, 0);

		// Version
		sizer->Add(new wxStaticText(parent, -1, Global::version), 0, wxLEFT|wxALIGN_BOTTOM, 10);

		return sizer;
	}
}

AboutDialog::AboutDialog(wxWindow* parent) : wxDialog(parent, -1, "About SLADE")
{
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Export/load logo image
	string path = App::path("icon.png", App::Dir::Temp);
	theArchiveManager->programResourceArchive()->entryAtPath("icon.png")->exportFile(path);
	logo_bitmap_.LoadFile(path, wxBITMAP_TYPE_PNG);

	// Logo
	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 0, wxEXPAND|wxALL, 10);
	auto logo = new wxGenericStaticBitmap(this, -1, logo_bitmap_);
	vbox->Add(logo, 0, wxEXPAND);

	/*auto label_slade = new wxStaticText(this, -1, "SLADE");
	wxFont font = label_slade->GetFont();
	font.MakeBold();
	font.SetPointSize(20);
	label_slade->SetFont(font);*/
	vbox->Add(createSladeBox(this), 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 10);

	SetMinClientSize(sizer->GetMinSize());
}
