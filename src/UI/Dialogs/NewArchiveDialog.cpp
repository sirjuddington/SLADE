
#include "Main.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "NewArchiveDiaog.h"
#include "Graphics/Icons.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;

NewArchiveDialog::NewArchiveDialog(wxWindow* parent) : SDialog(parent, "Create New Archive", "new_archive")
{
	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(icons::getIcon(icons::General, "newarchive"));
	SetIcon(icon);
	
	// Create controls
	auto* choice_type = new wxChoice(this, -1);
	auto* btn_create  = new wxButton(this, -1, "Create");
	auto* btn_cancel  = new wxButton(this, -1, "Cancel");

	// Fill formats list
	for (const auto& format : Archive::allFormats())
		if (format.create)
			choice_type->AppendString(format.name + " Archive");

	// Setup controls
	choice_type->SetSelection(0);
	btn_create->SetDefault();

	// Layout
	auto* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
	sizer->Add(choice_type, 0, wxEXPAND | wxALL, ui::padLarge());
	auto* hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->AddStretchSpacer(1);
	hbox->Add(btn_create, 0, wxEXPAND|wxRIGHT, ui::pad());
	hbox->Add(btn_cancel, 0, wxEXPAND);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, ui::padLarge());

	// Create button click
	btn_create->Bind(wxEVT_BUTTON, [this, choice_type](wxCommandEvent&)
	{
		for (const auto& format : Archive::allFormats())
			if (choice_type->GetString(choice_type->GetSelection()) == (format.name + " Archive"))
			{
				archive_created_ = app::archiveManager().newArchive(format.id).get();
				EndModal(wxID_OK);
			}
	});

	// Cancel button click
	btn_cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });

	SetInitialSize({ -1, -1 });
	wxWindowBase::Layout();
	wxWindowBase::Fit();
	wxTopLevelWindowBase::SetMinSize(GetBestSize());
	CenterOnParent();
}

Archive* NewArchiveDialog::createdArchive() const
{
	return archive_created_;
}
