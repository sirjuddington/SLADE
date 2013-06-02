
#include "Main.h"
#include "WxStuff.h"
#include "ShapeDrawPanel.h"

CVAR(Int, shapedraw_shape, 0, CVAR_SAVE)
CVAR(Bool, shapedraw_centered, false, CVAR_SAVE)
CVAR(Bool, shapedraw_lockratio, false, CVAR_SAVE)
CVAR(Int, shapedraw_sides, 16, CVAR_SAVE)

ShapeDrawPanel::ShapeDrawPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Shape
	string shapes[] = { "Rectangle", "Ellipse" };
	choice_shape = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 2, shapes);
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "Shape:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(choice_shape, 1, wxEXPAND);

	// Centered
	cb_centered = new wxCheckBox(this, -1, "Centered");
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(cb_centered, 0, wxEXPAND|wxRIGHT, 4);

	// Lock ratio (1:1)
	cb_lockratio = new wxCheckBox(this, -1, "1:1 Size");
	hbox->Add(cb_lockratio, 0, wxEXPAND);

	// Sides
	panel_sides = new wxPanel(this, -1);
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	panel_sides->SetSizer(vbox);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 1, wxEXPAND|wxALL, 4);
	spin_sides = new wxSpinCtrl(panel_sides, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER|wxALIGN_LEFT, 12, 1000);
	hbox->Add(new wxStaticText(panel_sides, -1, "Sides:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(spin_sides, 1, wxEXPAND);

	// Set control values
	choice_shape->SetSelection(shapedraw_shape);
	cb_centered->SetValue(shapedraw_centered);
	cb_lockratio->SetValue(shapedraw_lockratio);
	spin_sides->SetValue(shapedraw_sides);

	// Show shape controls with most options (to get minimum height)
	showShapeOptions(1);
	SetMinSize(GetBestSize());

	// Show controls for current shape
	showShapeOptions(shapedraw_shape);

	// Bind events
	choice_shape->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &ShapeDrawPanel::onShapeChanged, this);
	cb_centered->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &ShapeDrawPanel::onCenteredChecked, this);
	cb_lockratio->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &ShapeDrawPanel::onLockRatioChecked, this);
	spin_sides->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &ShapeDrawPanel::onSidesChanged, this);
}

ShapeDrawPanel::~ShapeDrawPanel()
{
}

void ShapeDrawPanel::showShapeOptions(int shape)
{
	// Remove all extra options
	GetSizer()->Detach(panel_sides);
	panel_sides->Show(false);

	// Polygon/Ellipse options
	if (shape == 1)
	{
		// Sides
		GetSizer()->Add(panel_sides, 0, wxEXPAND|wxTOP, 4);
		panel_sides->Show(true);
	}

	Layout();
}


void ShapeDrawPanel::onShapeChanged(wxCommandEvent& e)
{
	shapedraw_shape = choice_shape->GetSelection();
	showShapeOptions(shapedraw_shape);
}

void ShapeDrawPanel::onCenteredChecked(wxCommandEvent& e)
{
	shapedraw_centered = cb_centered->GetValue();
}

void ShapeDrawPanel::onLockRatioChecked(wxCommandEvent& e)
{
	shapedraw_lockratio = cb_lockratio->GetValue();
}

void ShapeDrawPanel::onSidesChanged(wxSpinEvent& e)
{
	shapedraw_sides = spin_sides->GetValue();
}
