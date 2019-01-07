
// -----------------------------------------------------------------------------
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "SetupWizardDialog.h"
#include "BaseResourceWizardPage.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "NodeBuildersWizardPage.h"
#include "TempFolderWizardPage.h"


// -----------------------------------------------------------------------------
//
// SetupWizardDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SetupWizardDialog class constructor
// -----------------------------------------------------------------------------
SetupWizardDialog::SetupWizardDialog(wxWindow* parent) :
	wxDialog(
		parent,
		-1,
		"First Time SLADE Setup",
		wxDefaultPosition,
		wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// Create pages
	// pages_.push_back(new TempFolderWizardPage(this));
	pages_.push_back(new BaseResourceWizardPage(this));
	pages_.push_back(new NodeBuildersWizardPage(this));
	current_page_ = 0;

	// Hide all pages
	for (auto& page : pages_)
		page->Show(false);

	// Init layout
	setupLayout();

	// Set icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::General, "logo"));
	SetIcon(icon);

	// Setup layout
	SetInitialSize(wxSize(UI::scalePx(600), UI::scalePx(500)));
	wxWindowBase::Layout();
	wxWindowBase::Fit();
	wxTopLevelWindowBase::SetMinSize(GetBestSize());
	CenterOnParent();

	showPage(0);

	// Bind events
	btn_next_->Bind(wxEVT_BUTTON, &SetupWizardDialog::onBtnNext, this);
	btn_prev_->Bind(wxEVT_BUTTON, &SetupWizardDialog::onBtnPrev, this);
}

// -----------------------------------------------------------------------------
// Sets up the dialog layout
// -----------------------------------------------------------------------------
void SetupWizardDialog::setupLayout()
{
	auto pad_xl = UI::scalePx(16);

	// Setup main sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Page title
	label_page_title_ = new wxStaticText(
		this, -1, pages_[0]->title(), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	label_page_title_->SetFont(label_page_title_->GetFont().MakeLarger().MakeBold());
	sizer->Add(label_page_title_, 0, wxEXPAND | wxALL, pad_xl);

	// Page description
	label_page_description_ = new wxStaticText(this, -1, "");
	sizer->Add(label_page_description_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, pad_xl);

	// Main page area
	sizer->Add(pages_[0], 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, pad_xl);

	// Bottom buttons
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->AddStretchSpacer();
	sizer->Add(hbox, 0, wxEXPAND | wxALL, pad_xl);

	// Previous button
	btn_prev_ = new wxButton(this, -1, "Previous");
	hbox->Add(btn_prev_, 0, wxEXPAND | wxRIGHT, UI::pad());

	// Next button
	btn_next_ = new wxButton(this, -1, "Next");
	hbox->Add(btn_next_, 0, wxEXPAND);

	btn_prev_->Enable(false);
}

// -----------------------------------------------------------------------------
// Shows the wizard page at [index]
// -----------------------------------------------------------------------------
void SetupWizardDialog::showPage(unsigned index)
{
	// Check index
	if (index >= pages_.size())
		return;

	// Swap pages
	pages_[current_page_]->Show(false);
	GetSizer()->Replace(pages_[current_page_], pages_[index]);
	pages_[index]->Show(true);
	current_page_ = index;

	// Check for last page
	if (index == pages_.size() - 1)
		btn_next_->SetLabel("Finish");
	else
		btn_next_->SetLabel("Next");

	// Check for first page
	if (index == 0)
		btn_prev_->Enable(false);
	else
		btn_prev_->Enable(true);

	// Update title
	label_page_title_->SetLabel(pages_[index]->title());

	// Update description
	label_page_description_->SetLabel(pages_[index]->description());
	label_page_description_->Wrap(label_page_title_->GetSize().x);

	Layout();
	Update();
	Refresh();
}


// -----------------------------------------------------------------------------
//
// SetupWizardDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Next' button is clicked
// -----------------------------------------------------------------------------
void SetupWizardDialog::onBtnNext(wxCommandEvent& e)
{
	if (pages_[current_page_]->canGoNext())
	{
		pages_[current_page_]->applyChanges();

		// Close if last page
		if (current_page_ == pages_.size() - 1)
		{
			EndModal(wxID_OK);
			return;
		}

		showPage(current_page_ + 1);
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Previous' button is clicked
// -----------------------------------------------------------------------------
void SetupWizardDialog::onBtnPrev(wxCommandEvent& e)
{
	showPage(current_page_ - 1);
}
