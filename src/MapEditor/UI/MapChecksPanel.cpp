
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "MapChecksPanel.h"
#include "General/UI.h"
#include "MapEditor/MapChecks.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "SLADEMap/MapObject/MapObject.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
vector<std::pair<MapCheck::StandardCheck, wxString>> std_checks = {
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


// -----------------------------------------------------------------------------
//
// MapChecksPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapChecksPanel class constructor
// -----------------------------------------------------------------------------
MapChecksPanel::MapChecksPanel(wxWindow* parent, SLADEMap* map) : DockPanel{ parent }, map_{ map }
{
	// Create controls
	clb_active_checks_ = new wxCheckListBox(this, -1);
	lb_errors_         = new wxListBox(this, -1);
	btn_edit_object_   = new wxButton(this, -1, "Edit Object Properties");
	btn_fix1_          = new wxButton(this, -1, "(Fix1)");
	btn_fix2_          = new wxButton(this, -1, "(Fix2)");
	label_status_      = new wxStaticText(this, -1, "Click Check to begin");
	btn_export_        = new wxButton(this, -1, "Export Results");
	btn_check_         = new wxButton(this, -1, "Check");

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

// -----------------------------------------------------------------------------
// MapChecksPanel class destructor
// -----------------------------------------------------------------------------
MapChecksPanel::~MapChecksPanel() = default;

// -----------------------------------------------------------------------------
// Updates the check status label text
// -----------------------------------------------------------------------------
void MapChecksPanel::updateStatusText(const wxString& text)
{
	label_status_->SetLabel(text);
	Update();
	Refresh();
}

// -----------------------------------------------------------------------------
// Shows the selected problem on the map view and sets up fix buttons
// -----------------------------------------------------------------------------
void MapChecksPanel::showCheckItem(unsigned index)
{
	if (index < check_items_.size())
	{
		// Set edit mode to object type
		auto obj = check_items_[index].check->getObject(check_items_[index].index);
		switch (obj->objType())
		{
		case MapObject::Type::Vertex: mapeditor::editContext().setEditMode(mapeditor::Mode::Vertices); break;
		case MapObject::Type::Line:   mapeditor::editContext().setEditMode(mapeditor::Mode::Lines); break;
		case MapObject::Type::Sector: mapeditor::editContext().setEditMode(mapeditor::Mode::Sectors); break;
		case MapObject::Type::Thing:  mapeditor::editContext().setEditMode(mapeditor::Mode::Things); break;
		default:                      break;
		}

		// Scroll to object
		mapeditor::editContext().showItem(obj->index());

		// Update UI
		btn_edit_object_->Enable(true);

		wxString fix1 = check_items_[index].check->fixText(0, check_items_[index].index);
		if (!fix1.empty())
		{
			// Show first fix button
			btn_fix1_->SetLabel(fix1);
			btn_fix1_->Show(true);
		}
		else
			btn_fix1_->Show(false);

		wxString fix2 = check_items_[index].check->fixText(1, check_items_[index].index);
		if (!fix2.empty())
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

// -----------------------------------------------------------------------------
// Refreshes the problems list
// -----------------------------------------------------------------------------
void MapChecksPanel::refreshList()
{
	int selected = lb_errors_->GetSelection();
	lb_errors_->Clear();
	check_items_.clear();

	for (auto& check : active_checks_)
	{
		for (unsigned b = 0; b < check->nProblems(); b++)
		{
			lb_errors_->Append(check->problemDesc(b));
			check_items_.emplace_back(check.get(), b);
		}
	}

	// Re-select
	if (selected < 0 && lb_errors_->GetCount() > 0)
	{
		lb_errors_->Select(0);
		lb_errors_->EnsureVisible(0);
	}
	else if (selected >= 0 && selected < static_cast<int>(lb_errors_->GetCount()))
	{
		lb_errors_->Select(selected);
		lb_errors_->EnsureVisible(selected);
	}
	else if (selected >= static_cast<int>(lb_errors_->GetCount()) && lb_errors_->GetCount() > 0)
	{
		lb_errors_->Select(lb_errors_->GetCount() - 1);
		lb_errors_->EnsureVisible(lb_errors_->GetCount());
	}
}

// -----------------------------------------------------------------------------
// Resets all map checks and panel controls
// -----------------------------------------------------------------------------
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
	active_checks_.clear();

	refreshList();
	lb_errors_->Show(true);
}

// -----------------------------------------------------------------------------
// Lays out panel controls vertically
// (for when the panel is docked vertically)
// -----------------------------------------------------------------------------
void MapChecksPanel::layoutVertical()
{
	namespace wx = wxutil;

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Checks
	sizer->Add(wx::createLabelVBox(this, "Check for:", clb_active_checks_), wx::sfWithBorder().Expand());
	sizer->Add(btn_check_, wx::sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Right());

	// Results
	sizer->Add(label_status_, wx::sfWithBorder(0, wxLEFT | wxRIGHT).Expand());
	sizer->AddSpacer(ui::px(ui::Size::PadMinimum));
	sizer->Add(lb_errors_, wx::sfWithBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Result actions
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wx::sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
	hbox->Add(btn_edit_object_, wx::sfWithBorder(0, wxRIGHT).Expand());
	hbox->AddStretchSpacer();
	hbox->Add(btn_export_, wxSizerFlags().Expand());
	sizer->Add(
		wx::layoutHorizontally(vector<wxObject*>{ btn_fix1_, btn_fix2_ }),
		wx::sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM));
}

// -----------------------------------------------------------------------------
// Lays out panel controls horizontally
// (for when the panel is docked horizontally)
// -----------------------------------------------------------------------------
void MapChecksPanel::layoutHorizontal()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	auto sizer = new wxGridBagSizer(ui::pad(), ui::pad());
	GetSizer()->Add(sizer, wxutil::sfWithBorder(1).Expand());

	// Checks
	sizer->Add(new wxStaticText(this, -1, "Check for:"), { 0, 0 }, { 1, 1 }, wxEXPAND);
	sizer->Add(clb_active_checks_, { 1, 0 }, { 1, 1 }, wxEXPAND);
	sizer->Add(btn_check_, { 2, 0 }, { 1, 1 }, wxALIGN_RIGHT);

	// Results
	sizer->Add(label_status_, { 0, 1 }, { 1, 1 }, wxEXPAND);
	sizer->Add(lb_errors_, { 1, 1 }, { 2, 1 }, wxEXPAND);

	// Result actions
	auto layout = wxutil::layoutVertically(vector<wxObject*>{ btn_export_, btn_edit_object_, btn_fix1_, btn_fix2_ });
	sizer->Add(layout, { 1, 2 }, { 2, 1 }, wxEXPAND);

	sizer->AddGrowableCol(1, 1);
	sizer->AddGrowableRow(1, 1);
}


// -----------------------------------------------------------------------------
//
// MapChecksPanel Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the 'check' button is clicked
// -----------------------------------------------------------------------------
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
	active_checks_.clear();

	// Setup checks
	for (auto a = 0u; a < std_checks.size(); ++a)
	{
		if (clb_active_checks_->IsChecked(a))
		{
			active_checks_.emplace_back(
				MapCheck::standardCheck(static_cast<MapCheck::StandardCheck>(a), map_, &mapeditor::textureManager()));
		}
	}

	// Run checks
	for (auto& check : active_checks_)
	{
		// Check
		updateStatusText(check->progressText());
		check->doCheck();

		// Add results to list
		for (unsigned b = 0; b < check->nProblems(); b++)
		{
			lb_errors_->Append(check->problemDesc(b));
			check_items_.emplace_back(check.get(), b);
		}
	}

	lb_errors_->Show(true);

	if (lb_errors_->GetCount() > 0)
	{
		updateStatusText(wxString::Format("%d problems found", lb_errors_->GetCount()));
		btn_export_->Enable(true);
	}
	else
		updateStatusText("No problems found");
}

// -----------------------------------------------------------------------------
// Called when a list item is selected
// -----------------------------------------------------------------------------
void MapChecksPanel::onListBoxItem(wxCommandEvent& e)
{
	int selected = lb_errors_->GetSelection();
	if (selected >= 0 && selected < static_cast<int>(check_items_.size()))
		showCheckItem(selected);
}

// -----------------------------------------------------------------------------
// Called when the first fix button is clicked
// -----------------------------------------------------------------------------
void MapChecksPanel::onBtnFix1(wxCommandEvent& e)
{
	int selected = lb_errors_->GetSelection();
	if (selected >= 0 && selected < static_cast<int>(check_items_.size()))
	{
		mapeditor::editContext().beginUndoRecord(wxutil::strToView(btn_fix1_->GetLabel()));
		mapeditor::editContext().clearSelection();
		bool fixed = check_items_[selected].check->fixProblem(
			check_items_[selected].index, 0, &mapeditor::editContext());
		mapeditor::editContext().endUndoRecord(fixed);
		if (fixed)
		{
			refreshList();
			showCheckItem(lb_errors_->GetSelection());
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the second fix button is clicked
// -----------------------------------------------------------------------------
void MapChecksPanel::onBtnFix2(wxCommandEvent& e)
{
	int selected = lb_errors_->GetSelection();
	if (selected >= 0 && selected < static_cast<int>(check_items_.size()))
	{
		mapeditor::editContext().beginUndoRecord(wxutil::strToView(btn_fix2_->GetLabel()));
		mapeditor::editContext().clearSelection();
		bool fixed = check_items_[selected].check->fixProblem(
			check_items_[selected].index, 1, &mapeditor::editContext());
		mapeditor::editContext().endUndoRecord(fixed);
		if (fixed)
		{
			refreshList();
			showCheckItem(lb_errors_->GetSelection());
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the 'edit object' button is clicked
// -----------------------------------------------------------------------------
void MapChecksPanel::onBtnEditObject(wxCommandEvent& e)
{
	int selected = lb_errors_->GetSelection();
	if (selected >= 0 && selected < static_cast<int>(check_items_.size()))
	{
		vector<MapObject*> list;
		list.push_back(check_items_[selected].check->getObject(check_items_[selected].index));
		mapeditor::openMultiObjectProperties(list);
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Export Results' button is clicked
// -----------------------------------------------------------------------------
void MapChecksPanel::onBtnExport(wxCommandEvent& e)
{
	auto               map_name = mapeditor::editContext().mapDesc().name;
	filedialog::FDInfo info;
	if (filedialog::saveFile(
			info,
			"Export Map Check Results",
			"Text Files (*.txt)|*.txt",
			mapeditor::windowWx(),
			map_name + "-Problems"))
	{
		auto text = fmt::format("{} problems found in map {}:\n\n", check_items_.size(), map_name);
		for (auto& item : check_items_)
			text += item.check->problemDesc(item.index) + "\n";
		wxFile file;
		file.Open(info.filenames[0], wxFile::write);
		if (file.IsOpened())
			file.Write(text);
		file.Close();
	}
}
