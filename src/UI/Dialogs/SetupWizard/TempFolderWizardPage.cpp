
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TempFolderWizardPage.cpp
// Description: Setup wizard page to set up the temporary folder
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
#include "TempFolderWizardPage.h"
#include "App.h"
#include "General/UI.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, temp_location)
EXTERN_CVAR(String, temp_location_custom)


// -----------------------------------------------------------------------------
//
// TempFolderWizardPage Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TempFolderWizardPage class constructor
// -----------------------------------------------------------------------------
TempFolderWizardPage::TempFolderWizardPage(wxWindow* parent) : WizardPageBase(parent)
{
	auto pad_xl = ui::scalePx(16);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	sizer->AddStretchSpacer();

	rb_use_system_ = new wxRadioButton(
		this, -1, wxS("Use system temp folder (Recommended)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	sizer->Add(rb_use_system_, 0, wxEXPAND | wxBOTTOM, pad_xl);

	rb_use_slade_dir_ = new wxRadioButton(this, -1, wxS("Use SLADE installation folder"));
	sizer->Add(rb_use_slade_dir_, 0, wxEXPAND | wxBOTTOM, pad_xl);

	rb_use_custom_dir_ = new wxRadioButton(this, -1, wxS("Use custom folder:"));
	sizer->Add(rb_use_custom_dir_, 0, wxEXPAND | wxBOTTOM, ui::pad());

	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND);
	text_custom_dir_ = new wxTextCtrl(this, -1, wxEmptyString);
	hbox->Add(text_custom_dir_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, ui::pad());
	btn_browse_dir_ = new wxButton(this, -1, wxS("Browse..."));
	hbox->Add(btn_browse_dir_, 0, wxEXPAND);
	text_custom_dir_->Enable(false);
	btn_browse_dir_->Enable(false);

	sizer->AddStretchSpacer();

	rb_use_custom_dir_->Bind(wxEVT_RADIOBUTTON, &TempFolderWizardPage::onRadioButtonChanged, this);
	rb_use_system_->Bind(wxEVT_RADIOBUTTON, &TempFolderWizardPage::onRadioButtonChanged, this);
	rb_use_slade_dir_->Bind(wxEVT_RADIOBUTTON, &TempFolderWizardPage::onRadioButtonChanged, this);
	btn_browse_dir_->Bind(wxEVT_BUTTON, &TempFolderWizardPage::onBtnBrowse, this);
}

// -----------------------------------------------------------------------------
// Returns true if the wizard page is valid
// -----------------------------------------------------------------------------
bool TempFolderWizardPage::canGoNext()
{
#ifdef WIN32
	string sep = "\\";
#else
	string sep = "/";
#endif

	string testfilename;
	if (rb_use_system_->GetValue())
		testfilename = wxStandardPaths::Get()
						   .GetTempDir()
						   .Append(wxString::FromUTF8(sep))
						   .Append(wxS("SLADE3"))
						   .Append(wxString::FromUTF8(sep))
						   .Append(wxS("test.txt"))
						   .utf8_string();
	else if (rb_use_slade_dir_->GetValue())
		testfilename = app::path("test.txt", app::Dir::Executable);
	else
		testfilename = text_custom_dir_->GetValue().utf8_string() + sep + "test.txt";

	SFile test;
	if (test.open(testfilename, SFile::Mode::Write))
	{
		test.close();

		if (fileutil::removeFile(testfilename))
			return true;
	}

	wxMessageBox(
		wxS("The selected folder cannot be written to. Please select a different folder to use."),
		wxEmptyString,
		wxICON_ERROR);

	return false;
}

// -----------------------------------------------------------------------------
// Applies any changes set on the wizard page
// -----------------------------------------------------------------------------
void TempFolderWizardPage::applyChanges()
{
	if (rb_use_system_->GetValue())
		temp_location = 0;
	else if (rb_use_slade_dir_->GetValue())
		temp_location = 1;
	else
	{
		temp_location        = 2;
		temp_location_custom = text_custom_dir_->GetValue().utf8_string();
	}
}

// -----------------------------------------------------------------------------
// Returns the description for the wizard page
// -----------------------------------------------------------------------------
string TempFolderWizardPage::description()
{
	return "Select the temp folder for SLADE to use during various operations. "
		   "Usually the system temp folder will be fine to use, however sometimes "
		   "it will not be write accessable, which can cause problems.";
}


// -----------------------------------------------------------------------------
//
// TempFolderWizardPage Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the radio button selection changes
// -----------------------------------------------------------------------------
void TempFolderWizardPage::onRadioButtonChanged(wxCommandEvent& e)
{
	if (rb_use_custom_dir_->GetValue())
	{
		text_custom_dir_->Enable(true);
		btn_browse_dir_->Enable(true);
	}
	else
	{
		text_custom_dir_->Enable(false);
		btn_browse_dir_->Enable(false);
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Browse' button is clicked
// -----------------------------------------------------------------------------
void TempFolderWizardPage::onBtnBrowse(wxCommandEvent& e)
{
	wxDirDialog dlg(this, wxS("Select a folder to write temp files to"));
	if (dlg.ShowModal() == wxID_OK)
	{
		// Check directory is empty
		wxDir dir;
		dir.Open(dlg.GetPath());
		if (dir.IsOpened())
		{
			if (dir.HasFiles() || dir.HasSubDirs())
			{
				if (wxMessageBox(
						wxS("The selected folder is not empty.\r\n\r\n"
							"All files in this folder will be DELETED when SLADE exits.\r\n"
							"Please make sure there are no important files in the folder."),
						wxS("Warning"),
						wxOK | wxCANCEL | wxICON_WARNING)
					!= wxID_OK)
					return;
			}
		}

		text_custom_dir_->SetValue(dlg.GetPath());
	}
}
