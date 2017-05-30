
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapChecksPanel.cpp
 * Description: Panel for performing map checks - checks to run are
 *              selected by checkboxes, check results are added to
 *              a list. When a list item is hilighted the problem is
 *              shown on the map view
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
#include "MapChecksPanel.h"
#include "Game/Configuration.h"
#include "MapEditor/MapChecks.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "Utility/SFileDialog.h"
#include "MapEditor/MapEditContext.h"


/*******************************************************************
 * MAPCHECKSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* MapChecksPanel::MapChecksPanel
 * MapChecksPanel class constructor
 *******************************************************************/
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

	// Check for invalid lines
	cb_invalid_lines = new wxCheckBox(this, -1, "Check for invalid lines");
	gb_sizer->Add(cb_invalid_lines, wxGBPosition(5, 0), wxDefaultSpan, wxEXPAND);

	// Check tagged objects
	cb_tagged_objects = new wxCheckBox(this, -1, "Check for missing tagged objects");
	gb_sizer->Add(cb_tagged_objects, wxGBPosition(5, 1), wxDefaultSpan, wxEXPAND);

	// Check special types
	cb_unknown_special = new wxCheckBox(this, -1, "Check for unknown line and thing specials");
	gb_sizer->Add(cb_unknown_special, wxGBPosition(6, 0), wxDefaultSpan, wxEXPAND);

	// Check special types
	cb_unknown_sector = new wxCheckBox(this, -1, "Check for unknown sector types");
	gb_sizer->Add(cb_unknown_sector, wxGBPosition(6, 1), wxDefaultSpan, wxEXPAND);

	// Check obsolete things
	cb_obsolete_things = new wxCheckBox(this, -1, "Check for obsolete things");
	gb_sizer->Add(cb_obsolete_things, wxGBPosition(7, 0), wxDefaultSpan, wxEXPAND);

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

	// Export button
	btn_export = new wxButton(this, -1, "Export Results");
	hbox->Add(btn_export, 0, wxEXPAND | wxRIGHT, 4);

	// Check button
	btn_check = new wxButton(this, -1, "Check");
	hbox->Add(btn_check, 0, wxEXPAND);

	// Bind events
	btn_check->Bind(wxEVT_BUTTON, &MapChecksPanel::onBtnCheck, this);
	lb_errors->Bind(wxEVT_LISTBOX, &MapChecksPanel::onListBoxItem, this);
	btn_edit_object->Bind(wxEVT_BUTTON, &MapChecksPanel::onBtnEditObject, this);
	btn_fix1->Bind(wxEVT_BUTTON, &MapChecksPanel::onBtnFix1, this);
	btn_fix2->Bind(wxEVT_BUTTON, &MapChecksPanel::onBtnFix2, this);
	btn_export->Bind(wxEVT_BUTTON, &MapChecksPanel::onBtnExport, this);

	// Check all by default
	cb_missing_tex->SetValue(true);
	cb_special_tags->SetValue(true);
	cb_intersecting->SetValue(true);
	cb_overlapping->SetValue(true);
	cb_unknown_flats->SetValue(true);
	cb_unknown_tex->SetValue(true);
	cb_unknown_things->SetValue(true);
	cb_stuck_things->SetValue(true);
	cb_invalid_lines->SetValue(true);
	//cb_sector_refs->SetValue(true);
	cb_tagged_objects->SetValue(true);
	cb_unknown_special->SetValue(true);
	cb_unknown_sector->SetValue(true);
	//cb_obsolete_things->SetValue(true);

	btn_fix1->Show(false);
	btn_fix2->Show(false);
	btn_edit_object->Enable(false);
	btn_export->Enable(false);
}

/* MapChecksPanel::~MapChecksPanel
 * MapChecksPanel class destructor
 *******************************************************************/
MapChecksPanel::~MapChecksPanel()
{
}

/* MapChecksPanel::updateStatusText
 * Updates the check status label text
 *******************************************************************/
void MapChecksPanel::updateStatusText(string text)
{
	label_status->SetLabel(text);
	Update();
	Refresh();
}

/* MapChecksPanel::showCheckItem
 * Shows the selected problem on the map view and sets up fix buttons
 *******************************************************************/
void MapChecksPanel::showCheckItem(unsigned index)
{
	if (index < check_items.size())
	{
		// Set edit mode to object type
		MapObject* obj = check_items[index].check->getObject(check_items[index].index);
		switch (obj->getObjType())
		{
		case MOBJ_VERTEX:
			MapEditor::editContext().setEditMode(MapEditor::Mode::Vertices);
			break;
		case MOBJ_LINE:
			MapEditor::editContext().setEditMode(MapEditor::Mode::Lines);
			break;
		case MOBJ_SECTOR:
			MapEditor::editContext().setEditMode(MapEditor::Mode::Sectors);
			break;
		case MOBJ_THING:
			MapEditor::editContext().setEditMode(MapEditor::Mode::Things);
			break;
		default: break;
		}

		// Scroll to object
		MapEditor::editContext().showItem(obj->getIndex());

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

/* MapChecksPanel::refreshList
 * Refreshes the problems list
 *******************************************************************/
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

/* MapChecksPanel::reset
 * Resets all map checks and panel controls
 *******************************************************************/
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


/*******************************************************************
 * MAPCHECKSPANEL CLASS EVENTS
 *******************************************************************/

/* MapChecksPanel::onBtnCheck
 * Called when the 'check' button is clicked
 *******************************************************************/
void MapChecksPanel::onBtnCheck(wxCommandEvent& e)
{
	// Clear interface
	lb_errors->Show(false);
	lb_errors->Clear();
	btn_fix1->Show(false);
	btn_fix2->Show(false);
	btn_edit_object->Enable(false);
	btn_export->Enable(false);
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
		active_checks.push_back(MapCheck::unknownTextureCheck(map, &MapEditor::textureManager()));
	if (cb_unknown_flats->GetValue())
		active_checks.push_back(MapCheck::unknownFlatCheck(map, &MapEditor::textureManager()));
	if (cb_unknown_things->GetValue())
		active_checks.push_back(MapCheck::unknownThingTypeCheck(map));
	if (cb_overlapping_things->GetValue())
		active_checks.push_back(MapCheck::overlappingThingCheck(map));
	if (cb_stuck_things->GetValue())
		active_checks.push_back(MapCheck::stuckThingsCheck(map));
	if (cb_sector_refs->GetValue())
		active_checks.push_back(MapCheck::sectorReferenceCheck(map));
	if (cb_invalid_lines->GetValue())
		active_checks.push_back(MapCheck::invalidLineCheck(map));
	if (cb_tagged_objects->GetValue())
		active_checks.push_back(MapCheck::missingTaggedCheck(map));
	if (cb_unknown_special->GetValue())
		active_checks.push_back(MapCheck::unknownSpecialCheck(map));
	if (cb_unknown_sector->GetValue())
		active_checks.push_back(MapCheck::unknownSectorCheck(map));
	if (cb_obsolete_things->GetValue())
		active_checks.push_back(MapCheck::obsoleteThingCheck(map));

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
	{
		updateStatusText(S_FMT("%d problems found", lb_errors->GetCount()));
		btn_export->Enable(true);
	}
	else
		updateStatusText("No problems found");
}

/* MapChecksPanel::onListBoxItem
 * Called when a list item is selected
 *******************************************************************/
void MapChecksPanel::onListBoxItem(wxCommandEvent& e)
{
	int selected = lb_errors->GetSelection();
	if (selected >= 0 && selected < (int)check_items.size())
	{
		showCheckItem(selected);
	}
}

/* MapChecksPanel::onBtnFix1
 * Called when the first fix button is clicked
 *******************************************************************/
void MapChecksPanel::onBtnFix1(wxCommandEvent& e)
{
	int selected = lb_errors->GetSelection();
	if (selected >= 0 && selected < (int)check_items.size())
	{
		MapEditor::editContext().beginUndoRecord(btn_fix1->GetLabel());
		MapEditor::editContext().selection().clear();
		bool fixed = check_items[selected].check->fixProblem(check_items[selected].index, 0, &(MapEditor::editContext()));
		MapEditor::editContext().endUndoRecord(fixed);
		if (fixed)
		{
			refreshList();
			showCheckItem(lb_errors->GetSelection());
		}
	}
}

/* MapChecksPanel::onBtnFix2
 * Called when the second fix button is clicked
 *******************************************************************/
void MapChecksPanel::onBtnFix2(wxCommandEvent& e)
{
	int selected = lb_errors->GetSelection();
	if (selected >= 0 && selected < (int)check_items.size())
	{
		MapEditor::editContext().beginUndoRecord(btn_fix2->GetLabel());
		MapEditor::editContext().selection().clear();
		bool fixed = check_items[selected].check->fixProblem(check_items[selected].index, 1, &(MapEditor::editContext()));
		MapEditor::editContext().endUndoRecord(fixed);
		if (fixed)
		{
			refreshList();
			showCheckItem(lb_errors->GetSelection());
		}
	}
}

/* MapChecksPanel::onBtnEditObject
 * Called when the 'edit object' button is clicked
 *******************************************************************/
void MapChecksPanel::onBtnEditObject(wxCommandEvent& e)
{
	int selected = lb_errors->GetSelection();
	if (selected >= 0 && selected < (int)check_items.size())
	{
		vector<MapObject*> list;
		list.push_back(check_items[selected].check->getObject(check_items[selected].index));
		MapEditor::openMultiObjectProperties(list);
	}
}

/* MapChecksPanel::onBtnExport
 * Called when the 'Export Results' button is clicked
 *******************************************************************/
void MapChecksPanel::onBtnExport(wxCommandEvent& e)
{
	string map_name = MapEditor::editContext().mapDesc().name;
	SFileDialog::fd_info_t info;
	if (SFileDialog::saveFile(
		info,
		"Export Map Check Results",
		"Text Files (*.txt)|*.txt",
		MapEditor::windowWx(), map_name + "-Problems"))
	{
		string text = S_FMT("%d problems found in map %s:\n\n", check_items.size(), CHR(map_name));
		for (unsigned a = 0; a < check_items.size(); a++)
			text += check_items[a].check->problemDesc(check_items[a].index) + "\n";
		wxFile file;
		file.Open(info.filenames[0], wxFile::write);
		if (file.IsOpened())
			file.Write(text);
		file.Close();
	}
}
