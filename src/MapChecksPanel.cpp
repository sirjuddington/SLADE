
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
	sizer->Add(gb_sizer, 0, wxEXPAND|wxALL, 10);

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

	// Error list
	lb_errors = new wxListBox(this, -1);
	sizer->Add(lb_errors, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);

	// Status text
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);
	label_status = new wxStaticText(this, -1, "");
	hbox->Add(label_status, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);

	// Check button
	btn_check = new wxButton(this, -1, "Check");
	hbox->Add(btn_check, 0, wxEXPAND);

	// Bind events
	btn_check->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MapChecksPanel::onBtnCheck, this);
	lb_errors->Bind(wxEVT_LISTBOX, &MapChecksPanel::onListBoxItem, this);

	// Check all by default
	cb_missing_tex->SetValue(true);
	cb_special_tags->SetValue(true);
	cb_intersecting->SetValue(true);
	cb_overlapping->SetValue(true);
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

void MapChecksPanel::checkMissingTextures()
{
	// Do check
	updateStatusText("Checking for missing textures...");
	vector<MapChecks::missing_tex_t> missing = MapChecks::checkMissingTextures(map);

	for (unsigned a = 0; a < missing.size(); a++)
	{
		string line = S_FMT("Line %d missing ", missing[a].line->getIndex());
		switch (missing[a].part)
		{
		case TEX_FRONT_UPPER: line += "front upper texture"; break;
		case TEX_FRONT_MIDDLE: line += "front middle texture"; break;
		case TEX_FRONT_LOWER: line += "front lower texture"; break;
		case TEX_BACK_UPPER: line += "back upper texture"; break;
		case TEX_BACK_MIDDLE: line += "back middle texture"; break;
		case TEX_BACK_LOWER: line += "back lower texture"; break;
		default: break;
		}

		lb_errors->Append(line);
		objects.push_back(missing[a].line);
	}
}

void MapChecksPanel::checkSpecialTags()
{
	// Do check
	updateStatusText("Checking for missing special tags...");
	vector<MapLine*> lines = MapChecks::checkSpecialTags(map);

	for (unsigned a = 0; a < lines.size(); a++)
	{
		int special = lines[a]->getSpecial();
		ActionSpecial* as = theGameConfiguration->actionSpecial(special);
		lb_errors->Append(S_FMT("Line %d: Special %d (%s) requires a tag", lines[a]->getIndex(), special, as->getName()));
		objects.push_back(lines[a]);
	}
}

void MapChecksPanel::checkIntersectingLines()
{
	// Do check
	updateStatusText("Checking for intersecting lines...");
	vector<MapChecks::intersect_line_t> lines = MapChecks::checkIntersectingLines(map);

	for (unsigned a = 0; a < lines.size(); a++)
	{
		lb_errors->Append(S_FMT("Lines %d and %d are intersecting", lines[a].line1->getIndex(), lines[a].line2->getIndex()));
		objects.push_back(lines[a].line1);
	}
}

void MapChecksPanel::checkOverlappingLines()
{
	// Do check
	updateStatusText("Checking for overlapping lines...");
	vector<MapChecks::intersect_line_t> lines = MapChecks::checkOverlappingLines(map);

	for (unsigned a = 0; a < lines.size(); a++)
	{
		lb_errors->Append(S_FMT("Lines %d and %d are overlapping", lines[a].line1->getIndex(), lines[a].line2->getIndex()));
		objects.push_back(lines[a].line1);
	}
}

void MapChecksPanel::onBtnCheck(wxCommandEvent& e)
{
	lb_errors->Show(false);
	lb_errors->Clear();
	objects.clear();

	if (cb_missing_tex->GetValue())
		checkMissingTextures();

	if (cb_special_tags->GetValue())
		checkSpecialTags();

	if (cb_intersecting->GetValue())
		checkIntersectingLines();

	if (cb_overlapping->GetValue())
		checkOverlappingLines();

	lb_errors->Show(true);

	if (lb_errors->GetCount() > 0)
		updateStatusText(S_FMT("%d problems found", lb_errors->GetCount()));
	else
		updateStatusText("No problems found");
}

void MapChecksPanel::onListBoxItem(wxCommandEvent& e)
{
	int selected = lb_errors->GetSelection();
	if (selected >= 0 && selected < (int)objects.size())
	{
		MapObject* obj = objects[selected];
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

		theMapEditor->mapEditor().showItem(obj->getIndex());
	}
}
