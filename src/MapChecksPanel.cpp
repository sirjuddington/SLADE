
#include "Main.h"
#include "WxStuff.h"
#include "MapChecksPanel.h"
#include "SLADEMap.h"
#include "MapChecks.h"
#include "GameConfiguration.h"
#include "MapEditorWindow.h"
#include <wx/gbsizer.h>


MapChecksPanel::MapChecksPanel(wxWindow* parent, SLADEMap* map) : wxPanel(parent, -1)
{
	// Init
	this->map = map;

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	sizer->Add(gb_sizer, 0, wxEXPAND|wxALL, 4);

	// Check missing textures
	cb_missing_tex = new wxCheckBox(this, -1, "Check for missing textures");
	gb_sizer->Add(cb_missing_tex, wxGBPosition(0, 0), wxDefaultSpan, wxEXPAND);

	// Check special tags
	cb_special_tags = new wxCheckBox(this, -1, "Check for missing tags");
	gb_sizer->Add(cb_special_tags, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// Check intersecting lines
	cb_intersecting = new wxCheckBox(this, -1, "Check for intersecting lines");
	gb_sizer->Add(cb_intersecting, wxGBPosition(1, 0), wxDefaultSpan, wxEXPAND);

	// Check overlapping lines
	cb_overlapping = new wxCheckBox(this, -1, "Check for overlapping lines");
	gb_sizer->Add(cb_overlapping, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	// Check unknown textures
	cb_unknown_tex = new wxCheckBox(this, -1, "Check for unknown wall textures");
	gb_sizer->Add(cb_unknown_tex, wxGBPosition(2, 0), wxDefaultSpan, wxEXPAND);

	// Check unknown textures
	cb_unknown_flats = new wxCheckBox(this, -1, "Check for unknown flats");
	gb_sizer->Add(cb_unknown_flats, wxGBPosition(2, 1), wxDefaultSpan, wxEXPAND);

	// Check unknown thing types
	cb_unknown_things = new wxCheckBox(this, -1, "Check for unknown thing types");
	gb_sizer->Add(cb_unknown_things, wxGBPosition(3, 0), wxDefaultSpan, wxEXPAND);

	// Check overlapping things
	cb_overlapping_things = new wxCheckBox(this, -1, "Check for overlapping things");
	gb_sizer->Add(cb_overlapping_things, wxGBPosition(3, 1), wxDefaultSpan, wxEXPAND);

	// Check stuck things
	cb_stuck_things = new wxCheckBox(this, -1, "Check for stuck things");
	gb_sizer->Add(cb_stuck_things, wxGBPosition(4, 0), wxDefaultSpan, wxEXPAND);

	// Check sector references
	cb_sector_refs = new wxCheckBox(this, -1, "Check sector references");
	gb_sizer->Add(cb_sector_refs, wxGBPosition(4, 1), wxDefaultSpan, wxEXPAND);

	// Error list
	lb_errors = new wxListBox(this, -1);
	sizer->Add(lb_errors, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Fix buttons
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	btn_edit_object = new wxButton(this, -1, "Edit Object Properties");
	hbox->Add(btn_edit_object, 0, wxEXPAND|wxRIGHT, 4);
	btn_fix1 = new wxButton(this, -1, "(Fix1)");
	hbox->Add(btn_fix1, 0, wxEXPAND|wxRIGHT, 4);
	btn_fix2 = new wxButton(this, -1, "(Fix2)");
	hbox->Add(btn_fix2, 0, wxEXPAND);

	// Status text
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	label_status = new wxStaticText(this, -1, "");
	hbox->Add(label_status, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);

	// Check button
	btn_check = new wxButton(this, -1, "Check");
	hbox->Add(btn_check, 0, wxEXPAND);

	// Bind events
	btn_check->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MapChecksPanel::onBtnCheck, this);
	lb_errors->Bind(wxEVT_LISTBOX, &MapChecksPanel::onListBoxItem, this);
	btn_edit_object->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MapChecksPanel::onBtnEditObject, this);
	btn_fix1->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MapChecksPanel::onBtnFix1, this);
	btn_fix2->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MapChecksPanel::onBtnFix2, this);

	// Check all by default
	cb_missing_tex->SetValue(true);
	cb_special_tags->SetValue(true);
	cb_intersecting->SetValue(true);
	cb_overlapping->SetValue(true);
	cb_unknown_flats->SetValue(true);
	cb_unknown_tex->SetValue(true);
	cb_unknown_things->SetValue(true);
	cb_stuck_things->SetValue(true);
	cb_sector_refs->SetValue(true);

	btn_fix1->Show(false);
	btn_fix2->Show(false);
	btn_edit_object->Enable(false);
}

MapChecksPanel::~MapChecksPanel()
{
}

void MapChecksPanel::updateStatusText(string text)
{
	label_status->SetLabel(text);
	Update();
	Refresh();
}

void MapChecksPanel::showCheckItem(unsigned index)
{
	if (index < check_items.size())
	{
		// Set edit mode to object type
		MapObject* obj = check_items[index].check->getObject(check_items[index].index);
		switch (obj->getObjType())
		{
		case MOBJ_VERTEX:
			theMapEditor->mapEditor().setEditMode(MapEditor::MODE_VERTICES);
			break;
		case MOBJ_LINE:
			theMapEditor->mapEditor().setEditMode(MapEditor::MODE_LINES);
			break;
		case MOBJ_SECTOR:
			theMapEditor->mapEditor().setEditMode(MapEditor::MODE_SECTORS);
			break;
		case MOBJ_THING:
			theMapEditor->mapEditor().setEditMode(MapEditor::MODE_THINGS);
			break;
		default: break;
		}

		// Scroll to object
		theMapEditor->mapEditor().showItem(obj->getIndex());

		// Update UI
		btn_edit_object->Enable(true);

		string fix1 = check_items[index].check->fixText(0, check_items[index].index);
		if (fix1 != "")
		{
			// Show first fix button
			btn_fix1->SetLabel(fix1);
			btn_fix1->Show(true);
		}
		else
			btn_fix1->Show(false);

		string fix2 = check_items[index].check->fixText(1, check_items[index].index);
		if (fix2 != "")
		{
			// Show second fix button
			btn_fix2->SetLabel(fix2);
			btn_fix2->Show(true);
		}
		else
			btn_fix2->Show(false);
	}
	else
	{
		btn_edit_object->Enable(false);
		btn_fix1->Show(false);
		btn_fix2->Show(false);
	}

	Layout();
}

void MapChecksPanel::refreshList()
{
	int selected = lb_errors->GetSelection();
	lb_errors->Clear();
	check_items.clear();

	for (unsigned a = 0; a < active_checks.size(); a++)
	{
		for (unsigned b = 0; b < active_checks[a]->nProblems(); b++)
		{
			lb_errors->Append(active_checks[a]->problemDesc(b));
			check_items.push_back(check_item_t(active_checks[a], b));
		}
	}

	// Re-select
	if (selected < 0 && lb_errors->GetCount() > 0)
	{
		lb_errors->Select(0);
		lb_errors->EnsureVisible(0);
	}
	else if (selected >= 0 && selected < (int)lb_errors->GetCount())
	{
		lb_errors->Select(selected);
		lb_errors->EnsureVisible(selected);
	}
	else if (selected >= (int)lb_errors->GetCount() && lb_errors->GetCount() > 0)
	{
		lb_errors->Select(lb_errors->GetCount() - 1);
		lb_errors->EnsureVisible(lb_errors->GetCount());
	}
}

void MapChecksPanel::reset()
{
	// Clear interface
	lb_errors->Show(false);
	lb_errors->Clear();
	btn_fix1->Show(false);
	btn_fix2->Show(false);
	btn_edit_object->Enable(false);
	check_items.clear();

	// Clear previous checks
	for (unsigned a = 0; a < active_checks.size(); a++)
		delete active_checks[a];
	active_checks.clear();

	refreshList();
	lb_errors->Show(true);
}


void MapChecksPanel::onBtnCheck(wxCommandEvent& e)
{
	MapTextureManager* texman = &(theMapEditor->textureManager());

	// Clear interface
	lb_errors->Show(false);
	lb_errors->Clear();
	btn_fix1->Show(false);
	btn_fix2->Show(false);
	btn_edit_object->Enable(false);
	check_items.clear();

	// Clear previous checks
	for (unsigned a = 0; a < active_checks.size(); a++)
		delete active_checks[a];
	active_checks.clear();

	// Setup checks
	if (cb_missing_tex->GetValue())
		active_checks.push_back(MapCheck::missingTextureCheck(map));
	if (cb_special_tags->GetValue())
		active_checks.push_back(MapCheck::specialTagCheck(map));
	if (cb_intersecting->GetValue())
		active_checks.push_back(MapCheck::intersectingLineCheck(map));
	if (cb_overlapping->GetValue())
		active_checks.push_back(MapCheck::overlappingLineCheck(map));
	if (cb_unknown_tex->GetValue())
		active_checks.push_back(MapCheck::unknownTextureCheck(map, texman));
	if (cb_unknown_flats->GetValue())
		active_checks.push_back(MapCheck::unknownFlatCheck(map, texman));
	if (cb_unknown_things->GetValue())
		active_checks.push_back(MapCheck::unknownThingTypeCheck(map));
	if (cb_overlapping_things->GetValue())
		active_checks.push_back(MapCheck::overlappingThingCheck(map));
	if (cb_stuck_things->GetValue())
		active_checks.push_back(MapCheck::stuckThingsCheck(map));
	if (cb_sector_refs->GetValue())
		active_checks.push_back(MapCheck::sectorReferenceCheck(map));

	// Run checks
	for (unsigned a = 0; a < active_checks.size(); a++)
	{
		// Check
		updateStatusText(active_checks[a]->progressText());
		active_checks[a]->doCheck();

		// Add results to list
		for (unsigned b = 0; b < active_checks[a]->nProblems(); b++)
		{
			lb_errors->Append(active_checks[a]->problemDesc(b));
			check_items.push_back(check_item_t(active_checks[a], b));
		}
	}

	lb_errors->Show(true);

	if (lb_errors->GetCount() > 0)
		updateStatusText(S_FMT("%d problems found", lb_errors->GetCount()));
	else
		updateStatusText("No problems found");
}

void MapChecksPanel::onListBoxItem(wxCommandEvent& e)
{
	int selected = lb_errors->GetSelection();
	if (selected >= 0 && selected < (int)check_items.size())
	{
		showCheckItem(selected);
	}
}

void MapChecksPanel::onBtnFix1(wxCommandEvent& e)
{
	int selected = lb_errors->GetSelection();
	if (selected >= 0 && selected < (int)check_items.size())
	{
		bool fixed = check_items[selected].check->fixProblem(check_items[selected].index, 0, &(theMapEditor->mapEditor()));
		if (fixed)
		{
			refreshList();
			showCheckItem(lb_errors->GetSelection());
		}
	}
}

void MapChecksPanel::onBtnFix2(wxCommandEvent& e)
{
	int selected = lb_errors->GetSelection();
	if (selected >= 0 && selected < (int)check_items.size())
	{
		bool fixed = check_items[selected].check->fixProblem(check_items[selected].index, 1, &(theMapEditor->mapEditor()));
		if (fixed)
		{
			refreshList();
			showCheckItem(lb_errors->GetSelection());
		}
	}
}

void MapChecksPanel::onBtnEditObject(wxCommandEvent& e)
{
	int selected = lb_errors->GetSelection();
	if (selected >= 0 && selected < (int)check_items.size())
	{
		vector<MapObject*> list;
		list.push_back(check_items[selected].check->getObject(check_items[selected].index));
		theMapEditor->editObjectProperties(list);
	}
}
