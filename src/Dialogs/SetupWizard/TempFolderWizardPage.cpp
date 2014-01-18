
#include "Main.h"
#include "WxStuff.h"
#include "TempFolderWizardPage.h"
#include "SFileDialog.h"
#include <wx/dirdlg.h>
#include <wx/stdpaths.h>

EXTERN_CVAR(Int, temp_location)
EXTERN_CVAR(String, temp_location_custom)

TempFolderWizardPage::TempFolderWizardPage(wxWindow* parent) : WizardPageBase(parent)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	sizer->AddStretchSpacer();

	rb_use_system = new wxRadioButton(this, -1, "Use system temp folder", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
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

TempFolderWizardPage::~TempFolderWizardPage()
{
}

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
		testfilename = appPath("test.txt", DIR_APP);
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

string TempFolderWizardPage::getDescription()
{
	return "Select the temp folder for SLADE to use during various operations. Usually the system temp folder will be fine to use, however sometimes it will not be write accessable, which can cause problems.";
}

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

void TempFolderWizardPage::onBtnBrowse(wxCommandEvent& e)
{
	wxDirDialog dlg(this, "Select a folder to write temp files to");
	if (dlg.ShowModal() == wxID_OK)
	{
		text_custom_dir->SetValue(dlg.GetPath());
	}
}
