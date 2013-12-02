
#include "Main.h"
#include "WxStuff.h"
#include "ResourceArchiveChooser.h"
#include "ArchiveManager.h"
#include "SFileDialog.h"
#include "SplashWindow.h"

ResourceArchiveChooser::ResourceArchiveChooser(wxWindow* parent, Archive* archive) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Resource archive list
	list_resources = new wxCheckListBox(this, -1);
	sizer->Add(list_resources, 1, wxEXPAND|wxBOTTOM, 4);
	list_resources->SetInitialSize(wxSize(350, 100));

	// Populate resource archive list
	int index = 0;
	for (int a = 0; a < theArchiveManager->numArchives(); a++)
	{
		Archive* arch = theArchiveManager->getArchive(a);
		if (arch != archive)
		{
			list_resources->Append(arch->getFilename(false));
			archives.push_back(arch);
			if (theArchiveManager->archiveIsResource(arch))
				list_resources->Check(index);
			index++;
		}
	}

	// 'Open Resource' button
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxRIGHT, 4);
	btn_open_resource = new wxButton(this, -1, "Open Archive");
	hbox->Add(btn_open_resource, 0, wxEXPAND|wxRIGHT, 4);

	// 'Open Recent' button
	btn_recent = new wxButton(this, -1, "Open Recent");
	hbox->Add(btn_recent, 0, wxEXPAND, 0);

	// Bind events
	btn_open_resource->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ResourceArchiveChooser::onBtnOpenResource, this);
	btn_recent->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ResourceArchiveChooser::onBtnRecent, this);

	Layout();
}

ResourceArchiveChooser::~ResourceArchiveChooser()
{
}

vector<Archive*> ResourceArchiveChooser::getSelectedResourceArchives()
{
	wxArrayInt checked;
	vector<Archive*> list;
	list_resources->GetCheckedItems(checked);
	for (unsigned a = 0; a < checked.size(); a++)
		list.push_back(archives[checked[a]]);
	return list;
}

string ResourceArchiveChooser::getSelectedResourceList()
{
	vector<Archive*> selected = getSelectedResourceArchives();
	string ret;
	for (unsigned a = 0; a < selected.size(); a++)
		ret += S_FMT("\"%s\" ", CHR(selected[a]->getFilename()));
	return ret;
}

void ResourceArchiveChooser::onBtnOpenResource(wxCommandEvent& e)
{
	SFileDialog::fd_info_t info;
	if (SFileDialog::openFile(info, "Open Resource Archive", theArchiveManager->getArchiveExtensionsString(), this))
	{
		theSplashWindow->show("Opening Resource Archive", true);
		Archive* na = theArchiveManager->openArchive(info.filenames[0], true, true);
		theSplashWindow->hide();
		if (na)
		{
			list_resources->Append(na->getFilename(false));
			list_resources->Check(list_resources->GetCount()-1);
			archives.push_back(na);
		}
	}
}

void ResourceArchiveChooser::onBtnRecent(wxCommandEvent& e)
{
	// Build list of recent wad filename strings
	wxArrayString recent;
	for (unsigned a = 0; a < theArchiveManager->numRecentFiles(); a++)
		recent.Add(theArchiveManager->recentFile(a));

	// Show dialog
	wxSingleChoiceDialog dlg(this, "Select a recent Archive to open", "Open Recent", recent);
	if (dlg.ShowModal() == wxID_OK)
	{
		Archive* na = theArchiveManager->openArchive(theArchiveManager->recentFile(dlg.GetSelection()), true, true);
		if (na)
		{
			list_resources->Append(na->getFilename(false));
			list_resources->Check(list_resources->GetCount()-1);
			archives.push_back(na);
		}
	}
}
