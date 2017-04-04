
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TempFolderWizardPage.cpp
 * Description: Setup wizard page to set up the temporary folder
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "App.h"
#include "TempFolderWizardPage.h"
#include "Utility/SFileDialog.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, temp_location)
EXTERN_CVAR(String, temp_location_custom)


/*******************************************************************
 * TEMPFOLDERWIZARDPAGE CLASS FUNCTIONS
 *******************************************************************/

/* TempFolderWizardPage::TempFolderWizardPage
 * TempFolderWizardPage class constructor
 *******************************************************************/
TempFolderWizardPage::TempFolderWizardPage(wxWindow* parent) : WizardPageBase(parent)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	sizer->AddStretchSpacer();

	rb_use_system = new wxRadioButton(this, -1, "Use system temp folder (Recommended)", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	sizer->Add(rb_use_system, 0, wxEXPAND|wxBOTTOM, 16);

	rb_use_slade_dir = new wxRadioButton(this, -1, "Use SLADE installation folder");
	sizer->Add(rb_use_slade_dir, 0, wxEXPAND|wxBOTTOM, 16);

	rb_use_custom_dir = new wxRadioButton(this, -1, "Use custom folder:");
	sizer->Add(rb_use_custom_dir, 0, wxEXPAND|wxBOTTOM, 4);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND);
	text_custom_dir = new wxTextCtrl(this, -1, "");
	hbox->Add(text_custom_dir, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	btn_browse_dir = new wxButton(this, -1, "Browse...");
	hbox->Add(btn_browse_dir, 0, wxEXPAND);
	text_custom_dir->Enable(false);
	btn_browse_dir->Enable(false);

	sizer->AddStretchSpacer();

	rb_use_custom_dir->Bind(wxEVT_RADIOBUTTON, &TempFolderWizardPage::onRadioButtonChanged, this);
	rb_use_system->Bind(wxEVT_RADIOBUTTON, &TempFolderWizardPage::onRadioButtonChanged, this);
	rb_use_slade_dir->Bind(wxEVT_RADIOBUTTON, &TempFolderWizardPage::onRadioButtonChanged, this);
	btn_browse_dir->Bind(wxEVT_BUTTON, &TempFolderWizardPage::onBtnBrowse, this);
}

/* TempFolderWizardPage::~TempFolderWizardPage
 * TempFolderWizardPage class destructor
 *******************************************************************/
TempFolderWizardPage::~TempFolderWizardPage()
{
}

/* TempFolderWizardPage::canGoNext
 * Returns true if the wizard page is valid
 *******************************************************************/
bool TempFolderWizardPage::canGoNext()
{
#ifdef WIN32
	string sep = "\\";
#else
	string sep = "/";
#endif

	string testfilename;
	if (rb_use_system->GetValue())
		testfilename = wxStandardPaths::Get().GetTempDir().Append(sep).Append("SLADE3").Append(sep).Append("test.txt");
	else if (rb_use_slade_dir->GetValue())
		testfilename = App::path("test.txt", App::Dir::Executable);
	else
		testfilename = text_custom_dir->GetValue() + sep + "test.txt";

	wxFile test;
	if (test.Open(testfilename, wxFile::write))
	{
		test.Close();

		if (wxRemoveFile(testfilename))
			return true;
	}

	wxMessageBox("The selected folder cannot be written to. Please select a different folder to use.", "", wxICON_ERROR);
	return false;
}

/* TempFolderWizardPage::applyChanges
 * Applies any changes set on the wizard page
 *******************************************************************/
void TempFolderWizardPage::applyChanges()
{
	if (rb_use_system->GetValue())
		temp_location = 0;
	else if (rb_use_slade_dir->GetValue())
		temp_location = 1;
	else
	{
		temp_location = 2;
		temp_location_custom = text_custom_dir->GetValue();
	}
}

/* TempFolderWizardPage::getDescription
 * Returns the description for the wizard page
 *******************************************************************/
string TempFolderWizardPage::getDescription()
{
	return "Select the temp folder for SLADE to use during various operations. "
		"Usually the system temp folder will be fine to use, however sometimes "
		"it will not be write accessable, which can cause problems.";
}


/*******************************************************************
 * TEMPFOLDERWIZARDPAGE CLASS EVENTS
 *******************************************************************/

/* TempFolderWizardPage::onRadioButtonChanged
 * Called when the radio button selection changes
 *******************************************************************/
void TempFolderWizardPage::onRadioButtonChanged(wxCommandEvent& e)
{
	if (rb_use_custom_dir->GetValue())
	{
		text_custom_dir->Enable(true);
		btn_browse_dir->Enable(true);
	}
	else
	{
		text_custom_dir->Enable(false);
		btn_browse_dir->Enable(false);
	}
}

/* TempFolderWizardPage::onBtnBrowse
 * Called when the 'Browse' button is clicked
 *******************************************************************/
void TempFolderWizardPage::onBtnBrowse(wxCommandEvent& e)
{
	wxDirDialog dlg(this, "Select a folder to write temp files to");
	if (dlg.ShowModal() == wxID_OK)
	{
		// Check directory is empty
		wxDir dir;
		dir.Open(dlg.GetPath());
		if (dir.IsOpened())
		{
			if (dir.HasFiles() || dir.HasSubDirs())
			{
				if (wxMessageBox("The selected folder is not empty.\r\n\r\n"
					"All files in this folder will be DELETED when SLADE exits.\r\n"
					"Please make sure there are no important files in the folder.",
					"Warning",
					wxOK | wxCANCEL | wxICON_WARNING) != wxID_OK)
					return;
			}
		}

		text_custom_dir->SetValue(dlg.GetPath());
	}
}
