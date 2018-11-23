
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapChecksPanel.cpp
// Description: Panel for performing map checks - checks to run are selected by
//              checkboxes, check results are added to a list. When a list item
//              is hilighted the problem is shown on the map view
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
#include "MapChecksPanel.h"
#include "MapEditor/MapChecks.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "Utility/SFileDialog.h"
#include "MapEditor/MapEditContext.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace
{
	vector<std::pair<MapCheck::StandardCheck, string>> std_checks =
	{
		{ MapCheck::MissingTexture, "Check for missing textures" },
		{ MapCheck::SpecialTag, "Check for missing tags" },
		{ MapCheck::IntersectingLine, "Check for intersecting lines" },
		{ MapCheck::OverlappingLine, "Check for overlapping lines" },
		{ MapCheck::OverlappingThing, "Check for unknown wall textures" },
		{ MapCheck::UnknownTexture, "Check for unknown flats" },
		{ MapCheck::UnknownFlat, "Check for unknown thing types" },
		{ MapCheck::UnknownThingType, "Check for overlapping things" },
		{ MapCheck::StuckThing, "Check for stuck things" },
		{ MapCheck::SectorReference, "Check sector references" },
		{ MapCheck::InvalidLine, "Check for invalid lines" },
		{ MapCheck::MissingTagged, "Check for missing tagged objects" },
		{ MapCheck::UnknownSector, "Check for unknown sector types" },
		{ MapCheck::UnknownSpecial, "Check for unknown line and thing specials" },
		{ MapCheck::ObsoleteThing, "Check for obsolete things" },
	};
}


// ----------------------------------------------------------------------------
//
// MapChecksPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// MapChecksPanel::MapChecksPanel
//
// MapChecksPanel class constructor
// ----------------------------------------------------------------------------
MapChecksPanel::MapChecksPanel(wxWindow* parent, SLADEMap* map) : DockPanel{ parent }, map_{ map }
{
	// Create controls
	clb_active_checks_ = new wxCheckListBox(this, -1);
	lb_errors_ = new wxListBox(this, -1);
	btn_edit_object_ = new wxButton(this, -1, "Edit Object Properties");
	btn_fix1_ = new wxButton(this, -1, "(Fix1)");
	btn_fix2_ = new wxButton(this, -1, "(Fix2)");
	label_status_ = new wxStaticText(this, -1, "Click Check to begin");
	btn_export_ = new wxButton(this, -1, "Export Results");
	btn_check_ = new wxButton(this, -1, "Check");

	// Populate checks list
	for (auto& check : std_checks)
		clb_active_checks_->Append(check.second);

	// Bind events
	btn_check_->Bind(wxEVT_BUTTON, &MapChecksPanel::onBtnCheck, this);
	lb_errors_->Bind(wxEVT_LISTBOX, &MapChecksPanel::onListBoxItem, this);
	btn_edit_object_->Bind(wxEVT_BUTTON, &MapChecksPanel::onBtnEditObject, this);
	btn_fix1_->Bind(wxEVT_BUTTON, &MapChecksPanel::onBtnFix1, this);
	btn_fix2_->Bind(wxEVT_BUTTON, &MapChecksPanel::onBtnFix2, this);
	btn_export_->Bind(wxEVT_BUTTON, &MapChecksPanel::onBtnExport, this);

	// Init default selected checks
	for (auto a = 0u; a < std_checks.size(); ++a)
		clb_active_checks_->Check(a);
	clb_active_checks_->Check(MapCheck::SectorReference, false);
	clb_active_checks_->Check(MapCheck::ObsoleteThing, false);

	// Init buttons
	btn_fix1_->Show(false);
	btn_fix2_->Show(false);
	btn_edit_object_->Enable(false);
	btn_export_->Enable(false);
}

// ----------------------------------------------------------------------------
// MapChecksPanel::updateStatusText
//
// Updates the check status label text
// ----------------------------------------------------------------------------
void MapChecksPanel::updateStatusText(string text)
{
	label_status_->SetLabel(text);
	Update();
	Refresh();
}

// ----------------------------------------------------------------------------
// MapChecksPanel::showCheckItem
//
// Shows the selected problem on the map view and sets up fix buttons
// ----------------------------------------------------------------------------
void MapChecksPanel::showCheckItem(unsigned index)
{
	if (index < check_items_.size())
	{
		// Set edit mode to object type
		MapObject* obj = check_items_[index].check->getObject(check_items_[index].index);
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
		btn_edit_object_->Enable(true);

		string fix1 = check_items_[index].check->fixText(0, check_items_[index].index);
		if (fix1 != "")
		{
			// Show first fix button
			btn_fix1_->SetLabel(fix1);
			btn_fix1_->Show(true);
		}
		else
			btn_fix1_->Show(false);

		string fix2 = check_items_[index].check->fixText(1, check_items_[index].index);
		if (fix2 != "")
		{
			// Show second fix button
			btn_fix2_->SetLabel(fix2);
			btn_fix2_->Show(true);
		}
		else
			btn_fix2_->Show(false);
	}
	else
	{
		btn_edit_object_->Enable(false);
		btn_fix1_->Show(false);
		btn_fix2_->Show(false);
	}

	Layout();
}

// ----------------------------------------------------------------------------
// MapChecksPanel::refreshList
//
// Refreshes the problems list
// ----------------------------------------------------------------------------
void MapChecksPanel::refreshList()
{
	int selected = lb_errors_->GetSelection();
	lb_errors_->Clear();
	check_items_.clear();

	for (unsigned a = 0; a < active_checks_.size(); a++)
	{
		for (unsigned b = 0; b < active_checks_[a]->nProblems(); b++)
		{
			lb_errors_->Append(active_checks_[a]->problemDesc(b));
			check_items_.push_back(CheckItem(active_checks_[a], b));
		}
	}

	// Re-select
	if (selected < 0 && lb_errors_->GetCount() > 0)
	{
		lb_errors_->Select(0);
		lb_errors_->EnsureVisible(0);
	}
	else if (selected >= 0 && selected < (int)lb_errors_->GetCount())
	{
		lb_errors_->Select(selected);
		lb_errors_->EnsureVisible(selected);
	}
	else if (selected >= (int)lb_errors_->GetCount() && lb_errors_->GetCount() > 0)
	{
		lb_errors_->Select(lb_errors_->GetCount() - 1);
		lb_errors_->EnsureVisible(lb_errors_->GetCount());
	}
}

// ----------------------------------------------------------------------------
// MapChecksPanel::reset
//
// Resets all map checks and panel controls
// ----------------------------------------------------------------------------
void MapChecksPanel::reset()
{
	// Clear interface
	lb_errors_->Show(false);
	lb_errors_->Clear();
	btn_fix1_->Show(false);
	btn_fix2_->Show(false);
	btn_edit_object_->Enable(false);
	check_items_.clear();

	// Clear previous checks
	for (unsigned a = 0; a < active_checks_.size(); a++)
		delete active_checks_[a];
	active_checks_.clear();

	refreshList();
	lb_errors_->Show(true);
}

// ----------------------------------------------------------------------------
// MapChecksPanel::layoutVertical
//
// Lays out panel controls vertically
// (for when the panel is docked vertically)
// ----------------------------------------------------------------------------
void MapChecksPanel::layoutVertical()
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Checks
	sizer->Add(WxUtils::createLabelVBox(this, "Check for:", clb_active_checks_), 0, wxEXPAND | wxALL, UI::pad());
	sizer->Add(btn_check_, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());

	// Results
	sizer->Add(label_status_, 0, wxEXPAND | wxLEFT | wxRIGHT, UI::pad());
	sizer->AddSpacer(UI::px(UI::Size::PadMinimum));
	sizer->Add(lb_errors_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());

	// Result actions
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());
	hbox->Add(btn_edit_object_, 0, wxEXPAND | wxRIGHT, UI::pad());
	hbox->AddStretchSpacer();
	hbox->Add(btn_export_, 0, wxEXPAND);
	sizer->Add(
		WxUtils::layoutHorizontally(vector<wxObject*>{ btn_fix1_, btn_fix2_ }),
		0,
		wxLEFT | wxRIGHT | wxBOTTOM,
		UI::pad()
	);
}

// ----------------------------------------------------------------------------
// MapChecksPanel::layoutHorizontal
//
// Lays out panel controls horizontally
// (for when the panel is docked horizontally)
// ----------------------------------------------------------------------------
void MapChecksPanel::layoutHorizontal()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	auto sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	GetSizer()->Add(sizer, 1, wxEXPAND | wxALL, UI::pad());

	// Checks
	sizer->Add(new wxStaticText(this, -1, "Check for:"), { 0, 0 }, { 1, 1 }, wxEXPAND);
	sizer->Add(clb_active_checks_, { 1, 0 }, { 1, 1 }, wxEXPAND);
	sizer->Add(btn_check_, { 2, 0 }, { 1, 1 }, wxALIGN_RIGHT);

	// Results
	sizer->Add(label_status_, { 0, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(lb_errors_, { 1, 1 }, { 2, 1 }, wxEXPAND);

	// Result actions
	auto layout = WxUtils::layoutVertically(vector<wxObject*>{ btn_export_, btn_edit_object_, btn_fix1_, btn_fix2_ });
	sizer->Add(layout, { 1, 2 }, { 2, 1 }, wxEXPAND);

	sizer->AddGrowableCol(1, 1);
	sizer->AddGrowableRow(1, 1);
}


// ----------------------------------------------------------------------------
//
// MapChecksPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// MapChecksPanel::onBtnCheck
//
// Called when the 'check' button is clicked
// ----------------------------------------------------------------------------
void MapChecksPanel::onBtnCheck(wxCommandEvent& e)
{
	// Clear interface
	lb_errors_->Show(false);
	lb_errors_->Clear();
	btn_fix1_->Show(false);
	btn_fix2_->Show(false);
	btn_edit_object_->Enable(false);
	btn_export_->Enable(false);
	check_items_.clear();

	// Clear previous checks
	for (unsigned a = 0; a < active_checks_.size(); a++)
		delete active_checks_[a];
	active_checks_.clear();

	// Setup checks
	for (auto a = 0u; a < std_checks.size(); ++a)
	{
		if (clb_active_checks_->IsChecked(a))
		{
			active_checks_.push_back(
				MapCheck::standardCheck(
					(MapCheck::StandardCheck)a,
					map_,
					&MapEditor::textureManager()
				)
			);
		}
	}

	// Run checks
	for (unsigned a = 0; a < active_checks_.size(); a++)
	{
		// Check
		updateStatusText(active_checks_[a]->progressText());
		active_checks_[a]->doCheck();

		// Add results to list
		for (unsigned b = 0; b < active_checks_[a]->nProblems(); b++)
		{
			lb_errors_->Append(active_checks_[a]->problemDesc(b));
			check_items_.push_back(CheckItem(active_checks_[a], b));
		}
	}

	lb_errors_->Show(true);

	if (lb_errors_->GetCount() > 0)
	{
		updateStatusText(S_FMT("%d problems found", lb_errors_->GetCount()));
		btn_export_->Enable(true);
	}
	else
		updateStatusText("No problems found");
}

// ----------------------------------------------------------------------------
// MapChecksPanel::onListBoxItem
//
// Called when a list item is selected
// ----------------------------------------------------------------------------
void MapChecksPanel::onListBoxItem(wxCommandEvent& e)
{
	int selected = lb_errors_->GetSelection();
	if (selected >= 0 && selected < (int)check_items_.size())
		showCheckItem(selected);
}

// ----------------------------------------------------------------------------
// MapChecksPanel::onBtnFix1
//
// Called when the first fix button is clicked
// ----------------------------------------------------------------------------
void MapChecksPanel::onBtnFix1(wxCommandEvent& e)
{
	int selected = lb_errors_->GetSelection();
	if (selected >= 0 && selected < (int)check_items_.size())
	{
		MapEditor::editContext().beginUndoRecord(btn_fix1_->GetLabel());
		MapEditor::editContext().selection().clear();
		bool fixed = check_items_[selected].check->fixProblem(
			check_items_[selected].index,
			0,
			&MapEditor::editContext()
		);
		MapEditor::editContext().endUndoRecord(fixed);
		if (fixed)
		{
			refreshList();
			showCheckItem(lb_errors_->GetSelection());
		}
	}
}

// ----------------------------------------------------------------------------
// MapChecksPanel::onBtnFix2
//
// Called when the second fix button is clicked
// ----------------------------------------------------------------------------
void MapChecksPanel::onBtnFix2(wxCommandEvent& e)
{
	int selected = lb_errors_->GetSelection();
	if (selected >= 0 && selected < (int)check_items_.size())
	{
		MapEditor::editContext().beginUndoRecord(btn_fix2_->GetLabel());
		MapEditor::editContext().selection().clear();
		bool fixed = check_items_[selected].check->fixProblem(
			check_items_[selected].index,
			1,
			&MapEditor::editContext()
		);
		MapEditor::editContext().endUndoRecord(fixed);
		if (fixed)
		{
			refreshList();
			showCheckItem(lb_errors_->GetSelection());
		}
	}
}

// ----------------------------------------------------------------------------
// MapChecksPanel::onBtnEditObject
//
// Called when the 'edit object' button is clicked
// ----------------------------------------------------------------------------
void MapChecksPanel::onBtnEditObject(wxCommandEvent& e)
{
	int selected = lb_errors_->GetSelection();
	if (selected >= 0 && selected < (int)check_items_.size())
	{
		vector<MapObject*> list;
		list.push_back(check_items_[selected].check->getObject(check_items_[selected].index));
		MapEditor::openMultiObjectProperties(list);
	}
}

// ----------------------------------------------------------------------------
// MapChecksPanel::onBtnExport
//
// Called when the 'Export Results' button is clicked
// ----------------------------------------------------------------------------
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
		string text = S_FMT("%lu problems found in map %s:\n\n", check_items_.size(), CHR(map_name));
		for (unsigned a = 0; a < check_items_.size(); a++)
			text += check_items_[a].check->problemDesc(check_items_[a].index) + "\n";
		wxFile file;
		file.Open(info.filenames[0], wxFile::write);
		if (file.IsOpened())
			file.Write(text);
		file.Close();
	}
}
