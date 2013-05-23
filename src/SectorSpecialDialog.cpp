
#include "Main.h"
#include "WxStuff.h"
#include "SectorSpecialDialog.h"
#include "GameConfiguration.h"

SectorSpecialDialog::SectorSpecialDialog(wxWindow* parent)
	: wxDialog(parent, -1, "Select Sector Special", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Special list
	wxStaticBox* frame = new wxStaticBox(this, -1, "Special");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	lv_specials = new ListView(this, -1);
	framesizer->Add(lv_specials, 1, wxEXPAND|wxALL, 4);
	sizer->Add(framesizer, 1, wxEXPAND|wxALL, 8);

#if (wxMAJOR_VERSION >= 2 && wxMINOR_VERSION >= 9 && wxRELEASE_NUMBER >= 4)
	lv_specials->AppendColumn("#");
	lv_specials->AppendColumn("Name");
#else
	lv_specials->InsertColumn(lv_specials->GetColumnCount(), "#");
	lv_specials->InsertColumn(lv_specials->GetColumnCount(), "Name");
#endif
	vector<sectype_t> types = theGameConfiguration->allSectorTypes();
	for (unsigned a = 0; a < types.size(); a++)
	{
		wxArrayString item;
		item.Add(S_FMT("%d", types[a].type));
		item.Add(types[a].name);
		lv_specials->addItem(999999, item);
	}

	// Boom Flags
	int width = 300;
	if (theGameConfiguration->isBoom())
	{
		frame = new wxStaticBox(this, -1, "Flags");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		sizer->Add(framesizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 8);

		// Damage
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		framesizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
		string damage_types[] = { "None", "5%", "10%", "20%" };
		choice_damage = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 4, damage_types);
		choice_damage->Select(0);
		hbox->Add(new wxStaticText(this, -1, "Damage:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		hbox->Add(choice_damage, 1, wxEXPAND);

		// Secret
		hbox = new wxBoxSizer(wxHORIZONTAL);
		framesizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
		cb_secret = new wxCheckBox(this, -1, "Secret");
		hbox->Add(cb_secret, 0, wxEXPAND|wxALL, 4);

		// Friction
		cb_friction = new wxCheckBox(this, -1, "Friction Enabled");
		hbox->Add(cb_friction, 0, wxEXPAND|wxALL, 4);

		// Pusher/Puller
		cb_pushpull = new wxCheckBox(this, -1, "Pushers/Pullers Enabled");
		hbox->Add(cb_pushpull, 0, wxEXPAND|wxALL, 4);

		width = -1;
	}

	// Dialog buttons
	sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Bind Events
	lv_specials->Bind(wxEVT_COMMAND_LIST_ITEM_ACTIVATED, &SectorSpecialDialog::onSpecialsListViewItemActivated, this);

	SetInitialSize(wxSize(width, 400));
	CenterOnParent();
}

SectorSpecialDialog::~SectorSpecialDialog()
{
}

void SectorSpecialDialog::setup(int special, int map_format)
{
	int base_type = theGameConfiguration->baseSectorType(special, map_format);

	// Select base type
	vector<sectype_t> types = theGameConfiguration->allSectorTypes();
	for (unsigned a = 0; a < types.size(); a++)
	{
		if (types[a].type == base_type)
		{
			lv_specials->selectItem(a);
			lv_specials->EnsureVisible(a);
			break;
		}
	}

	// Flags
	if (theGameConfiguration->isBoom())
	{
		// Damage
		choice_damage->Select(theGameConfiguration->sectorBoomDamage(special, map_format));

		// Secret
		cb_secret->SetValue(theGameConfiguration->sectorBoomSecret(special, map_format));

		// Friction
		cb_friction->SetValue(theGameConfiguration->sectorBoomFriction(special, map_format));

		// Pusher/Puller
		cb_pushpull->SetValue(theGameConfiguration->sectorBoomPushPull(special, map_format));
	}
}

int SectorSpecialDialog::getSelectedSpecial(int map_format)
{
	vector<sectype_t> types = theGameConfiguration->allSectorTypes();
	int selection = lv_specials->selectedItems()[0];

	// Get selected base type
	int base = 0;
	if (selection < (int)types.size())
		base = types[selection].type;

	if (theGameConfiguration->isBoom())
		return theGameConfiguration->boomSectorType(base, choice_damage->GetSelection(), cb_secret->GetValue(), cb_friction->GetValue(), cb_pushpull->GetValue(), map_format);
	else
		return base;
}


void SectorSpecialDialog::onSpecialsListViewItemActivated(wxListEvent& e)
{
	EndModal(wxID_OK);
}
