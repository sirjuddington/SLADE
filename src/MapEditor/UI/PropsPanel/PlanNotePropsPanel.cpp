
#include "Main.h"
#include "PlanNotePropsPanel.h"
#include "UI/ColourBox.h"
#include "MapEditor/Edit/Planning.h"

namespace
{
	struct IconDef
	{
		string icon;
		string name;
	};
	vector<IconDef> icons =
	{
		{ "",			"None" },
		{ "ammo",		"Ammo" },
		{ "armour",		"Armour" },
		{ "camera",		"Camera" },
		{ "spot",		"Cogwheel" },
		{ "weapon",		"Gun" },
		{ "health",		"Health" },
		{ "key",		"Key" },
		{ "light",		"Light" },
		{ "minus",		"Minus" },
		{ "particle",	"Particles" },
		{ "unknown",	"Question Mark" },
		{ "powerup",	"Star" },
		{ "slope",		"Slope" },
		{ "sound",		"Sound" },
	};
}

PlanNotePropsPanel::PlanNotePropsPanel(wxWindow* parent) : PropsPanelBase(parent)
{
	auto gbsizer = new wxGridBagSizer(4, 4);
	SetSizer(gbsizer);

	// Text
	int row = 0;
	text_note_ = new wxTextCtrl(this, -1, "");
	gbsizer->Add(new wxStaticText(this, -1, "Text:"), { row, 0 }, { 1, 4 }, wxEXPAND);
	gbsizer->Add(text_note_, { ++row, 0 }, { 1, 4 }, wxEXPAND);

	// Detail text
	text_detail_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	gbsizer->Add(new wxStaticText(this, -1, "Detail:"), { ++row, 0 }, { 1, 4 }, wxEXPAND);
	gbsizer->Add(text_detail_, { ++row, 0 }, { 1, 4 }, wxEXPAND);

	// Colour
	colbox_colour_ = new ColourBox(this, -1);
	gbsizer->Add(new wxStaticText(this, -1, "Colour:"), { ++row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gbsizer->Add(colbox_colour_, { row, 1 }, { 1, 1 }, wxEXPAND);

	// Icon
	choice_icon_ = new wxChoice(this, -1);
	gbsizer->Add(new wxStaticText(this, -1, "Icon:"), { row, 2 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gbsizer->Add(choice_icon_, { row, 3 }, { 1, 1 }, wxEXPAND);

	gbsizer->AddGrowableRow(3, 1);
	gbsizer->AddGrowableCol(3, 1);

	// Setup icon list
	for (auto& icon : icons)
		choice_icon_->Append(icon.name);
}

void PlanNotePropsPanel::openObjects(vector<MapObject*>& objects)
{
	this->objects = objects;
	auto note = (MapEditor::PlanNote*)objects[0];

	// Text/Detail
	if (objects.size() > 1)
	{
		text_note_->SetValue("");
		text_detail_->SetValue("");
	}
	else
	{
		text_note_->SetValue(note->text());
		text_detail_->SetValue(note->detail());
	}

	// Colour
	colbox_colour_->setColour(note->colour());

	// Icon
	choice_icon_->SetSelection(0);
	for (auto a = 0u; a < icons.size(); ++a)
		if (S_CMPNOCASE(icons[a].icon, note->icon()))
		{
			choice_icon_->SetSelection(a);
			break;
		}
}

void PlanNotePropsPanel::applyChanges()
{
	for (auto object : objects)
	{
		auto note = (MapEditor::PlanNote*)object;

		if (!text_note_->GetValue().empty())
			note->setText(text_note_->GetValue());
		if (!text_detail_->GetValue().empty())
			note->setDetail(text_detail_->GetValue());

		note->setColour(colbox_colour_->getColour());
		note->setIcon(icons[choice_icon_->GetSelection()].icon);
	}
}
