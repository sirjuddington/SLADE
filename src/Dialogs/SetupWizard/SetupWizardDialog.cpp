
#include "Main.h"
#include "WxStuff.h"
#include "SetupWizardDialog.h"
#include "TempFolderWizardPage.h"
#include "BaseResourceWizardPage.h"
#include "NodeBuildersWizardPage.h"
#include "Icons.h"

SetupWizardDialog::SetupWizardDialog(wxWindow* parent) : wxDialog(parent, -1, "First Time SLADE Setup", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
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
	icon.CopyFromBitmap(getIcon("i_logo"));
	SetIcon(icon);

	// Setup layout
	SetInitialSize(wxSize(600, 500));
	Layout();
	Fit();
	SetMinSize(GetBestSize());
	CenterOnParent();

	showPage(0);

	// Bind events
	btn_next->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SetupWizardDialog::onBtnNext, this);
	btn_prev->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SetupWizardDialog::onBtnPrev, this);
}

SetupWizardDialog::~SetupWizardDialog()
{
}

void SetupWizardDialog::setupLayout()
{
	// Setup main sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Page title
	label_page_title = new wxStaticText(this, -1, pages[0]->getTitle(), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	label_page_title->SetFont(label_page_title->GetFont().MakeLarger().MakeBold());
	sizer->Add(label_page_title, 0, wxEXPAND|wxALL, 16);

	// Page description
	label_page_description = new wxStaticText(this, -1, "");
	sizer->Add(label_page_description, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 16);

	// Main page area
	sizer->Add(pages[0], 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 16);

	// Bottom buttons
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->AddStretchSpacer();
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 16);

	// Previous button
	btn_prev = new wxButton(this, -1, "Previous");
	hbox->Add(btn_prev, 0, wxEXPAND|wxRIGHT, 4);

	// Next button
	btn_next = new wxButton(this, -1, "Next");
	hbox->Add(btn_next, 0, wxEXPAND);

	btn_prev->Enable(false);
}

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

void SetupWizardDialog::onBtnPrev(wxCommandEvent& e)
{
	showPage(current_page - 1);
}

void SetupWizardDialog::showPage(unsigned index)
{
	// Check index
	if (index < 0 || index >= pages.size())
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
