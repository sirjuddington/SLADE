
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SetupWizardDialog.cpp
// Description: Setup wizard dialog that is shown on the first run to set up
//              important editing preferences and settings
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "BaseResourceWizardPage.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "NodeBuildersWizardPage.h"
#include "SetupWizardDialog.h"
#include "TempFolderWizardPage.h"


// ----------------------------------------------------------------------------
//
// SetupWizardDialog Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SetupWizardDialog::SetupWizardDialog
//
// SetupWizardDialog class constructor
// ----------------------------------------------------------------------------
SetupWizardDialog::SetupWizardDialog(wxWindow* parent) :
	wxDialog(
		parent,
		-1,
		"First Time SLADE Setup",
		wxDefaultPosition,
		wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	// Create pages
	pages.push_back(new TempFolderWizardPage(this));
	pages.push_back(new BaseResourceWizardPage(this));
	pages.push_back(new NodeBuildersWizardPage(this));
	current_page = 0;

	// Hide all pages
	for (unsigned a = 0; a < pages.size(); a++)
		pages[a]->Show(false);

	// Init layout
	setupLayout();

	// Set icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "logo"));
	SetIcon(icon);

	// Setup layout
	SetInitialSize(wxSize(UI::scalePx(600), UI::scalePx(500)));
	Layout();
	Fit();
	SetMinSize(GetBestSize());
	CenterOnParent();

	showPage(0);

	// Bind events
	btn_next->Bind(wxEVT_BUTTON, &SetupWizardDialog::onBtnNext, this);
	btn_prev->Bind(wxEVT_BUTTON, &SetupWizardDialog::onBtnPrev, this);
}

// ----------------------------------------------------------------------------
// SetupWizardDialog::~SetupWizardDialog
//
// SetupWizardDialog class destructor
// ----------------------------------------------------------------------------
SetupWizardDialog::~SetupWizardDialog()
{
}

// ----------------------------------------------------------------------------
// SetupWizardDialog::setupLayout
//
// Sets up the dialog layout
// ----------------------------------------------------------------------------
void SetupWizardDialog::setupLayout()
{
	auto pad_xl = UI::scalePx(16);

	// Setup main sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Page title
	label_page_title = new wxStaticText(
		this,
		-1,
		pages[0]->getTitle(),
		wxDefaultPosition,
		wxDefaultSize,
		wxST_NO_AUTORESIZE
	);
	label_page_title->SetFont(label_page_title->GetFont().MakeLarger().MakeBold());
	sizer->Add(label_page_title, 0, wxEXPAND|wxALL, pad_xl);

	// Page description
	label_page_description = new wxStaticText(this, -1, "");
	sizer->Add(label_page_description, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, pad_xl);

	// Main page area
	sizer->Add(pages[0], 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, pad_xl);

	// Bottom buttons
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->AddStretchSpacer();
	sizer->Add(hbox, 0, wxEXPAND|wxALL, pad_xl);

	// Previous button
	btn_prev = new wxButton(this, -1, "Previous");
	hbox->Add(btn_prev, 0, wxEXPAND|wxRIGHT, UI::pad());

	// Next button
	btn_next = new wxButton(this, -1, "Next");
	hbox->Add(btn_next, 0, wxEXPAND);

	btn_prev->Enable(false);
}

// ----------------------------------------------------------------------------
// SetupWizardDialog::showPage
//
// Shows the wizard page at [index]
// ----------------------------------------------------------------------------
void SetupWizardDialog::showPage(unsigned index)
{
	// Check index
	if (index >= pages.size())
		return;

	// Swap pages
	pages[current_page]->Show(false);
	GetSizer()->Replace(pages[current_page], pages[index]);
	pages[index]->Show(true);
	current_page = index;

	// Check for last page
	if (index == pages.size() - 1)
		btn_next->SetLabel("Finish");
	else
		btn_next->SetLabel("Next");

	// Check for first page
	if (index == 0)
		btn_prev->Enable(false);
	else
		btn_prev->Enable(true);

	// Update title
	label_page_title->SetLabel(pages[index]->getTitle());

	// Update description
	label_page_description->SetLabel(pages[index]->getDescription());
	label_page_description->Wrap(label_page_title->GetSize().x);

	Layout();
	Update();
	Refresh();
}


// ----------------------------------------------------------------------------
//
// SetupWizardDialog Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SetupWizardDialog::onBtnNext
//
// Called when the 'Next' button is clicked
// ----------------------------------------------------------------------------
void SetupWizardDialog::onBtnNext(wxCommandEvent& e)
{
	if (pages[current_page]->canGoNext())
	{
		pages[current_page]->applyChanges();

		// Close if last page
		if (current_page == pages.size() - 1)
		{
			EndModal(wxID_OK);
			return;
		}

		showPage(current_page + 1);
	}
}

// ----------------------------------------------------------------------------
// SetupWizardDialog::onBtnPrev
//
// Called when the 'Previous' button is clicked
// ----------------------------------------------------------------------------
void SetupWizardDialog::onBtnPrev(wxCommandEvent& e)
{
	showPage(current_page - 1);
}
